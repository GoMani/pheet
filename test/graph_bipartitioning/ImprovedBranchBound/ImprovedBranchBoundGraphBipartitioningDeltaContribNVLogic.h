/*
 * ImprovedBranchBoundGraphBipartitioningDeltaContribNVLogic.h
 *
 *  Created on: Jan 13, 2012
 *      Author: Martin Wimmer
 *	   License: Boost Software License 1.0 (BSL1.0)
 */

#ifndef IMPROVEDBRANCHBOUNDGRAPHBIPARTITIONINGDELTACONTRIBNVLOGIC_H_
#define IMPROVEDBRANCHBOUNDGRAPHBIPARTITIONINGDELTACONTRIBNVLOGIC_H_

#include "pheet/pheet.h"
#include "../graph_helpers.h"

#include <string.h>
#include <algorithm>

namespace pheet {

template <class Pheet, class SubProblem>
class ImprovedBranchBoundGraphBipartitioningDeltaContribNVLogic {
public:
	typedef ImprovedBranchBoundGraphBipartitioningDeltaContribNVLogic<Pheet, SubProblem> Self;
	typedef typename SubProblem::Set Set;

	ImprovedBranchBoundGraphBipartitioningDeltaContribNVLogic(SubProblem* sub_problem);
	ImprovedBranchBoundGraphBipartitioningDeltaContribNVLogic(SubProblem* sub_problem, Self const& other);
	~ImprovedBranchBoundGraphBipartitioningDeltaContribNVLogic();

	void init_root();
	size_t get_next_vertex();
	size_t get_cut();
	size_t get_lower_bound();
	size_t get_estimate();
	size_t get_upper_bound();
	void update(uint8_t set, size_t pos);
	void bulk_update(uint8_t set, Set positions);

	size_t get_minnode(uint8_t set);
//	size_t get_lowdeg_lower();
//	size_t cc_w(size_t largest_w);

//	bool no_edges();
//	void assign_deltabound();

	bool can_complete();
	void complete_solution();

	static void print_name();
private:
	SubProblem* sub_problem;

	size_t cut;
	size_t lb;
	size_t nv;
	size_t ub;
	size_t contrib_sum;

	size_t* weights[2];
	size_t* contributions;
};

template <class Pheet, class SubProblem>
ImprovedBranchBoundGraphBipartitioningDeltaContribNVLogic<Pheet, SubProblem>::ImprovedBranchBoundGraphBipartitioningDeltaContribNVLogic(SubProblem* sub_problem)
: sub_problem(sub_problem), cut(0), lb(0), nv(0), ub(0), contrib_sum(0) {
	weights[0] = new size_t[sub_problem->size];
	weights[1] = new size_t[sub_problem->size];
	memset(weights[0], 0, sizeof(size_t)*sub_problem->size);
	memset(weights[1], 0, sizeof(size_t)*sub_problem->size);
	contributions = new size_t[sub_problem->size];

	size_t best_contrib = 0;
	size_t best_contrib_i = 0;
	for(size_t i = 0; i < sub_problem->size; ++i) {
		contributions[i] = 0;
		for(size_t j = 0; j < sub_problem->graph[i].num_edges; ++j) {
			contributions[i] += sub_problem->graph[i].edges[j].weight;
		}
		contrib_sum += contributions[i];
		if(contributions[i] > best_contrib) {
			best_contrib = contributions[i];
			best_contrib_i = i;
		}
	}
	nv = best_contrib_i;
	contrib_sum >>= 1;
}

template <class Pheet, class SubProblem>
ImprovedBranchBoundGraphBipartitioningDeltaContribNVLogic<Pheet, SubProblem>::ImprovedBranchBoundGraphBipartitioningDeltaContribNVLogic(SubProblem* sub_problem, Self const& other)
: sub_problem(sub_problem), cut(other.cut), lb(other.lb), nv(other.nv), ub(other.ub), contrib_sum(other.contrib_sum) {
	weights[0] = new size_t[sub_problem->size];
	weights[1] = new size_t[sub_problem->size];
	memcpy(weights[0], other.weights[0], sizeof(size_t)*sub_problem->size);
	memcpy(weights[1], other.weights[1], sizeof(size_t)*sub_problem->size);
	contributions = new size_t[sub_problem->size];
	memcpy(contributions, other.contributions, sizeof(size_t)*sub_problem->size);
}

template <class Pheet, class SubProblem>
ImprovedBranchBoundGraphBipartitioningDeltaContribNVLogic<Pheet, SubProblem>::~ImprovedBranchBoundGraphBipartitioningDeltaContribNVLogic() {
	delete[] weights[0];
	delete[] weights[1];
	delete[] contributions;
}

template <class Pheet, class SubProblem>
void ImprovedBranchBoundGraphBipartitioningDeltaContribNVLogic<Pheet, SubProblem>::init_root() {

}

template <class Pheet, class SubProblem>
size_t ImprovedBranchBoundGraphBipartitioningDeltaContribNVLogic<Pheet, SubProblem>::get_next_vertex() {
	pheet_assert(sub_problem->sets[2].test(nv));
	return nv;
}

template <class Pheet, class SubProblem>
size_t ImprovedBranchBoundGraphBipartitioningDeltaContribNVLogic<Pheet, SubProblem>::get_cut() {
	return cut;
}

template <class Pheet, class SubProblem>
size_t ImprovedBranchBoundGraphBipartitioningDeltaContribNVLogic<Pheet, SubProblem>::get_lower_bound() {
	return get_cut() + lb;
}
/*
template <class Pheet, class SubProblem>
size_t ImprovedBranchBoundGraphBipartitioningDeltaContribNVLogic<Pheet, SubProblem>::get_lowdeg_lower() {
  return 0;
}*/

template <class Pheet, class SubProblem>
size_t ImprovedBranchBoundGraphBipartitioningDeltaContribNVLogic<Pheet, SubProblem>::get_estimate() {
//	return get_cut() + ((lb + ub + contrib_sum) >> 1);
	return get_cut() + lb + contrib_sum;
}

template <class Pheet, class SubProblem>
size_t ImprovedBranchBoundGraphBipartitioningDeltaContribNVLogic<Pheet, SubProblem>::get_upper_bound() {
	return get_cut() + ub + contrib_sum;
}

template <class Pheet, class SubProblem>
size_t ImprovedBranchBoundGraphBipartitioningDeltaContribNVLogic<Pheet, SubProblem>::get_minnode(uint8_t set) {
  // only one free

  size_t fweight;

  size_t w = sub_problem->sets[2]._Find_first();
  fweight = 0;
  for (size_t i=0; i<sub_problem->graph[w].num_edges; ++i) {
    if(sub_problem->sets[2].test(sub_problem->graph[w].edges[i].target)) {
      fweight += sub_problem->graph[w].edges[i].weight;
    }
  }
  
  size_t mincut, sumcut = 0;
  size_t v = sub_problem->sets[2]._Find_next(w);
  while(v != sub_problem->sets[2].size()) {
    sumcut += weights[set][v];
    v = sub_problem->sets[2]._Find_next(v);
  }
		
  mincut = sumcut+weights[set^1][w]+fweight;

  v = sub_problem->sets[2]._Find_first();
  while(v != sub_problem->sets[2].size()) {
    sumcut += weights[set][v];
    v = sub_problem->sets[2]._Find_next(v);
    if (v!=sub_problem->sets[2].size()) {
      fweight = 0;
      for (size_t i=0; i<sub_problem->graph[v].num_edges; ++i) {
	if(sub_problem->sets[2].test(sub_problem->graph[v].edges[i].target)) {
	  fweight += sub_problem->graph[v].edges[i].weight;
	}
      }
      sumcut -= weights[set][v];
      if (sumcut+weights[set^1][v]+fweight<mincut) {
	mincut = sumcut+weights[set^1][v]+fweight;
	w = v;
      }
    }
  }

  return w;
}
/*
template <class Pheet, class SubProblem>
size_t ImprovedBranchBoundGraphBipartitioningDeltaContribNVLogic<Pheet, SubProblem>::cc_w(size_t largest_w) {
  return 0;
}
*/
/*
template <class Pheet, class SubProblem>
bool ImprovedBranchBoundGraphBipartitioningDeltaContribNVLogic<Pheet, SubProblem>::no_edges() {
  return 0;
 }
*/
template <class Pheet, class SubProblem>
void ImprovedBranchBoundGraphBipartitioningDeltaContribNVLogic<Pheet, SubProblem>::update(uint8_t set, size_t pos) {
	pheet_assert((set & 1) == set);
	pheet_assert(pos < sub_problem->size);
	pheet_assert(!sub_problem->sets[set].test(pos));
	pheet_assert(sub_problem->sets[2].test(pos));

	sub_problem->sets[2].set(pos, false);
	sub_problem->sets[set].set(pos);

	cut += weights[set ^ 1][pos];

	for(size_t i = 0; i < sub_problem->graph[pos].num_edges; ++i) {
		weights[set][sub_problem->graph[pos].edges[i].target] += sub_problem->graph[pos].edges[i].weight;
		if(sub_problem->sets[2].test(sub_problem->graph[pos].edges[i].target)) {
			pheet_assert(contributions[sub_problem->graph[pos].edges[i].target] >= sub_problem->graph[pos].edges[i].weight);
			contributions[sub_problem->graph[pos].edges[i].target] -= sub_problem->graph[pos].edges[i].weight;
			contrib_sum -= sub_problem->graph[pos].edges[i].weight;
		}
	}

	ptrdiff_t* delta = new ptrdiff_t[sub_problem->sets[2].count()];

	size_t di = 0;
	lb = 0;
	ub = 0;
	size_t current_bit = sub_problem->sets[2]._Find_first();
	nv = current_bit;
	size_t best_contrib = 0;
	while(current_bit != sub_problem->sets[2].size()) {
		ptrdiff_t d = (ptrdiff_t)weights[1][current_bit] - (ptrdiff_t)weights[0][current_bit];
		if(std::abs(d) + contributions[current_bit] >= best_contrib) {
			best_contrib = std::abs(d) + contributions[current_bit];
			nv = current_bit;
		}
		delta[di++] = d;
		lb += std::min(weights[0][current_bit], weights[1][current_bit]);
		ub += std::max(weights[0][current_bit], weights[1][current_bit]);
		current_bit = sub_problem->sets[2]._Find_next(current_bit);
	}
	std::sort(delta, delta + di);

	size_t pivot = (/*sub_problem->size - */sub_problem->k) - sub_problem->sets[0].count();
	if(pivot < di && delta[pivot] < 0) {
		do {
			lb += -delta[pivot];
			ub -= -delta[pivot];
			++pivot;
		} while(pivot < di && delta[pivot] < 0);
	}
	else if(pivot > 0 && delta[pivot - 1] > 0) {
		do {
			--pivot;
			lb += delta[pivot];
			ub -= delta[pivot];
		} while(pivot > 0 && delta[pivot - 1] > 0);
	}

	delete[] delta;
}

template <class Pheet, class SubProblem>
void ImprovedBranchBoundGraphBipartitioningDeltaContribNVLogic<Pheet, SubProblem>::bulk_update(uint8_t set, Set positions) {
	size_t current_bit = positions._Find_first();
	while(current_bit != positions.size()) {
		update(set, current_bit);
		current_bit = positions._Find_next(current_bit);
	}
}

template <class Pheet, class SubProblem>
bool ImprovedBranchBoundGraphBipartitioningDeltaContribNVLogic<Pheet, SubProblem>::can_complete() {
	return ((sub_problem->sets[0].count() == sub_problem->k-1) ||
			(sub_problem->sets[1].count() == (sub_problem->size - sub_problem->k)-1));
}

template <class Pheet, class SubProblem>
void ImprovedBranchBoundGraphBipartitioningDeltaContribNVLogic<Pheet, SubProblem>::complete_solution() {
	size_t s;
	if(sub_problem->sets[0].count() == sub_problem->k-1) {
		s = 0;
	}
	else if(sub_problem->sets[1].count() == (sub_problem->size - sub_problem->k)-1) {
		s = 1;
	}
	else {
		pheet_assert(true);
	}

	//std::cout<<'#';

	size_t v = get_minnode(s);

	update(s,v);

	sub_problem->sets[1-s] |= sub_problem->sets[2];
	Set tmp = sub_problem->sets[2];
	sub_problem->sets[2].reset();
	bulk_update(1-s, tmp);
}

template <class Pheet, class SubProblem>
void ImprovedBranchBoundGraphBipartitioningDeltaContribNVLogic<Pheet, SubProblem>::print_name() {
	std::cout << "DeltaContribNVLogic";
}

}

#endif /* IMPROVEDBRANCHBOUNDGRAPHBIPARTITIONINGDELTACONTRIBNVLOGIC_H_ */
