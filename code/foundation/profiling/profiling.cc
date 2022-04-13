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
Threading::AtomicCounter ProfilingContextCounter = 0;
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
    ProfilingScope scope = ctx.scopes.Pop();

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
    //n_assert(ctx.threadName == "MainThread");

    for (IndexT i = 0; i < profilingContexts.Size(); i++)
    {
        profilingContexts[i].topLevelScopes.Clear();
        profilingContexts[i].timer.Reset();
    }
}

//------------------------------------------------------------------------------
/**
*/
Timing::Time 
ProfilingGetTime()
{
    // get current context and return time
    ProfilingContext& ctx = profilingContexts[ProfilingContextIndex];
    return ctx.timer.GetTime();
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
    ProfilingContextIndex = Threading::Interlocked::Add(&ProfilingContextCounter, 1);
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

Threading::CriticalSection counterLock;
Util::Dictionary<const char*, uint64> counters;
Util::Dictionary<const char*, Util::Pair<uint64, uint64>> budgetCounters;

//------------------------------------------------------------------------------
/**
*/
void 
ProfilingIncreaseCounter(const char* id, uint64 value)
{
    counterLock.Enter();
    IndexT idx = counters.FindIndex(id);
    if (idx == InvalidIndex)
        counters.Add(id, value);
    else
    {
        counters.ValueAtIndex(idx) += value;
    }
    counterLock.Leave();
}

//------------------------------------------------------------------------------
/**
*/
void 
ProfilingDecreaseCounter(const char* id, uint64 value)
{
    counterLock.Enter();
    IndexT idx = counters.FindIndex(id);
    n_assert(idx != InvalidIndex);
    counters.ValueAtIndex(idx) -= value;
    counterLock.Leave();
}

//------------------------------------------------------------------------------
/**
*/
const Util::Dictionary<const char*, uint64>&
ProfilingGetCounters()
{
    return counters;
}

//------------------------------------------------------------------------------
/**
*/
void
ProfilingSetupBudgetCounter(const char* id, uint64 budget)
{
    counterLock.Enter();

    // Add budget
    IndexT idx = budgetCounters.FindIndex(id);
    n_assert(idx == InvalidIndex);
    budgetCounters.Add(id, { budget, 0 });

    counterLock.Leave();
}

//------------------------------------------------------------------------------
/**
*/
void
ProfilingBudgetIncreaseCounter(const char* id, uint64 value)
{
    IndexT idx = budgetCounters.FindIndex(id);
    n_assert(idx != InvalidIndex);
    budgetCounters.ValueAtIndex(idx).second += value;
}

//------------------------------------------------------------------------------
/**
*/
void
ProfilingBudgetDecreaseCounter(const char* id, uint64 value)
{
    IndexT idx = budgetCounters.FindIndex(id);
    n_assert(idx != InvalidIndex);
    budgetCounters.ValueAtIndex(idx).second -= value;
}

//------------------------------------------------------------------------------
/**
*/
void
ProfilingBudgetResetCounter(const char* id)
{
    IndexT idx = budgetCounters.FindIndex(id);
    n_assert(idx != InvalidIndex);

    // Just reset the counter to budget
    budgetCounters.ValueAtIndex(idx).second = 0;
}

//------------------------------------------------------------------------------
/**
*/
const Util::Dictionary<const char*, Util::Pair<uint64, uint64>>&
ProfilingGetBudgetCounters()
{
    return budgetCounters;
}

} // namespace Profiling
