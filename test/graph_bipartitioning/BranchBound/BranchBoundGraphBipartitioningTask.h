/*
 * BranchBoundGraphBipartitioningTask.h
 *
 *  Created on: 08.09.2011
 *   Author(s): Martin Wimmer
 *     License: Ask author
 */

#ifndef BRANCHBOUNDGRAPHBIPARTITIONINGTASK_H_
#define BRANCHBOUNDGRAPHBIPARTITIONINGTASK_H_

#include "../graph_helpers.h"
#include "../../../primitives/Reducer/Max/MaxReducer.h"
#include "../../../primitives/Backoff/Exponential/ExponentialBackoff.h"

#include <algorithm>

/*
 *
 */
namespace pheet {



template <class Pheet, class LowerBound, class NextVertex, size_t MaxSize>
class BranchBoundGraphBipartitioningTask : public Pheet::Task {
public:
	typedef BranchBoundGraphBipartitioningTask<Pheet, LowerBound, NextVertex, MaxSize> BBTask;
	typedef GraphBipartitioningSolution<MaxSize> Solution;
	typedef MaxReducer<Pheet, Solution> SolutionReducer;

	BranchBoundGraphBipartitioningTask(GraphVertex* graph, size_t size, size_t k, SolutionReducer& best, size_t* set1, size_t set1_size, size_t* set2, size_t set2_size, size_t* ub, size_t lb);
	virtual ~BranchBoundGraphBipartitioningTask();

	virtual void operator()();

private:
	void prepare_solution(size_t* set1, size_t set1_size, size_t* set2, size_t set2_size);
	size_t* create_new_set(size_t* set, size_t new_el, size_t set_size, size_t max_size);
	size_t* clone_set(size_t* set, size_t set_size, size_t max_size);

	GraphVertex* graph;
	size_t size;
	size_t k;
	SolutionReducer best;
	size_t* set1;
	size_t set1_size;
	size_t* set2;
	size_t set2_size;
	size_t* ub;
	size_t lb;

	LowerBound lb_calc;
	NextVertex nv_calc;
};

template <class Pheet, class LowerBound, class NextVertex, size_t MaxSize>
BranchBoundGraphBipartitioningTask<Pheet, LowerBound, NextVertex, MaxSize>::BranchBoundGraphBipartitioningTask(GraphVertex* graph, size_t size, size_t k, SolutionReducer& best, size_t* set1, size_t set1_size, size_t* set2, size_t set2_size, size_t* ub, size_t lb)
: graph(graph), size(size), k(k), best(best), set1(set1), set1_size(set1_size), set2(set2), set2_size(set2_size), ub(ub), lb(lb) {

}

template <class Pheet, class LowerBound, class NextVertex, size_t MaxSize>
BranchBoundGraphBipartitioningTask<Pheet, LowerBound, NextVertex, MaxSize>::~BranchBoundGraphBipartitioningTask() {
	delete[] set1;
	delete[] set2;
}

template <class Pheet, class LowerBound, class NextVertex, size_t MaxSize>
void BranchBoundGraphBipartitioningTask<Pheet, LowerBound, NextVertex, MaxSize>::operator()() {
	if(lb >= *ub) {
		return;
	}
	assert(set1_size < k);
	assert(set2_size < size - k);
	size_t next = nv_calc(graph, size, k, set1, set1_size, set2, set2_size);

//	set1[set1_size] = next;
	size_t* new_set = create_new_set(set1, next, set1_size, k);
	if(set1_size == k - 1) {
		size_t* tmp_set = clone_set(set2, set2_size, size - k);
		prepare_solution(new_set, set1_size + 1, tmp_set, set2_size);
		delete[] tmp_set;
		delete[] new_set;
	}
	else {
		size_t sub_lb = lb_calc(graph, size, k, new_set, set1_size + 1, set2, set2_size);
		if(sub_lb < *ub) {
			Pheet::template spawn<BBTask>(graph, size, k, best, new_set, set1_size + 1, clone_set(set2, set2_size, size - k), set2_size, ub, sub_lb);
		}
		else {
			delete[] new_set;
		}
	}

	new_set = create_new_set(set2, next, set2_size, size - k);
	if(set2_size == (size - k) - 1) {
		size_t* tmp_set = clone_set(set1, set1_size, k);
		prepare_solution(tmp_set, set1_size, new_set, set2_size + 1);
		delete[] tmp_set;
		delete[] new_set;
	}
	else {
		size_t sub_lb = lb_calc(graph, size, k, set1, set1_size, new_set, set2_size + 1);
		if(sub_lb < *ub) {
			Pheet::template spawn<BBTask>(graph, size, k, best, clone_set(set1, set1_size, k), set1_size, new_set, set2_size + 1, ub, sub_lb);
		}
		else {
			delete[] new_set;
		}
	}
}

template <class Pheet, class LowerBound, class NextVertex, size_t MaxSize>
void BranchBoundGraphBipartitioningTask<Pheet, LowerBound, NextVertex, MaxSize>::prepare_solution(size_t* set1, size_t set1_size, size_t* set2, size_t set2_size) {
	size_t* unfinished;
	size_t unfinished_size;
	GraphBipartitioningSolution<MaxSize> sol;
	sol.weight = 0;

	if(set1_size == k) {
		unfinished = set2;
		unfinished_size = set2_size;
	}
	else {
		unfinished = set1;
		unfinished_size = set1_size;
	}
	size_t s1_i = 0;
	size_t s2_i = 0;
	for(size_t i = 0; i < size; ++i) {
		while(s1_i < set1_size && set1[s1_i] < i) {
			++s1_i;
		}
		while(s2_i < set2_size && set2[s2_i] < i) {
			++s2_i;
		}
		if((s1_i == set1_size || set1[s1_i] > i) &&
			(s2_i == set2_size || set2[s2_i] > i)) {
			unfinished[unfinished_size] = i;
			++unfinished_size;
		}
	}
	std::sort(unfinished, unfinished + unfinished_size);
	if(set1_size == k) {
		assert(unfinished_size == size - k);
		set2_size = unfinished_size;
	}
	else {
		assert(unfinished_size == k);
		set1_size = unfinished_size;
	}

	assert(set1_size == k);
	assert(set2_size == size - k);

	for(size_t i = 0; i < set1_size; ++i) {
		size_t node = set1[i];
		size_t i2 = i + 1;
		for(size_t j = 0; j < graph[node].num_edges; ++j) {
			size_t target = graph[node].edges[j].target;
			if(target > node) {
				while(i2 < set1_size && set1[i2] < target) {
					++i2;
				}
				if(i2 == set1_size || set1[i2] > target) {
					sol.weight += graph[node].edges[j].weight;
				}
			}
		}
	}
	for(size_t i = 0; i < set2_size; ++i) {
		size_t node = set2[i];
		size_t i2 = i + 1;
		for(size_t j = 0; j < graph[node].num_edges; ++j) {
			size_t target = graph[node].edges[j].target;
			if(target > node) {
				while(i2 < set2_size && set2[i2] < target) {
					++i2;
				}
				if(i2 == set2_size || set2[i2] > target) {
					sol.weight += graph[node].edges[j].weight;
				}
			}
		}
	}


	while(true) {
		size_t old_ub = *ub;
		if(old_ub > sol.weight) {
			if(SIZET_CAS(ub, old_ub, sol.weight)) {
				for(size_t i = 0; i < set1_size; ++i) {
					sol.sets[0].set(set1[i]);
				}
				for(size_t i = 0; i < set2_size; ++i) {
					sol.sets[1].set(set2[i]);
				}
				best.add_value(sol);
				break;
			}
		}
		else {
			break;
		}
	}
}

template <class Pheet, class LowerBound, class NextVertex, size_t MaxSize>
size_t* BranchBoundGraphBipartitioningTask<Pheet, LowerBound, NextVertex, MaxSize>::create_new_set(size_t* set, size_t new_el, size_t set_size, size_t max_size) {
	size_t* ret = new size_t[max_size];
	size_t i, j;
	for(i = 0, j = 0; i < set_size; ++i, ++j) {
		if(i == j && set[i] > new_el) {
			ret[j] = new_el;
			++j;
		}
		assert(j < max_size);
		ret[j] = set[i];
	}
	if(i == j) {
		assert(j < max_size);
		ret[j] = new_el;
	}
	return ret;
}

template <class Pheet, class LowerBound, class NextVertex, size_t MaxSize>
size_t* BranchBoundGraphBipartitioningTask<Pheet, LowerBound, NextVertex, MaxSize>::clone_set(size_t* set, size_t set_size, size_t max_size) {
	size_t* ret = new size_t[max_size];
	assert(set_size <= max_size);
	for(size_t i = 0; i < set_size; ++i) {
		ret[i] = set[i];
	}
	return ret;
}

}

#endif /* BRANCHBOUNDGRAPHBIPARTITIONINGTASK_H_ */
