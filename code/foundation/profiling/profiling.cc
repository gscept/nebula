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

	// get thread id
	ProfilingContext& ctx = profilingContexts[ProfilingContextIndex];

	ctx.scopes.Push(scope);
	ctx.timers.Push(Timing::Timer());
	ctx.timers.Peek().Start();
}

//------------------------------------------------------------------------------
/**
*/
void
ProfilingPopScope()
{
	n_assert(ProfilingContextIndex != InvalidIndex);
	Threading::AssertingScope lock(&contextMutexes[ProfilingContextIndex]);

	// get thread id
	ProfilingContext& ctx = profilingContexts[ProfilingContextIndex];

	// we can safely assume the scope and timers won't be modified from different threads here
	Timing::Timer& timer = ctx.timers.Pop();
	timer.Stop();
	ProfilingScope& scope = ctx.scopes.Pop();

	// add to category lookup
	scope.duration = timer.GetTime();
	categoryLock.Enter();
	scopesByCategory.AddUnique(scope.category).Append(scope);
	categoryLock.Leave();

	// add to top level scopes if necessary
	if (ctx.scopes.IsEmpty())
	{
		// lock since we might modify the top level dictionary keys
		ctx.topLevelScopes.Append(scope);
	}
	else
	{
		// add as child scope
		ctx.scopes.Peek().children.Append(scope);
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
		n_assert(profilingContexts[i].timers.Size() == 0);
		profilingContexts[i].topLevelScopes.Clear();
	}
}

} // namespace Profiling
