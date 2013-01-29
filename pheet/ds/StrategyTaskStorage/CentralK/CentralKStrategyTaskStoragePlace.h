/*
 * CentralKStrategyTaskStoragePlace.h
 *
 *  Created on: 24.10.2012
 *      Author: mwimmer
 */

#ifndef CENTRALKSTRATEGYTASKSTORAGEPLACE_H_
#define CENTRALKSTRATEGYTASKSTORAGEPLACE_H_

#include "CentralKStrategyTaskStorageDataBlock.h"
#include "CentralKStrategyTaskStorageItem.h"
#include "CentralKStrategyTaskStoragePerformanceCounters.h"

#include <pheet/memory/ItemReuse/ItemReuseMemoryManager.h>

namespace pheet {

template <class Pheet, class Item>
struct CentralKStrategyTaskStorageItemReference {
	Item* item;
	size_t position;
	// If strategy equals item->strategy => locally created
	typename Pheet::Scheduler::BaseStrategy* strategy;
};

template <class Pheet, class Ref>
class CentralKStrategyTaskStorageStrategyRetriever {
public:
	typedef CentralKStrategyTaskStorageStrategyRetriever<Pheet, Ref> Self;

	CentralKStrategyTaskStorageStrategyRetriever() {}

	typename Pheet::Scheduler::BaseStrategy* operator()(Ref const& ref) {
		return ref.strategy;
	}

	inline bool is_active(Ref const& ref) {
		return ref.item.position == ref.position;
	}

	inline void mark_removed(Ref& ref) {
	//	item.block->mark_removed(item.index, task_storage->get_current_view());
	}

private:
};

template <class Pheet, class TaskStorage, typename TT, template <class SP, typename ST, class SR> class StrategyHeapT, size_t BlockSize, size_t Tests, bool LocalKPrio>
class CentralKStrategyTaskStoragePlace {
public:
	typedef CentralKStrategyTaskStoragePlace<Pheet, TaskStorage, TT, StrategyHeapT, BlockSize, Tests, LocalKPrio> Self;

	typedef CentralKStrategyTaskStorageDataBlock<Pheet, Self, TT, BlockSize, Tests> DataBlock;

	typedef TT T;
	typedef CentralKStrategyTaskStorageItem<Pheet, Self, TT> Item;
	typedef CentralKStrategyTaskStorageItemReference<Pheet, Item> Ref;
	typedef CentralKStrategyTaskStorageStrategyRetriever<Pheet, Ref> StrategyRetriever;

	typedef StrategyHeapT<Pheet, Ref, StrategyRetriever> StrategyHeap;
	typedef CentralKStrategyTaskStoragePerformanceCounters<Pheet, typename StrategyHeap::PerformanceCounters>
			PerformanceCounters;

	typedef ItemReuseMemoryManager<Pheet, Item, CentralKStrategyTaskStorageItemReuseCheck<Item> > ItemMemoryManager;
	typedef ItemReuseMemoryManager<Pheet, DataBlock, CentralKStrategyTaskStorageDataBlockReuseCheck<DataBlock> > DataBlockMemoryManager;

	typedef typename Pheet::Scheduler::Place SchedulerPlace;

	CentralKStrategyTaskStoragePlace(TaskStorage* task_storage, SchedulerPlace* scheduler_place, PerformanceCounters pc)
	:pc(pc), task_storage(task_storage), heap(sr, pc.strategy_heap_performance_counters), head(0) {
		DataBlock* tmp = task_storage->start_block;
		if(tmp == nullptr) {
			// This assumes the first place is initialized before all others, otherwise synchronization would be needed!
			tmp = &(data_blocks.acquire_item());
			tmp->init_first(task_storage->get_num_places());
			task_storage->start_block = tmp;
		}
		tail_block = tmp;
		head_block = tmp;
	}

	~CentralKStrategyTaskStoragePlace() {
		// Check whether this is needed at all, or if scheduler only terminates if heap is empty
		while(!heap.empty()) {
			Ref r = heap.pop();
			// All items should have been processed
			pheet_assert(r.position != r.item->position);
		//	if(r.strategy != r.item->strategy) {
				delete r.strategy;
		//	}
			r.strategy = nullptr;
		}
	}

	template <class Strategy>
	void push(Strategy&& s, T data) {
		Item& it = items.acquire_item();
		pheet_assert(it.strategy == nullptr);
		it.strategy = new Strategy(s);
		it.data = data;
		it.item_push = &Self::template item_push<Strategy>;
		it.owner = this;

		size_t old_tail = task_storage->tail;
		size_t cur_tail = old_tail;

		while(!tail_block->put(&(task_storage->head), &(task_storage->tail), cur_tail, &it, pc.data_block_performance_counters)) {
			if(tail_block->get_next() == nullptr) {
				DataBlock& next_block = data_blocks.acquire_item();
				pc.num_blocks_created.incr();
				tail_block->add_block(&next_block, task_storage->get_num_places());
			}
			pheet_assert(tail_block->get_next() != nullptr);
			pheet_assert(tail_block->get_offset() + BlockSize == tail_block->get_next()->get_offset());
			tail_block = tail_block->get_next();
		}

		if(cur_tail != old_tail) {
			size_t nold_tail = task_storage->tail;
			ptrdiff_t diff = (ptrdiff_t)cur_tail - (ptrdiff_t)nold_tail;
			while(diff > 0) {
				if(SIZET_CAS(&(task_storage->tail), nold_tail, cur_tail)) {
					break;
				}
				nold_tail = task_storage->tail;
				diff = (ptrdiff_t)cur_tail - (ptrdiff_t)nold_tail;
			}
		}

		Ref r;
		r.item = &it;
		r.position = it.position;
		r.strategy = new Strategy(std::move(s));

		heap.template push<Strategy>(std::move(r));
	}

	T pop() {
		update_heap();

		while(heap.size() > 0) {
			Ref r = heap.pop();

			pheet_assert(r.strategy != nullptr);
		//	if(r.strategy != r.item->strategy) {
				delete r.strategy;
		//	}

			if(r.item->position == r.position) {
				if(SIZET_CAS(&(r.item->position), r.position, r.position + 1)) {
					pc.num_successful_takes.incr();
					return r.item->data;
				}
				else {
					pc.num_unsuccessful_takes.incr();
					pc.num_taken_heap_items.incr();
				}
			}
			else {
				pc.num_taken_heap_items.incr();
			}
			update_heap();
		}

		// Heap is empty. Try getting random item directly from fifo queue (without heap)

		// Tries to find and take a random item from the queue inside the block
		// Synchronization and verification all take place inside this method.
		// Returned item is free to use by this thread
		Item* item = head_block->take_rand_filled(head, pc.data_block_performance_counters, pc.num_unsuccessful_takes, pc.num_successful_takes);
		if(item != nullptr) {
			return item->data;
		}

		return nullable_traits<T>::null_value;
	}

	template <class Strategy>
	void item_push(Item* item, size_t position) {
		Ref r;
		r.item = item;
		r.position = position;
		// TODO: check whether the position is still valid, otherwise end function

		Strategy* s = new Strategy(*reinterpret_cast<Strategy*>(item->strategy));
		r.strategy = s;

		heap.template push<Strategy>(std::move(r));
	}

	bool is_full() {
		return false;
	}

	TaskStorage* get_central_task_storage() {
		return task_storage;
	}

	size_t size() {
		return heap.size();
	}
private:
	void update_heap() {
		// Check whether update is necessary
		if(!heap.empty()) {
			if(LocalKPrio) {
				size_t pos = heap.peek().position;
				ptrdiff_t diff = ((ptrdiff_t)head) - ((ptrdiff_t) pos);
				if(diff >= 0) {
					return;
				}
			}
			else {
				if(task_storage->tail == head) {
					return;
				}
			}
		}

		process_until(task_storage->head);
		while(head != task_storage->tail) {
			if(!heap.empty() && LocalKPrio) {
				size_t pos = heap.peek().position;
				ptrdiff_t diff = ((ptrdiff_t)head) - ((ptrdiff_t) pos);
				if(diff >= 0) {
					return;
				}
			}
			// If we fail to update, some other thread must have succeeded
			if(task_storage->head == head) {
				SIZET_CAS(&(task_storage->head), head, task_storage->tail);
			}
			process_until(task_storage->head);
		}
	}

	void process_until(size_t limit) {
		while(head != limit) {
			while(!head_block->in_block(head)) {
				DataBlock* next = head_block->get_next();
				if(next == nullptr) {
					MEMORY_FENCE();
					next = head_block->get_next();
					pheet_assert(next != nullptr);
				}
				if(head_block == tail_block) { // Make sure tail block doesn't lag behind
					tail_block = next;
				}
				pheet_assert((ptrdiff_t)tail_block->get_offset() - (ptrdiff_t)head_block->get_offset() >= 0);
				head_block->deregister();
				head_block = next;
			}

			Item* item = head_block->get_item(head);
			if(item != nullptr && item->owner != this && item->position == head) {
				// Push item to local heap
				auto ip = item->item_push;
				(this->*ip)(item, head);
			}

			++head;
		}
		while(!head_block->in_block(head)) {
			pheet_assert(head_block->get_next() != nullptr);
			DataBlock* next = head_block->get_next();
			if(head_block == tail_block) { // Make sure tail block doesn't lag behind
				tail_block = next;
			}
			pheet_assert(next != nullptr);
			pheet_assert((ptrdiff_t)tail_block->get_offset() - (ptrdiff_t)head_block->get_offset() >= 0);
			head_block->deregister();
			head_block = next;
		}
	}

	PerformanceCounters pc;

	TaskStorage* task_storage;
	StrategyRetriever sr;
	StrategyHeap heap;

	DataBlock* tail_block;
	DataBlock* head_block;
	size_t head;

	ItemMemoryManager items;
	DataBlockMemoryManager data_blocks;
};

} /* namespace pheet */
#endif /* CENTRALKSTRATEGYTASKSTORAGEPLACE_H_ */
