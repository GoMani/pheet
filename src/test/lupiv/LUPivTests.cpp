/*
 * LUPivTests.cpp
 *
 *  Created on: Dec 20, 2011
 *      Author: mwimmer
 *	   License: Ask Author
 */

#include "LUPivTests.h"
#include "Simple/SimpleLUPiv.h"
#include "LocalityStrategy/LocalityStrategyLUPiv.h"

#include "../test_schedulers.h"

#include <iostream>

namespace pheet {

LUPivTests<true>::LUPivTests() {

}

LUPivTests<true>::~LUPivTests() {

}

void LUPivTests<true>::run_test() {
#ifdef LUPIV_TEST
	std::cout << "----" << std::endl;

	this->run_kernel<LocalityStrategyLUPiv<ArrayListHeapMultiStealPriorityScheduler> >();
	this->run_kernel<LocalityStrategyLUPiv<ArrayListHeapPriorityScheduler> >();
	this->run_kernel<SimpleLUPiv<ArrayListHeapMultiStealPriorityScheduler> >();
	this->run_kernel<SimpleLUPiv<DefaultBasicScheduler> >();
#endif
}

}
