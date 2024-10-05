//------------------------------------------------------------------------------
//  profiling.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "profiling/profiling.h"

namespace Profiling
{

Util::Array<ProfilingContext> ProfilingContexts;
Util::Array<Threading::CriticalSection*> ContextMutexes;
Util::Dictionary<Util::StringAtom, Util::Array<ProfilingScope>> ScopesByCategory;
Threading::CriticalSection CategoryLock;
Threading::AtomicCounter ProfilingContextCounter = 0;
Threading::Interlocked::AtomicInt Enabled = { 0 };
thread_local IndexT ProfilingContextIndex = InvalidIndex;

//------------------------------------------------------------------------------
/**
*/
void 
ProfilingPushScope(const ProfilingScope& scope)
{   
    n_assert(ProfilingContextIndex != InvalidIndex);
    if (!Enabled.Load())
        return;
    ContextMutexes[ProfilingContextIndex]->Enter();

    // get thread context
    ProfilingContext& ctx = ProfilingContexts[ProfilingContextIndex];

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
    if (!Enabled.Load())
        return;

    // get thread context
    ProfilingContext& ctx = ProfilingContexts[ProfilingContextIndex];
    ProfilingScope scope = ctx.scopes.Pop();
    
    // add to category lookup
    scope.duration = ctx.timer.GetTime() - scope.start;

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
    ContextMutexes[ProfilingContextIndex]->Leave();
}

//------------------------------------------------------------------------------
/**
*/
void
ProfilingNewFrame()
{
    // get thread context
    //ProfilingContext& ctx = profilingContexts[ProfilingContextIndex];
    //n_assert(ctx.threadName == "MainThread");

    for (IndexT i = 0; i < ProfilingContexts.Size(); i++)
    {
        Threading::CriticalScope lock(ContextMutexes[i]);
        n_assert(ProfilingContexts[i].scopes.IsEmpty());
        ProfilingContexts[i].topLevelScopes.Clear();
        ProfilingContexts[i].timer.Reset();
    }
}

//------------------------------------------------------------------------------
/**
*/
Timing::Time 
ProfilingGetTime()
{
    // get current context and return time
    ProfilingContext& ctx = ProfilingContexts[ProfilingContextIndex];
    return ctx.timer.GetTime();
}

//------------------------------------------------------------------------------
/**
*/
void 
ProfilingRegisterThread()
{
    // make sure we don't add contexts simulatenously
    Threading::CriticalScope lock(&CategoryLock);
    ProfilingContextIndex = Threading::Interlocked::Add(&ProfilingContextCounter, 1);
    ProfilingContexts.Append(ProfilingContext());
    ProfilingContexts.Back().timer.Start();
    ContextMutexes.Append(new Threading::CriticalSection);
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<ProfilingScope>&
ProfilingGetScopes(Threading::ThreadId thread)
{
#if NEBULA_DEBUG
    n_assert(ProfilingContexts[thread].topLevelScopes.IsEmpty());
#endif
    Threading::CriticalScope lock(&CategoryLock);
    return ProfilingContexts[thread].topLevelScopes;
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<ProfilingContext>
ProfilingGetContexts()
{
    return ProfilingContexts;
}

//------------------------------------------------------------------------------
/**
*/
void 
ProfilingClear()
{
    for (IndexT i = 0; i < ProfilingContexts.Size(); i++)
    {
        n_assert(ProfilingContexts[i].scopes.Size() == 0);
        ProfilingContexts[i].topLevelScopes.Reset();
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

//------------------------------------------------------------------------------
/**
*/
void
ProfilingEnable(bool enabled)
{
    Enabled.Exchange(enabled);
}

} // namespace Profiling
