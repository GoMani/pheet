/*
 * Copyright Jakob Gruber, Martin Kalany 2013.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef __QUEUE_H
#define __QUEUE_H

#include <vector>

#include "test/msp/lib/ShortestPath/Path.h"

namespace pareto
{

/**
 * A high level interface for the required pareto queue operations
 * first() and insert().
 */
class Queue
{
public:
	/**
	 * Returns the first n elements and removes them from the queue.  If
	 * queue.size() = n' < n, n' elements are returned and the queue is
	 * cleared.
	 */
	virtual std::vector<sp::PathPtr> first(size_t const n) = 0;

	/**
	 * Inserts path into the queue.
	 *
	 * Let ps be the set of all paths p' with p'.head() == path.head()
	 * (including path itself).
	 *
	 * insert() additionally removes all nodes <- ps which are dominated by
	 * any other node <- ps.
	 */
	virtual void insert(sp::PathPtr path) = 0;

	virtual bool empty() const = 0;
};

}

#endif /* __QUEUE_H */
