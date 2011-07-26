/*
 * BasicSchedulerTaskExecutionContext.h
 *
 *  Created on: 13.04.2011
 *      Author: Martin Wimmer
 *     License: Pheet license
 */

#ifndef BASICSCHEDULERTASKEXECUTIONCONTEXT_H_
#define BASICSCHEDULERTASKEXECUTIONCONTEXT_H_

#include "../../../../settings.h"
#include "../../common/CPUThreadExecutor.h"
#include "../../common/FinishRegion.h"
#include "../../../misc/atomics.h"
#include "../../../misc/bitops.h"
#include "../../../misc/type_traits.h"

#include <vector>
#include <assert.h>
#include <iostream>

namespace pheet {

struct BasicSchedulerTaskExecutionContextStackElement {
	// Modified by local thread. Incremented when task is spawned, decremented when finished
	size_t num_spawned;

	// Only modified by other threads. Stolen tasks that were finished (including spawned tasks)
	size_t num_finished_remote;

	// Pointer to num_finished_remote of another thread (the one we stole tasks from)
	BasicSchedulerTaskExecutionContextStackElement* parent;
};

template <class TaskExecutionContext>
struct BasicSchedulerTaskExecutionContextLevelDescription {
	TaskExecutionContext** partners;
	procs_t num_partners;
	procs_t local_id;
	procs_t total_size;
};

template <class TaskExecutionContext>
struct BasicSchedulerTaskExecutionContextDequeItem {
	BasicSchedulerTaskExecutionContextDequeItem();

	typename TaskExecutionContext::Task* task;
	typename TaskExecutionContext::StackElement* stack_element;

	bool operator==(BasicSchedulerTaskExecutionContextDequeItem<TaskExecutionContext> const& other) const;
	bool operator!=(BasicSchedulerTaskExecutionContextDequeItem<TaskExecutionContext> const& other) const;
};

template <class TaskExecutionContext>
BasicSchedulerTaskExecutionContextDequeItem<TaskExecutionContext>::BasicSchedulerTaskExecutionContextDequeItem()
: task(NULL), stack_element(NULL) {

}

template <class TaskExecutionContext>
bool BasicSchedulerTaskExecutionContextDequeItem<TaskExecutionContext>::operator==(BasicSchedulerTaskExecutionContextDequeItem<TaskExecutionContext> const& other) const {
	return other.task == task;
}

template <class TaskExecutionContext>
bool BasicSchedulerTaskExecutionContextDequeItem<TaskExecutionContext>::operator!=(BasicSchedulerTaskExecutionContextDequeItem<TaskExecutionContext> const& other) const {
	return other.task != task;
}


template <class TaskExecutionContext>
class nullable_traits<BasicSchedulerTaskExecutionContextDequeItem<TaskExecutionContext> > {
public:
	static BasicSchedulerTaskExecutionContextDequeItem<TaskExecutionContext> const null_value;
};

template <class TaskExecutionContext>
BasicSchedulerTaskExecutionContextDequeItem<TaskExecutionContext> const nullable_traits<BasicSchedulerTaskExecutionContextDequeItem<TaskExecutionContext> >::null_value;

template <class Scheduler, template <typename T> class StealingDeque>
class BasicSchedulerTaskExecutionContext {
public:
	typedef BasicSchedulerTaskExecutionContextLevelDescription<BasicSchedulerTaskExecutionContext<Scheduler, StealingDeque> > LevelDescription;
	typedef typename Scheduler::Backoff Backoff;
	typedef typename Scheduler::CPUHierarchy CPUHierarchy;
	typedef typename Scheduler::Task Task;
	typedef BasicSchedulerTaskExecutionContextStackElement StackElement;
	typedef BasicSchedulerTaskExecutionContextDequeItem<BasicSchedulerTaskExecutionContext<Scheduler, StealingDeque> > DequeItem;

	BasicSchedulerTaskExecutionContext(vector<LevelDescription*> const* levels, vector<typename CPUHierarchy::CPUDescriptor*> const* cpus, typename Scheduler::State* scheduler_state);
	~BasicSchedulerTaskExecutionContext();

	void join();

	template<class CallTaskType, typename ... TaskParams>
		void finish(TaskParams ... params);

	template<class CallTaskType, typename ... TaskParams>
		void call(TaskParams ... params);

	template<class CallTaskType, typename ... TaskParams>
		void spawn(TaskParams ... params);

private:
	void run();
	void execute_task(Task* task, StackElement* parent);
	void main_loop();
	void process_queue();
	void wait_for_finish(StackElement* parent);

	void empty_stack();
	StackElement* create_finish_block(StackElement* parent);
	void signal_task_completion(StackElement* stack_element);
	void finalize_stack_element(StackElement* element, StackElement* parent);

	void start_finish_region();
	void end_finish_region();

	static size_t const stack_size;
	StackElement* stack;
	StackElement* current_task_parent;
	size_t stack_filled;
	size_t stack_block;

	LevelDescription* levels;
	procs_t num_levels;

	CPUThreadExecutor<typename CPUHierarchy::CPUDescriptor, BasicSchedulerTaskExecutionContext<Scheduler, StealingDeque> > thread_executor;

	typename Scheduler::State* scheduler_state;

	size_t max_queue_length;
	StealingDeque<DequeItem> stealing_deque;

	friend class CPUThreadExecutor<typename CPUHierarchy::CPUDescriptor, BasicSchedulerTaskExecutionContext<Scheduler, StealingDeque>>;
	friend class Scheduler::Finish;
};

template <class Scheduler, template <typename T> class StealingDeque>
size_t const BasicSchedulerTaskExecutionContext<Scheduler, StealingDeque>::stack_size = 256;

template <class Scheduler, template <typename T> class StealingDeque>
BasicSchedulerTaskExecutionContext<Scheduler, StealingDeque>::BasicSchedulerTaskExecutionContext(vector<LevelDescription*> const* levels, vector<typename CPUHierarchy::CPUDescriptor*> const* cpus, typename Scheduler::State* scheduler_state)
: stack_filled(0), stack_block(0), num_levels(levels->size()), thread_executor(cpus, this), scheduler_state(scheduler_state), max_queue_length(find_last_bit_set((*levels)[0]->total_size - 1) << 4), stealing_deque(max_queue_length) {
	stack = new StackElement[stack_size];
	this->levels = new LevelDescription[num_levels];
	procs_t local_id = 0;
	for(ptrdiff_t i = num_levels - 1; i >= 0; i--) {
		this->levels[i].partners = (*levels)[i]->partners;
		this->levels[i].num_partners = (*levels)[i]->num_partners;
		local_id += (*levels)[i]->local_id;
		this->levels[i].local_id = local_id;
		this->levels[i].total_size = (*levels)[i]->total_size;
	}

	thread_executor.run();
}

template <class Scheduler, template <typename T> class StealingDeque>
BasicSchedulerTaskExecutionContext<Scheduler, StealingDeque>::~BasicSchedulerTaskExecutionContext() {
	delete[] stack;
	delete[] levels;
}

template <class Scheduler, template <typename T> class StealingDeque>
void BasicSchedulerTaskExecutionContext<Scheduler, StealingDeque>::join() {
	thread_executor.join();
}


template <class Scheduler, template <typename T> class StealingDeque>
void BasicSchedulerTaskExecutionContext<Scheduler, StealingDeque>::run() {
	scheduler_state->state_barrier.wait(0, 1);

	Task* startup_task = scheduler_state->startup_task;
	if(startup_task != NULL) {
		if(PTR_CAS(&(scheduler_state->startup_task), startup_task, NULL)) {
			execute_task(startup_task, NULL);
		}
	}
	main_loop();

	scheduler_state->state_barrier.barrier(1, levels[0].total_size);

	// Now we can safely finish execution
}

template <class Scheduler, template <typename T> class StealingDeque>
void BasicSchedulerTaskExecutionContext<Scheduler, StealingDeque>::execute_task(Task* task, StackElement* parent) {
	if(parent < stack || (parent >= (stack + stack_size))) {
		// to prevent thrashing on the parent finish block (owned by another thread), we create a new finish block local to the thread

		// Perform cleanup on finish stack
		empty_stack();

		// Create new stack element for finish
		parent = create_finish_block(parent);
	}

	// Store parent (needed for spawns inside the task)
	current_task_parent = parent;

	// Execute task
	(*task)(*this);

	// Signal that we finished executing this task
	signal_task_completion(parent);
}

template <class Scheduler, template <typename T> class StealingDeque>
void BasicSchedulerTaskExecutionContext<Scheduler, StealingDeque>::main_loop() {
	while(true) {
		// Make sure our queue is empty
		process_queue();

		{	// Local scope so we have a new backoff object
			Backoff bo;
			DequeItem di;
			while(true) {
				// Finalize elements in stack
				procs_t next_rand = random();

				// We do not steal from the last level as there are no partners
				procs_t level = num_levels - 1;
				while(level > 0) {
					level--;
					// For all except the last level we assume num_partners > 0
					assert(levels[level].num_partners > 0);
					assert(levels[level].partners[next_rand % levels[level].num_partners] != this);
					di = levels[level].partners[next_rand % levels[level].num_partners]->stealing_deque.steal_push(this->stealing_deque);
				//	di = levels[level].partners[next_rand % levels[level].num_partners]->stealing_deque.steal();

					if(di.task != NULL) {
						break;
					}
				}
				if(di.task == NULL) {
					if(scheduler_state->current_state >= 2) {
						return;
					}
					bo.backoff();
				}
				else {
					execute_task(di.task, di.stack_element);
					delete di.task;
					break;
				}
			}
		}
	}
}

template <class Scheduler, template <typename T> class StealingDeque>
void BasicSchedulerTaskExecutionContext<Scheduler, StealingDeque>::wait_for_finish(StackElement* parent) {
	size_t old_stack_block = stack_block;
	stack_block = parent - stack + 1;
	while(true) {
		// TODO: try a policy where we do not need to empty our queues before we notice the finish
		// (currently not implemented for simplicity)

		// Make sure our queue is empty
		process_queue();

		{	// Local scope so we have a new backoff object
			Backoff bo;
			DequeItem di;
			while(true) {
				// Finalize elements in stack
				procs_t next_rand = random();

				// We do not steal from the last level as there are no partners
				procs_t level = num_levels - 1;
				while(level > 0) {
					level--;
					// For all except the last level we assume num_partners > 0
					assert(levels[level].num_partners > 0);
					assert(levels[level].partners[next_rand % levels[level].num_partners] != this);
					di = levels[level].partners[next_rand % levels[level].num_partners]->stealing_deque.steal_push(this->stealing_deque);
				//	di = levels[level].partners[next_rand % levels[level].num_partners]->stealing_deque.steal();

					if(di.task != NULL) {
						break;
					}
				}
				if(di.task == NULL) {
					if(parent->num_finished_remote == parent->num_spawned) {
						stack_block = old_stack_block;
						return;
					}
					bo.backoff();
				}
				else {
					execute_task(di.task, di.stack_element);
					delete di.task;
					break;
				}
			}
		}
	}
}

template <class Scheduler, template <typename T> class StealingDeque>
void BasicSchedulerTaskExecutionContext<Scheduler, StealingDeque>::process_queue() {
	DequeItem di = stealing_deque.pop();
	while(di.task != NULL) {
		// Warning, no distinction between locally spawned tasks and remote tasks
		// But this makes it easier with the finish construct, etc.
		// Otherwise we would have to empty our deque on the next finish call
		// which is bad for balancing
		execute_task(di.task, di.stack_element);
		delete di.task;
		di = stealing_deque.pop();
	}
}

template <class Scheduler, template <typename T> class StealingDeque>
typename BasicSchedulerTaskExecutionContext<Scheduler, StealingDeque>::StackElement*
BasicSchedulerTaskExecutionContext<Scheduler, StealingDeque>::create_finish_block(StackElement* parent) {
	assert(stack_filled < stack_size);

	stack[stack_filled].num_finished_remote = 0;
	// As we create it out of a parent region that is waiting for completion of a single task, we can already add this one task here
	stack[stack_filled].num_spawned = 1;
	stack[stack_filled].parent = parent;

	++stack_filled;

	return &(stack[stack_filled - 1]);
}

/*
 * empty stack but not below limit
 */
template <class Scheduler, template <typename T> class StealingDeque>
void BasicSchedulerTaskExecutionContext<Scheduler, StealingDeque>::empty_stack() {
	while(stack_filled > stack_block) {
		size_t se = stack_filled - 1;
		if(stack[se].num_spawned == stack[se].num_finished_remote) {
			finalize_stack_element(&(stack[se]), stack[se].parent);

			stack_filled = se;

			// When parent is set to NULL, some thread is finalizing/has finalized this stack element (otherwise we would have an error)
			assert(stack[stack_filled].parent == NULL);
		}
		else {
			break;
		}
	}
}

template <class Scheduler, template <typename T> class StealingDeque>
void BasicSchedulerTaskExecutionContext<Scheduler, StealingDeque>::signal_task_completion(StackElement* stack_element) {
	StackElement* parent = stack_element->parent;

	if(stack_element >= stack && (stack_element < (stack + stack_size))) {
		--(stack_element->num_spawned);

		// Memory fence is unfortunately required to guarantee that some thread finalizes the stack_element
		// TODO: prove that the fence is enough
		MEMORY_FENCE();
	}
	else {
		// Increment num_finished_remote of parent
		SIZET_ATOMIC_ADD(&(stack_element->num_finished_remote), 1);
	}
	if(stack_element->num_spawned == stack_element->num_finished_remote) {
		finalize_stack_element(stack_element, parent);
	}
}

template <class Scheduler, template <typename T> class StealingDeque>
void BasicSchedulerTaskExecutionContext<Scheduler, StealingDeque>::finalize_stack_element(StackElement* element, StackElement* parent) {
	if(parent != NULL) {
		if(element->num_spawned == 0) {
			// No tasks processed remotely - no need for atomic ops
			element->parent = NULL;
			signal_task_completion(parent);
		}
		else {
			if(PTR_CAS(&(element->parent), parent, NULL)) {
				signal_task_completion(parent);
			}
		}
	}
	else {
		// Root element - we can shut down the scheduler
		scheduler_state->current_state = 2;
	}
}

template <class Scheduler, template <typename T> class StealingDeque>
void BasicSchedulerTaskExecutionContext<Scheduler, StealingDeque>::start_finish_region() {
	// Perform cleanup on finish stack
	empty_stack();

	// Create new stack element for finish
	current_task_parent = create_finish_block(current_task_parent);
}

template <class Scheduler, template <typename T> class StealingDeque>
void BasicSchedulerTaskExecutionContext<Scheduler, StealingDeque>::end_finish_region() {
	// Retrieve old parent (before the pointer is set to NULL)
	StackElement* old_parent = current_task_parent->parent;

	// Signal that we finished executing this task
	signal_task_completion(current_task_parent);

	// Process other tasks until this task has been finished
	wait_for_finish(current_task_parent);

	// Restore old parent
	current_task_parent = old_parent;
}

template <class Scheduler, template <typename T> class StealingDeque>
template<class CallTaskType, typename ... TaskParams>
void BasicSchedulerTaskExecutionContext<Scheduler, StealingDeque>::finish(TaskParams ... params) {
	start_finish_region();

	call<CallTaskType>(params ...);

	end_finish_region();
}

template <class Scheduler, template <typename T> class StealingDeque>
template<class CallTaskType, typename ... TaskParams>
void BasicSchedulerTaskExecutionContext<Scheduler, StealingDeque>::spawn(TaskParams ... params) {
	if(stealing_deque.get_length() >= max_queue_length) {
		call<CallTaskType>(params ...);
	}
	else {
		CallTaskType* task = new CallTaskType(params ...);
		assert(stack_filled > 0);
		stack[stack_filled - 1].num_spawned++;
		DequeItem di;
		di.task = task;
		di.stack_element = current_task_parent;
		stealing_deque.push(di);
	}
}

template <class Scheduler, template <typename T> class StealingDeque>
template<class CallTaskType, typename ... TaskParams>
void BasicSchedulerTaskExecutionContext<Scheduler, StealingDeque>::call(TaskParams ... params) {
	// Create task
	CallTaskType task(params ...);
	// Execute task
	task(*this);
}

}

#endif /* BASICSCHEDULERTASKEXECUTIONCONTEXT_H_ */
