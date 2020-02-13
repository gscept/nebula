//------------------------------------------------------------------------------
//  profiling.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "profiling/profiling.h"
namespace Profiling
{

Util::Array<ProfilingContext> profilingContexts;
Util::Array<Threading::AssertingMutex> contextMutexes;
Util::Dictionary<Util::StringAtom, Util::Array<ProfilingScope>> scopesByCategory;
Threading::CriticalSection categoryLock;
std::atomic_uint ProfilingContextCounter = 0;
thread_local IndexT ProfilingContextIndex = InvalidIndex;

//------------------------------------------------------------------------------
/**
*/
void 
ProfilingPushScope(const ProfilingScope& scope)
{	
	n_assert(ProfilingContextIndex != InvalidIndex);
	Threading::AssertingScope lock(&contextMutexes[ProfilingContextIndex]);

	// get thread context
	ProfilingContext& ctx = profilingContexts[ProfilingContextIndex];

	ctx.scopes.Push(scope);
	ctx.scopes.Peek().start = ctx.timer.GetTime();
}

//------------------------------------------------------------------------------
/**
*/
void
ProfilingPopScope()
{
	n_assert(ProfilingContextIndex != InvalidIndex);
	Threading::AssertingScope lock(&contextMutexes[ProfilingContextIndex]);

	// get thread context
	ProfilingContext& ctx = profilingContexts[ProfilingContextIndex];

	// we can safely assume the scope and timers won't be modified from different threads here
	ProfilingScope& scope = ctx.scopes.Pop();

	// add to category lookup
	scope.duration = ctx.timer.GetTime() - scope.start;
	//categoryLock.Enter();
	//scopesByCategory.AddUnique(scope.category).Append(scope);
	//categoryLock.Leave();

	// add to top level scopes if stack is empty
	if (ctx.scopes.IsEmpty())
	{
		if (scope.accum)
		{
			if (!ctx.topLevelScopes.IsEmpty())
			{
				ProfilingScope& parent = ctx.topLevelScopes.Back();
				if (parent.name == scope.name)
					parent.duration += scope.duration;
				else
					ctx.topLevelScopes.Append(scope);
			}
			else
				ctx.topLevelScopes.Append(scope);
		}
		else
			ctx.topLevelScopes.Append(scope);
	}
	else
	{
		// add as child scope
		ProfilingScope& parent = ctx.scopes.Peek();
		if (scope.accum && parent.name == scope.name)
			parent.duration += scope.duration;
		else
			parent.children.Append(scope);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
ProfilingNewFrame()
{
	// get thread context
	ProfilingContext& ctx = profilingContexts[ProfilingContextIndex];
	n_assert(ctx.threadName == "MainThread");

	for (IndexT i = 0; i < profilingContexts.Size(); i++)
	{
		profilingContexts[i].topLevelScopes.Clear();
		profilingContexts[i].timer.Reset();
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
ProfilingRegisterThread()
{
	// make sure we don't add contexts simulatenously
	Threading::CriticalScope lock(&categoryLock);
	Threading::ThreadId thread = Threading::Thread::GetMyThreadId();
	ProfilingContextIndex = ProfilingContextCounter.fetch_add(1);
	profilingContexts.Append(ProfilingContext());
	profilingContexts.Back().timer.Start();
	contextMutexes.Append(Threading::AssertingMutex());
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<ProfilingScope>&
ProfilingGetScopes(Threading::ThreadId thread)
{
#if NEBULA_DEBUG
	n_assert(profilingContexts[thread].topLevelScopes.IsEmpty());
#endif
	Threading::CriticalScope lock(&categoryLock);
	return profilingContexts[thread].topLevelScopes;
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<ProfilingContext>
ProfilingGetContexts()
{
	return profilingContexts;
}

//------------------------------------------------------------------------------
/**
*/
void 
ProfilingClear()
{
	for (IndexT i = 0; i < profilingContexts.Size(); i++)
	{
		n_assert(profilingContexts[i].scopes.Size() == 0);
		profilingContexts[i].topLevelScopes.Reset();
	}
}

} // namespace Profiling
