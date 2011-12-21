/*
 * GraphBipartitioningTests.cpp
 *
 *  Created on: 07.09.2011
 *   Author(s): Martin Wimmer
 *     License: Ask author
 */

#include "GraphBipartitioningTests.h"
#include "BranchBound/BranchBoundGraphBipartitioning.h"
#include "BranchBound/BasicLowerBound.h"
#include "BranchBound/BasicNextVertex.h"
#include "StrategyBranchBound/StrategyBranchBoundGraphBipartitioning.h"
#include "StrategyBranchBound/BranchBoundGraphBipartitioningAutoStrategy.h"
#include "StrategyBranchBound/BranchBoundGraphBipartitioningBestFirstStrategy.h"
#include "ImprovedBranchBound/ImprovedBranchBoundGraphBipartitioning.h"
#include "ImprovedBranchBound/ImprovedBranchBoundGraphBipartitioningBasicLogic.h"
#include "ImprovedBranchBound/ImprovedBranchBoundGraphBipartitioningImprovedLogic.h"
#include "ImprovedBranchBound/ImprovedBranchBoundGraphBipartitioningDeltaLogic.h"
#include "ImprovedBranchBound/ImprovedBranchBoundGraphBipartitioningDeltaNVLogic.h"
#include "ImprovedStrategyBranchBound/ImprovedStrategyBranchBoundGraphBipartitioning.h"
#include "ImprovedStrategyBranchBound/ImprovedStrategyBranchBoundGraphBipartitioningBestFirstStrategy.h"
#include "ImprovedStrategyBranchBound/ImprovedStrategyBranchBoundGraphBipartitioningDepthFirstStrategy.h"
#include "ImprovedStrategyBranchBound/ImprovedStrategyBranchBoundGraphBipartitioningLowerBoundStrategy.h"
#include "ImprovedStrategyBranchBound/ImprovedStrategyBranchBoundGraphBipartitioningAutoStrategy.h"
#include "../../sched/strategies/Fifo/FifoStrategy.h"
#include "../../sched/strategies/Lifo/LifoStrategy.h"
#include "../../sched/strategies/LifoFifo/LifoFifoStrategy.h"

#include "../test_schedulers.h"

namespace pheet {


template <typename T>
class AutoLifoStrategy : public ImprovedStrategyBranchBoundGraphBipartitioningAutoStrategy<LifoStrategy, T> {
public:
	template <typename ... ConsParams>
	AutoLifoStrategy(ConsParams&& ... params)
	: ImprovedStrategyBranchBoundGraphBipartitioningAutoStrategy<LifoStrategy, T >(static_cast<ConsParams&&>(params) ...) {}
};

template <typename T>
class AutoFifoStrategy : public ImprovedStrategyBranchBoundGraphBipartitioningAutoStrategy<FifoStrategy, T> {
public:
	template <typename ... ConsParams>
	AutoFifoStrategy(ConsParams&& ... params)
	: ImprovedStrategyBranchBoundGraphBipartitioningAutoStrategy<FifoStrategy, T >(static_cast<ConsParams&&>(params) ...) {}
};

template <typename T>
class AutoLifoFifoStrategy : public ImprovedStrategyBranchBoundGraphBipartitioningAutoStrategy<LifoFifoStrategy, T> {
public:
	template <typename ... ConsParams>
	AutoLifoFifoStrategy(ConsParams&& ... params)
	: ImprovedStrategyBranchBoundGraphBipartitioningAutoStrategy<LifoFifoStrategy, T >(static_cast<ConsParams&&>(params) ...) {}
};

GraphBipartitioningTests::GraphBipartitioningTests() {

}

GraphBipartitioningTests::~GraphBipartitioningTests() {

}

void GraphBipartitioningTests::run_test() {
	if(graph_bipartitioning_test) {
		std::cout << "----" << std::endl;

		this->run_partitioner<ImprovedStrategyBranchBoundGraphBipartitioning<ArrayListPriorityScheduler, ImprovedBranchBoundGraphBipartitioningDeltaNVLogic<ArrayListPriorityScheduler, 64>, ImprovedStrategyBranchBoundGraphBipartitioningDepthFirstStrategy, 64 > >();
		this->run_partitioner<ImprovedStrategyBranchBoundGraphBipartitioning<ArrayListPriorityScheduler, ImprovedBranchBoundGraphBipartitioningDeltaNVLogic<ArrayListPriorityScheduler, 64>, ImprovedStrategyBranchBoundGraphBipartitioningLowerBoundStrategy, 64 > >();
		this->run_partitioner<ImprovedStrategyBranchBoundGraphBipartitioning<ArrayListPriorityScheduler, ImprovedBranchBoundGraphBipartitioningDeltaNVLogic<ArrayListPriorityScheduler, 64>, ImprovedStrategyBranchBoundGraphBipartitioningBestFirstStrategy, 64 > >();

		this->run_partitioner<ImprovedStrategyBranchBoundGraphBipartitioning<ArrayListHeapPriorityScheduler, ImprovedBranchBoundGraphBipartitioningDeltaNVLogic<ArrayListHeapPriorityScheduler, 64>, ImprovedStrategyBranchBoundGraphBipartitioningDepthFirstStrategy, 64 > >();
		this->run_partitioner<ImprovedStrategyBranchBoundGraphBipartitioning<ArrayListHeapPriorityScheduler, ImprovedBranchBoundGraphBipartitioningDeltaNVLogic<ArrayListHeapPriorityScheduler, 64>, ImprovedStrategyBranchBoundGraphBipartitioningLowerBoundStrategy, 64 > >();
		this->run_partitioner<ImprovedStrategyBranchBoundGraphBipartitioning<ArrayListHeapPriorityScheduler, ImprovedBranchBoundGraphBipartitioningDeltaNVLogic<ArrayListHeapPriorityScheduler, 64>, ImprovedStrategyBranchBoundGraphBipartitioningBestFirstStrategy, 64 > >();
/*		this->run_partitioner<ImprovedStrategyBranchBoundGraphBipartitioning<ArrayListPriorityScheduler, ImprovedBranchBoundGraphBipartitioningDeltaLogic<ArrayListPriorityScheduler, 64>, ImprovedStrategyBranchBoundGraphBipartitioningBestFirstStrategy, 64 > >();
		this->run_partitioner<ImprovedStrategyBranchBoundGraphBipartitioning<ArrayListPriorityScheduler, ImprovedBranchBoundGraphBipartitioningImprovedLogic<ArrayListPriorityScheduler, 64>, ImprovedStrategyBranchBoundGraphBipartitioningBestFirstStrategy, 64 > >();
		this->run_partitioner<ImprovedBranchBoundGraphBipartitioning<DefaultBasicScheduler, ImprovedBranchBoundGraphBipartitioningDeltaNVLogic<DefaultBasicScheduler, 64>, 64 > >();
		this->run_partitioner<ImprovedBranchBoundGraphBipartitioning<DefaultBasicScheduler, ImprovedBranchBoundGraphBipartitioningDeltaLogic<DefaultBasicScheduler, 64>, 64 > >();
		this->run_partitioner<ImprovedBranchBoundGraphBipartitioning<DefaultBasicScheduler, ImprovedBranchBoundGraphBipartitioningImprovedLogic<DefaultBasicScheduler, 64>, 64 > >();
	/*	this->run_partitioner<ImprovedBranchBoundGraphBipartitioning<ArrayListPriorityScheduler, ImprovedBranchBoundGraphBipartitioningImprovedLogic<ArrayListPriorityScheduler, 64>, 64 > >();
		this->run_partitioner<ImprovedStrategyBranchBoundGraphBipartitioning<ArrayListPriorityScheduler, ImprovedBranchBoundGraphBipartitioningImprovedLogic<ArrayListPriorityScheduler, 64>, AutoFifoStrategy, 64 > >();
		this->run_partitioner<ImprovedStrategyBranchBoundGraphBipartitioning<ArrayListPriorityScheduler, ImprovedBranchBoundGraphBipartitioningImprovedLogic<ArrayListPriorityScheduler, 64>, ImprovedStrategyBranchBoundGraphBipartitioningBestFirstStrategy, 64 > >();
		this->run_partitioner<ImprovedBranchBoundGraphBipartitioning<ArrayListHeapPriorityScheduler, ImprovedBranchBoundGraphBipartitioningImprovedLogic<ArrayListHeapPriorityScheduler, 64>, 64 > >();
		this->run_partitioner<ImprovedStrategyBranchBoundGraphBipartitioning<ArrayListHeapPriorityScheduler, ImprovedBranchBoundGraphBipartitioningImprovedLogic<ArrayListHeapPriorityScheduler, 64>, AutoFifoStrategy, 64 > >();
		this->run_partitioner<ImprovedStrategyBranchBoundGraphBipartitioning<ArrayListHeapPriorityScheduler, ImprovedBranchBoundGraphBipartitioningImprovedLogic<ArrayListHeapPriorityScheduler, 64>, ImprovedStrategyBranchBoundGraphBipartitioningBestFirstStrategy, 64 > >();
		this->run_partitioner<ImprovedBranchBoundGraphBipartitioning<PrimitiveHeapPriorityScheduler, ImprovedBranchBoundGraphBipartitioningImprovedLogic<PrimitiveHeapPriorityScheduler, 64>, 64 > >();
		this->run_partitioner<ImprovedStrategyBranchBoundGraphBipartitioning<PrimitiveHeapPriorityScheduler, ImprovedBranchBoundGraphBipartitioningImprovedLogic<PrimitiveHeapPriorityScheduler, 64>, AutoFifoStrategy, 64 > >();
		this->run_partitioner<ImprovedStrategyBranchBoundGraphBipartitioning<PrimitiveHeapPriorityScheduler, ImprovedBranchBoundGraphBipartitioningImprovedLogic<PrimitiveHeapPriorityScheduler, 64>, ImprovedStrategyBranchBoundGraphBipartitioningBestFirstStrategy, 64 > >();
		this->run_partitioner<ImprovedBranchBoundGraphBipartitioning<PrimitiveHeapPriorityScheduler, ImprovedBranchBoundGraphBipartitioningImprovedLogic<PrimitiveHeapPriorityScheduler, 64>, 64 > >();
		this->run_partitioner<ImprovedStrategyBranchBoundGraphBipartitioning<PrimitiveHeapPriorityScheduler, ImprovedBranchBoundGraphBipartitioningImprovedLogic<PrimitiveHeapPriorityScheduler, 64>, AutoFifoStrategy, 64 > >();
		this->run_partitioner<ImprovedStrategyBranchBoundGraphBipartitioning<PrimitiveHeapPriorityScheduler, ImprovedBranchBoundGraphBipartitioningImprovedLogic<PrimitiveHeapPriorityScheduler, 64>, ImprovedStrategyBranchBoundGraphBipartitioningBestFirstStrategy, 64 > >();
*/

		// All to slow compared to newer implementations
/*		this->run_partitioner<ImprovedBranchBoundGraphBipartitioning<ArrayListPriorityScheduler, ImprovedBranchBoundGraphBipartitioningBasicLogic<64>, 64 > >();
		this->run_partitioner<ImprovedStrategyBranchBoundGraphBipartitioning<ArrayListPriorityScheduler, ImprovedBranchBoundGraphBipartitioningBasicLogic<64>, AutoFifoStrategy, 64 > >();
		this->run_partitioner<ImprovedStrategyBranchBoundGraphBipartitioning<ArrayListPriorityScheduler, ImprovedBranchBoundGraphBipartitioningBasicLogic<64>, AutoLifoStrategy, 64 > >();
		this->run_partitioner<ImprovedStrategyBranchBoundGraphBipartitioning<ArrayListPriorityScheduler, ImprovedBranchBoundGraphBipartitioningBasicLogic<64>, AutoLifoFifoStrategy, 64 > >();
		this->run_partitioner<ImprovedStrategyBranchBoundGraphBipartitioning<ArrayListPriorityScheduler, ImprovedBranchBoundGraphBipartitioningBasicLogic<64>, ImprovedStrategyBranchBoundGraphBipartitioningBestFirstStrategy, 64 > >();

		this->run_partitioner<StrategyBranchBoundGraphBipartitioning<ArrayListPriorityScheduler, BasicLowerBound, BasicNextVertex, BranchBoundGraphBipartitioningBestFirstStrategy > >();
		this->run_partitioner<StrategyBranchBoundGraphBipartitioning<ArrayListPrioritySchedulerShortQueues, BasicLowerBound, BasicNextVertex, BranchBoundGraphBipartitioningBestFirstStrategy > >();
		this->run_partitioner<StrategyBranchBoundGraphBipartitioning<ArrayListPrioritySchedulerLongQueues, BasicLowerBound, BasicNextVertex, BranchBoundGraphBipartitioningBestFirstStrategy > >();

		this->run_partitioner<StrategyBranchBoundGraphBipartitioning<PrimitivePriorityScheduler, BasicLowerBound, BasicNextVertex, BranchBoundGraphBipartitioningBestFirstStrategy > >();
		this->run_partitioner<StrategyBranchBoundGraphBipartitioning<PrimitivePrioritySchedulerShortQueues, BasicLowerBound, BasicNextVertex, BranchBoundGraphBipartitioningBestFirstStrategy > >();
		this->run_partitioner<StrategyBranchBoundGraphBipartitioning<PrimitivePrioritySchedulerLongQueues, BasicLowerBound, BasicNextVertex, BranchBoundGraphBipartitioningBestFirstStrategy > >();
		// Too slow...
		// this->run_partitioner<StrategyBranchBoundGraphBipartitioning<PrimitivePrioritySchedulerVeryLongQueues, BasicLowerBound, BasicNextVertex, BranchBoundGraphBipartitioningBestFirstStrategy > >();
		this->run_partitioner<StrategyBranchBoundGraphBipartitioning<PrimitiveHeapPriorityScheduler, BasicLowerBound, BasicNextVertex, BranchBoundGraphBipartitioningBestFirstStrategy > >();
		this->run_partitioner<StrategyBranchBoundGraphBipartitioning<PrimitiveHeapPrioritySchedulerShortQueues, BasicLowerBound, BasicNextVertex, BranchBoundGraphBipartitioningBestFirstStrategy > >();
		this->run_partitioner<StrategyBranchBoundGraphBipartitioning<PrimitiveHeapPrioritySchedulerLongQueues, BasicLowerBound, BasicNextVertex, BranchBoundGraphBipartitioningBestFirstStrategy > >();
		this->run_partitioner<StrategyBranchBoundGraphBipartitioning<PrimitiveHeapPrioritySchedulerVeryLongQueues, BasicLowerBound, BasicNextVertex, BranchBoundGraphBipartitioningBestFirstStrategy > >();

		this->run_partitioner<BranchBoundGraphBipartitioning<PrimitivePriorityScheduler, BasicLowerBound, BasicNextVertex> >();
		this->run_partitioner<BranchBoundGraphBipartitioning<PrimitiveHeapPriorityScheduler, BasicLowerBound, BasicNextVertex> >();
		this->run_partitioner<BranchBoundGraphBipartitioning<FallbackPriorityScheduler, BasicLowerBound, BasicNextVertex> >();
		this->run_partitioner<BranchBoundGraphBipartitioning<DefaultMixedModeScheduler, BasicLowerBound, BasicNextVertex> >();
		this->run_partitioner<BranchBoundGraphBipartitioning<DefaultBasicScheduler, BasicLowerBound, BasicNextVertex> >();
		this->run_partitioner<BranchBoundGraphBipartitioning<DefaultSynchroneousScheduler, BasicLowerBound, BasicNextVertex> >();

		this->run_partitioner<StrategyBranchBoundGraphBipartitioning<PrimitivePriorityScheduler, BasicLowerBound, BasicNextVertex, BranchBoundGraphBipartitioningAutoStrategy<FifoStrategy> > >();
		this->run_partitioner<StrategyBranchBoundGraphBipartitioning<PrimitiveHeapPriorityScheduler, BasicLowerBound, BasicNextVertex, BranchBoundGraphBipartitioningAutoStrategy<FifoStrategy> > >();
		this->run_partitioner<StrategyBranchBoundGraphBipartitioning<PrimitivePheetHeapPriorityScheduler, BasicLowerBound, BasicNextVertex, BranchBoundGraphBipartitioningAutoStrategy<FifoStrategy> > >();
		this->run_partitioner<StrategyBranchBoundGraphBipartitioning<PrimitiveSortedArrayHeapPriorityScheduler, BasicLowerBound, BasicNextVertex, BranchBoundGraphBipartitioningAutoStrategy<FifoStrategy> > >();
		this->run_partitioner<StrategyBranchBoundGraphBipartitioning<ArrayListPriorityScheduler, BasicLowerBound, BasicNextVertex, BranchBoundGraphBipartitioningAutoStrategy<FifoStrategy> > >();
		this->run_partitioner<StrategyBranchBoundGraphBipartitioning<PrimitiveHeapPriorityScheduler, BasicLowerBound, BasicNextVertex, BranchBoundGraphBipartitioningAutoStrategy<LifoStrategy> > >();
		this->run_partitioner<StrategyBranchBoundGraphBipartitioning<PrimitiveHeapPriorityScheduler, BasicLowerBound, BasicNextVertex, BranchBoundGraphBipartitioningAutoStrategy<LifoFifoStrategy> > >();
		this->run_partitioner<StrategyBranchBoundGraphBipartitioning<ArrayListPriorityScheduler, BasicLowerBound, BasicNextVertex, BranchBoundGraphBipartitioningAutoStrategy<LifoFifoStrategy> > >();
		*/
	}
}

}
