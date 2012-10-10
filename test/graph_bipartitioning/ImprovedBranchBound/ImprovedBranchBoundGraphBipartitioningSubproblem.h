/*
 * ImprovedBranchBoundGraphBipartitioningSubproblem.h
 *
 *  Created on: Dec 1, 2011
 *      Author: Martin Wimmer
 *	   License: Boost Software License 1.0 (BSL1.0)
 */

#ifndef IMPROVEDBRANCHBOUNDGRAPHBIPARTITIONINGSUBPROBLEM_H_
#define IMPROVEDBRANCHBOUNDGRAPHBIPARTITIONINGSUBPROBLEM_H_

#include "pheet/pheet.h"
#include "../graph_helpers.h"
#include "pheet/primitives/Reducer/Max/MaxReducer.h"

#include "ImprovedBranchBoundGraphBipartitioningSubproblemPerformanceCounters.h"

#include <bitset>

namespace pheet {

// Needed to resolve circular template dependency problem in gcc
template <class Pheet, size_t MaxSize = 64>
class ImprovedBranchBoundGraphBipartitioningSubproblemBase {
protected:
	ImprovedBranchBoundGraphBipartitioningSubproblemBase(GraphVertex const* graph, size_t size, size_t k)
: graph(graph), size(size), k(k) {}

public:
	typedef std::bitset<MaxSize> Set;

	GraphVertex const* const graph;
//size_t const lw; // largest weight edge (JLT: more dynamic approx. perhaps better)
	size_t const size;
	size_t const k;

	Set sets[3];
// JLT would like
// uint8 assigned[size] = 0/1/2 - where is vertex assigned?
// free[size] = [0,...,size-1]; - list of free vertices
// subsetsize[3]; current size of subsets, subsetsize[2] is number of free vertices
};

template <class Pheet, template <class P, class SP> class LogicT, size_t MaxSize = 64>
class ImprovedBranchBoundGraphBipartitioningSubproblem : public ImprovedBranchBoundGraphBipartitioningSubproblemBase<Pheet, MaxSize> {
public:
	typedef std::bitset<MaxSize> Set;
	typedef ImprovedBranchBoundGraphBipartitioningSubproblemPerformanceCounters<Pheet> PerformanceCounters;
	typedef ImprovedBranchBoundGraphBipartitioningSubproblem<Pheet, LogicT, MaxSize> Self;
	typedef ImprovedBranchBoundGraphBipartitioningSubproblemBase<Pheet, MaxSize> Base;
	typedef GraphBipartitioningSolution<MaxSize> Solution;
	typedef MaxReducer<Pheet, Solution> SolutionReducer;
	typedef LogicT<Pheet, Base> Logic;

	ImprovedBranchBoundGraphBipartitioningSubproblem(GraphVertex const* graph, size_t size, size_t k);
	ImprovedBranchBoundGraphBipartitioningSubproblem(Self const& other);
	~ImprovedBranchBoundGraphBipartitioningSubproblem();

	Self* split(PerformanceCounters& pc);
	bool is_solution();
	void update_solution(size_t* upper_bound, SolutionReducer& best, PerformanceCounters& pc);
	bool can_complete1();
	bool can_complete2();
	void complete_solution1(size_t* upper_bound, SolutionReducer& best, PerformanceCounters& pc);
	void complete_solution2(size_t* upper_bound, SolutionReducer& best, PerformanceCounters& pc);
	size_t get_lower_bound();
	size_t get_estimate();
	size_t get_upper_bound();
	size_t get_lowdeg_lower();
	size_t cc_w(size_t largest_w);

private:
	void update(uint8_t set, size_t pos);

	Logic logic;
};

template <class Pheet, template <class P, class SP> class LogicT, size_t MaxSize>
ImprovedBranchBoundGraphBipartitioningSubproblem<Pheet, LogicT, MaxSize>::ImprovedBranchBoundGraphBipartitioningSubproblem(GraphVertex const* graph, size_t size, size_t k)
: Base(graph, size, k), logic(this) {
	for(size_t i = 0; i < size; ++i) {
		this->sets[2].set(i);
	}
	if(this->k == this->size - this->k) {
		// If both groups have the same size we can already fill in the first vertex
		size_t nv = logic.get_next_vertex();
		update(0, nv);
	}
}

template <class Pheet, template <class P, class SP> class LogicT, size_t MaxSize>
ImprovedBranchBoundGraphBipartitioningSubproblem<Pheet, LogicT, MaxSize>::ImprovedBranchBoundGraphBipartitioningSubproblem(Self const& other)
: Base(other.graph, other.size, other.k), logic(this, other.logic) {
	for(size_t i = 0; i < 3; ++i) {
		this->sets[i] = other.sets[i];
	}
}

template <class Pheet, template <class P, class SP> class LogicT, size_t MaxSize>
ImprovedBranchBoundGraphBipartitioningSubproblem<Pheet, LogicT, MaxSize>::~ImprovedBranchBoundGraphBipartitioningSubproblem() {

}

template <class Pheet, template <class P, class SP> class LogicT, size_t MaxSize>
ImprovedBranchBoundGraphBipartitioningSubproblem<Pheet, LogicT, MaxSize>* ImprovedBranchBoundGraphBipartitioningSubproblem<Pheet, LogicT, MaxSize>::split(PerformanceCounters& pc) {
	pc.num_allocated_subproblems.incr();
	pc.memory_allocation_time.start_timer();
	Self* other = new Self(*this);
	pc.memory_allocation_time.stop_timer();

	size_t nv = logic.get_next_vertex();
	update(0, nv);
	other->update(1, nv);

	return other;
}

template <class Pheet, template <class P, class SP> class LogicT, size_t MaxSize>
void ImprovedBranchBoundGraphBipartitioningSubproblem<Pheet, LogicT, MaxSize>::update(uint8_t set, size_t pos) {
	pheet_assert((set & 1) == set);
	pheet_assert(pos < this->size);
	pheet_assert(!this->sets[set].test(pos));
	pheet_assert(this->sets[2].test(pos));

	this->sets[2].set(pos, false);
	this->sets[set].set(pos);

//	uint8_t other_set = set ^ 1;

	logic.update(set, pos); // JLT - sometimes not good
}

template <class Pheet, template <class P, class SP> class LogicT, size_t MaxSize>
bool ImprovedBranchBoundGraphBipartitioningSubproblem<Pheet, LogicT, MaxSize>::is_solution() {
	return (this->sets[0].count() == this->k) || (this->sets[1].count() == (this->size - this->k));
}

template <class Pheet, template <class P, class SP> class LogicT, size_t MaxSize>
void ImprovedBranchBoundGraphBipartitioningSubproblem<Pheet, LogicT, MaxSize>::update_solution(size_t* upper_bound, MaxReducer<Pheet, GraphBipartitioningSolution<MaxSize> >& best, PerformanceCounters& pc) {
	if(this->sets[0].count() == this->k) {
		this->sets[1] |= this->sets[2];
		Set tmp = this->sets[2];
		this->sets[2].reset();
		logic.bulk_update(1, tmp);
	}
	else {
		pheet_assert(this->sets[1].count() == (this->size - this->k));
		this->sets[0] |= this->sets[2];
		Set tmp = this->sets[2];
		this->sets[2].reset();
		logic.bulk_update(0, tmp);
	}
	size_t cut = logic.get_cut();
	size_t old_ub = *upper_bound;

	while(cut < old_ub) {
		if(SIZET_CAS(upper_bound, old_ub, cut)) {
			pc.last_update_time.take_time();
			pc.num_upper_bound_changes.incr();

			pc.events.add(cut);

			GraphBipartitioningSolution<MaxSize> my_best;
			my_best.weight = cut;
			my_best.sets[0] = this->sets[0];
			my_best.sets[1] = this->sets[1];
			best.add_value(my_best);
		}
		else {
			old_ub = *upper_bound;
		}
	}
}

template <class Pheet, template <class P, class SP> class LogicT, size_t MaxSize>
bool ImprovedBranchBoundGraphBipartitioningSubproblem<Pheet, LogicT, MaxSize>::can_complete1() {
  return ((this->sets[0].count() == this->k-1) || 
	  (this->sets[1].count() == (this->size - this->k)-1));
  //return 0;
}

template <class Pheet, template <class P, class SP> class LogicT, size_t MaxSize>
void ImprovedBranchBoundGraphBipartitioningSubproblem<Pheet, LogicT, MaxSize>::complete_solution1(size_t* upper_bound, MaxReducer<Pheet, GraphBipartitioningSolution<MaxSize> >& best, PerformanceCounters& pc) {
  size_t s = (this->sets[0].count() == this->k-1) ? 0 : 1;

  //std::cout<<'#';

  size_t v = logic.get_minnode(s);

  update(s,v);

  this->sets[1-s] |= this->sets[2];
  Set tmp = this->sets[2];
  this->sets[2].reset();
  logic.bulk_update(1-s, tmp);

	size_t cut = logic.get_cut();
	size_t old_ub = *upper_bound;

	while(cut < old_ub) {
		if(SIZET_CAS(upper_bound, old_ub, cut)) {
			pc.last_update_time.take_time();
			pc.num_upper_bound_changes.incr();

			pc.events.add(cut);

			GraphBipartitioningSolution<MaxSize> my_best;
			my_best.weight = cut;
			my_best.sets[0] = this->sets[0];
			my_best.sets[1] = this->sets[1];
			best.add_value(my_best);
		}
		else {
			old_ub = *upper_bound;
		}
	}
}

template <class Pheet, template <class P, class SP> class LogicT, size_t MaxSize>
bool ImprovedBranchBoundGraphBipartitioningSubproblem<Pheet, LogicT, MaxSize>::can_complete2() {
  return logic.no_edges();
}

template <class Pheet, template <class P, class SP> class LogicT, size_t MaxSize>
void ImprovedBranchBoundGraphBipartitioningSubproblem<Pheet, LogicT, MaxSize>::complete_solution2(size_t* upper_bound, MaxReducer<Pheet, GraphBipartitioningSolution<MaxSize> >& best, PerformanceCounters& pc) {
  logic.assign_deltabound();

	size_t cut = logic.get_cut();
	size_t old_ub = *upper_bound;

	while(cut < old_ub) {
		if(SIZET_CAS(upper_bound, old_ub, cut)) {
			pc.last_update_time.take_time();
			pc.num_upper_bound_changes.incr();

			pc.events.add(cut);

			GraphBipartitioningSolution<MaxSize> my_best;
			my_best.weight = cut;
			my_best.sets[0] = this->sets[0];
			my_best.sets[1] = this->sets[1];
			best.add_value(my_best);
		}
		else {
			old_ub = *upper_bound;
		}
	}
}

template <class Pheet, template <class P, class SP> class LogicT, size_t MaxSize>
size_t ImprovedBranchBoundGraphBipartitioningSubproblem<Pheet, LogicT, MaxSize>::get_lower_bound() {
	return logic.get_lower_bound();
}

template <class Pheet, template <class P, class SP> class LogicT, size_t MaxSize>
size_t ImprovedBranchBoundGraphBipartitioningSubproblem<Pheet, LogicT, MaxSize>::get_lowdeg_lower() {
	return logic.get_lowdeg_lower();
}

template <class Pheet, template <class P, class SP> class LogicT, size_t MaxSize>
size_t ImprovedBranchBoundGraphBipartitioningSubproblem<Pheet, LogicT, MaxSize>::cc_w(size_t largest_w) {
	return logic.cc_w(largest_w);
}

template <class Pheet, template <class P, class SP> class LogicT, size_t MaxSize>
size_t ImprovedBranchBoundGraphBipartitioningSubproblem<Pheet, LogicT, MaxSize>::get_estimate() {
	return logic.get_estimate();
}

template <class Pheet, template <class P, class SP> class LogicT, size_t MaxSize>
size_t ImprovedBranchBoundGraphBipartitioningSubproblem<Pheet, LogicT, MaxSize>::get_upper_bound() {
	return logic.get_upper_bound();
}

}

#endif /* IMPROVEDBRANCHBOUNDGRAPHBIPARTITIONINGSUBPROBLEM_H_ */
