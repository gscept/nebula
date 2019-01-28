//------------------------------------------------------------------------------
//  delegates.cc
//  (C) 2019 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "delegates.h"
#include "util/delegate.h"
#include <functional>
#include "util/random.h"

namespace Benchmarking
{
__ImplementClass(Benchmarking::DelegateBench, 'DLGB', Benchmarking::Benchmark);

using namespace Timing;
using namespace Util;

void foo(int& a, int b)
{
	a += b;
}

//------------------------------------------------------------------------------
/**
*/
void
DelegateBench::Run(Timer& timer)
{
	timer.Start();
	const SizeT numCalls = 100000000;
	int a = 0;
	int b = 1;

	n_printf("Testing function calls... \n");

	Time start = timer.GetTime();
	for (SizeT i = 0; i < numCalls; i++)
	{
		foo(a, b);
	}
	Time last = timer.GetTime();
	n_printf("Baseline function call:  %f\n", last - start);

	void(*FuncPtr)(int&, int) = &foo;

	start = timer.GetTime();
	for (SizeT i = 0; i < numCalls; i++)
	{
		FuncPtr(a, b);
	}
	last = timer.GetTime();
	n_printf("Function pointer:        %f\n", last - start);

	Delegate<void(int&, int)> d = Delegate<void(int&, int)>::FromFunction<&foo>();

	start = timer.GetTime();
	for (SizeT i = 0; i < numCalls; i++)
	{
		d(a, b);
	}
	last = timer.GetTime();
	n_printf("Delegate:                %f\n", last - start);

	std::function<void(int&, int)> StdFunc = &foo;

	start = timer.GetTime();
	for (SizeT i = 0; i < numCalls; i++)
	{
		StdFunc(a, b);
	}
	last = timer.GetTime();
	n_printf("std::function:           %f\n", last - start);

	/// --- Lambdas
	n_printf("Testing lambdas... \n");

#define LAMBDA [](int a, int b) -> int { return a + b; }
	
	std::function<void(int&, int)> StdFuncLambda = LAMBDA;
	// StdFuncLambda = LAMBDA;

	start = timer.GetTime();
	for (SizeT i = 0; i < numCalls; i++)
	{
		StdFuncLambda(a, b);
	}
	last = timer.GetTime();
	n_printf("std::function:           %f\n", last - start);

	Delegate<int(int, int)> dLambda = LAMBDA;
	
	start = timer.GetTime();
	for (SizeT i = 0; i < numCalls; i++)
	{
		dLambda(a, b);
	}
	last = timer.GetTime();
	n_printf("Delegate:                %f\n", last - start);


	timer.Stop();
}

} // namespace Benchmarking
