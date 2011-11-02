/*
 * test_schedulers.h
 *
 *  Created on: 07.09.2011
 *   Author(s): Martin Wimmer
 *     License: Ask author
 */

#ifndef TEST_SCHEDULERS_H_
#define TEST_SCHEDULERS_H_

#include "../sched/Basic/Synchroneous/SynchroneousScheduler.h"
#include "../sched/Basic/Basic/BasicScheduler.h"
#include "../sched/Basic/Priority/PriorityScheduler.h"
#include "../sched/MixedMode/Basic/BasicMixedModeScheduler.h"

#include "../sched/strategies/LifoFifo/LifoFifoStrategy.h"
#include "../sched/strategies/UserDefinedPriority/UserDefinedPriority.h"


#include "../em/CPUHierarchy/Oversubscribed/OversubscribedSimpleCPUHierarchy.h"

#include "../ds/CircularArray/FixedSize/FixedSizeCircularArray.h"
#include "../ds/CircularArray/TwoLevelGrowing/TwoLevelGrowingCircularArray.h"
#include "../ds/StealingDeque/CircularArray/CircularArrayStealingDeque.h"
#include "../ds/PriorityTaskStorage/Fallback/FallbackTaskStorage.h"
#include "../ds/PriorityTaskStorage/Modular/ModularTaskStorage.h"
#include "../ds/PriorityTaskStorage/Modular/ModularTaskStorage.h"
#include "../ds/PriorityTaskStorage/Modular/Primary/Primitive/PrimitivePrimaryTaskStorage.h"
#include "../ds/PriorityTaskStorage/Modular/Primary/PrimitiveHeap/PrimitiveHeapPrimaryTaskStorage.h"
#include "../ds/PriorityTaskStorage/Modular/Secondary/Primitive/PrimitiveSecondaryTaskStorage.h"

#include "../primitives/Backoff/Exponential/ExponentialBackoff.h"
#include "../primitives/Barrier/Simple/SimpleBarrier.h"

namespace pheet {

//using FixedSizeCircularArrayStealingDeque = CircularArrayStealingDeque<T, FixedSizeCircularArray<T> >;
template <typename T>
class FixedSizeCircularArrayStealingDeque : public CircularArrayStealingDeque<T, FixedSizeCircularArray > {
public:
	FixedSizeCircularArrayStealingDeque(size_t initial_capacity);
	FixedSizeCircularArrayStealingDeque(size_t initial_capacity, BasicPerformanceCounter<stealing_deque_count_steals>& num_stolen, BasicPerformanceCounter<stealing_deque_count_pop_cas>& num_pop_cas);
};

template <typename T>
FixedSizeCircularArrayStealingDeque<T>::FixedSizeCircularArrayStealingDeque(size_t initial_capacity)
: CircularArrayStealingDeque<T, FixedSizeCircularArray >(initial_capacity){

}

template <typename T>
FixedSizeCircularArrayStealingDeque<T>::FixedSizeCircularArrayStealingDeque(size_t initial_capacity, BasicPerformanceCounter<stealing_deque_count_steals>& num_stolen, BasicPerformanceCounter<stealing_deque_count_pop_cas>& num_pop_cas)
: CircularArrayStealingDeque<T, FixedSizeCircularArray >(initial_capacity, num_stolen, num_pop_cas){

}

template <typename T>
class MyTwoLevelGrowingCircularArray : public TwoLevelGrowingCircularArray<T> {
public:
	MyTwoLevelGrowingCircularArray(size_t initial_capacity);
};

template <typename T>
MyTwoLevelGrowingCircularArray<T>::MyTwoLevelGrowingCircularArray(size_t initial_capacity)
: TwoLevelGrowingCircularArray<T>(initial_capacity){

}

template <typename T>
class TwoLevelGrowingCircularArrayStealingDeque : public CircularArrayStealingDeque<T, MyTwoLevelGrowingCircularArray > {
public:
	TwoLevelGrowingCircularArrayStealingDeque(size_t initial_capacity);
	TwoLevelGrowingCircularArrayStealingDeque(size_t initial_capacity, BasicPerformanceCounter<stealing_deque_count_steals>& num_stolen, BasicPerformanceCounter<stealing_deque_count_pop_cas>& num_pop_cas);
};

template <typename T>
TwoLevelGrowingCircularArrayStealingDeque<T>::TwoLevelGrowingCircularArrayStealingDeque(size_t initial_capacity)
: CircularArrayStealingDeque<T, MyTwoLevelGrowingCircularArray >(initial_capacity){

}

template <typename T>
TwoLevelGrowingCircularArrayStealingDeque<T>::TwoLevelGrowingCircularArrayStealingDeque(size_t initial_capacity, BasicPerformanceCounter<stealing_deque_count_steals>& num_stolen, BasicPerformanceCounter<stealing_deque_count_pop_cas>& num_pop_cas)
: CircularArrayStealingDeque<T, MyTwoLevelGrowingCircularArray >(initial_capacity, num_stolen, num_pop_cas){

}

template <typename T>
class FixedSizeCircularArrayStealingDequeFallbackTaskStorage : public FallbackTaskStorage<T, FixedSizeCircularArrayStealingDeque > {
public:
	FixedSizeCircularArrayStealingDequeFallbackTaskStorage(size_t initial_capacity);
	FixedSizeCircularArrayStealingDequeFallbackTaskStorage(size_t initial_capacity, BasicPerformanceCounter<stealing_deque_count_steals>& num_stolen, BasicPerformanceCounter<stealing_deque_count_pop_cas>& num_pop_cas);
};

template <typename T>
FixedSizeCircularArrayStealingDequeFallbackTaskStorage<T>::FixedSizeCircularArrayStealingDequeFallbackTaskStorage(size_t initial_capacity)
: FallbackTaskStorage<T, FixedSizeCircularArrayStealingDeque >(initial_capacity){

}

template <typename T>
FixedSizeCircularArrayStealingDequeFallbackTaskStorage<T>::FixedSizeCircularArrayStealingDequeFallbackTaskStorage(size_t initial_capacity, BasicPerformanceCounter<stealing_deque_count_steals>& num_stolen, BasicPerformanceCounter<stealing_deque_count_pop_cas>& num_pop_cas)
: FallbackTaskStorage<T, FixedSizeCircularArrayStealingDeque >(initial_capacity, num_stolen, num_pop_cas){

}

template <typename T>
class DefaultPrimitivePrimaryTaskStorage : public PrimitivePrimaryTaskStorage<T, MyTwoLevelGrowingCircularArray> {
public:
	template <typename ... ConsParams>
	DefaultPrimitivePrimaryTaskStorage(ConsParams&& ... params)
	: PrimitivePrimaryTaskStorage<T, MyTwoLevelGrowingCircularArray>(static_cast<ConsParams&&>(params) ...) {}
};

template <typename T>
class DefaultPrimitiveHeapPrimaryTaskStorage : public PrimitiveHeapPrimaryTaskStorage<T, MyTwoLevelGrowingCircularArray> {
public:
	template <typename ... ConsParams>
	DefaultPrimitiveHeapPrimaryTaskStorage(ConsParams&& ... params)
	: PrimitiveHeapPrimaryTaskStorage<T, MyTwoLevelGrowingCircularArray>(static_cast<ConsParams&&>(params) ...) {}
};

template <typename T>
class PrimitiveModularTaskStorage : public ModularTaskStorage<T, DefaultPrimitivePrimaryTaskStorage, PrimitiveSecondaryTaskStorage > {
public:
	template <typename ... ConsParams>
	PrimitiveModularTaskStorage(ConsParams&& ... params)
	: ModularTaskStorage<T, DefaultPrimitivePrimaryTaskStorage, PrimitiveSecondaryTaskStorage >(static_cast<ConsParams&&>(params) ...) {}
};

template <typename T>
class PrimitiveHeapModularTaskStorage : public ModularTaskStorage<T, DefaultPrimitiveHeapPrimaryTaskStorage, PrimitiveSecondaryTaskStorage > {
public:
	template <typename ... ConsParams>
	PrimitiveHeapModularTaskStorage(ConsParams&& ... params)
	: ModularTaskStorage<T, DefaultPrimitiveHeapPrimaryTaskStorage, PrimitiveSecondaryTaskStorage >(static_cast<ConsParams&&>(params) ...) {}
};

typedef BasicMixedModeScheduler<OversubscribedSimpleCPUHierarchy, TwoLevelGrowingCircularArrayStealingDeque, SimpleBarrier<StandardExponentialBackoff>, StandardExponentialBackoff>
	DefaultMixedModeScheduler;
typedef BasicScheduler<OversubscribedSimpleCPUHierarchy, FixedSizeCircularArrayStealingDeque, SimpleBarrier<StandardExponentialBackoff>, StandardExponentialBackoff>
	DefaultBasicScheduler;
typedef PriorityScheduler<OversubscribedSimpleCPUHierarchy, FixedSizeCircularArrayStealingDequeFallbackTaskStorage, SimpleBarrier<StandardExponentialBackoff>, StandardExponentialBackoff, LifoFifoStrategy>
	FallbackPriorityScheduler;
typedef PriorityScheduler<OversubscribedSimpleCPUHierarchy, PrimitiveModularTaskStorage, SimpleBarrier<StandardExponentialBackoff>, StandardExponentialBackoff, LifoFifoStrategy>
	PrimitivePriorityScheduler;
typedef PriorityScheduler<OversubscribedSimpleCPUHierarchy, PrimitiveHeapModularTaskStorage, SimpleBarrier<StandardExponentialBackoff>, StandardExponentialBackoff, LifoFifoStrategy>
	PrimitiveHeapPriorityScheduler;
typedef SynchroneousScheduler<OversubscribedSimpleCPUHierarchy>
	DefaultSynchroneousScheduler;

}

#endif /* TEST_SCHEDULERS_H_ */
