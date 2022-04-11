#pragma once
//------------------------------------------------------------------------------
/**
    Profiling interface

    @copyright
    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "timing/time.h"
#include "timing/timer.h"
#include "util/stack.h"
#include "util/dictionary.h"
#include "util/stringatom.h"
#include "util/ringbuffer.h"
#include "util/tupleutility.h"
#include "threading/threadid.h"
#include "threading/thread.h"
#include "threading/criticalsection.h"
#include "threading/assertingmutex.h"
#include <atomic>

//------------------------------------------------------------------------------
/**
    Listing a bunch of marker names here for consistency:
    Graphics - All generic graphics/compute stuff
    Wait - When the engine is just waiting for a sync, GPU or CPU (these 
*/
//------------------------------------------------------------------------------

// use these macros to insert markers
#if NEBULA_ENABLE_PROFILING
#define N_SCOPE(name, cat)              Profiling::ProfilingScopeLock __##name##cat##scope__(#name, #cat, __FILE__, __LINE__);
#define N_SCOPE_DYN(str, cat)           Profiling::ProfilingScopeLock __dynscope##cat##__(str, #cat, __FILE__, __LINE__);
#define N_SCOPE_ACCUM(name, cat)        Profiling::ProfilingScopeLock __##name##cat##scope__(#name, #cat, __FILE__, __LINE__, true);
#define N_SCOPE_DYN_ACCUM(name, cat)    Profiling::ProfilingScopeLock __##name##cat##scope__(#name, #cat, __FILE__, __LINE__, true);
#define N_MARKER_BEGIN(name, cat)       { Profiling::ProfilingScope __##name##cat##scope__ = {#name, #cat, __FILE__, __LINE__, false}; Profiling::ProfilingPushScope(__##name##cat##scope__); }
#define N_MARKER_DYN_BEGIN(str, cat)    { Profiling::ProfilingScope __dynmarker##cat##__ = {str, #cat, __FILE__, __LINE__, false}; Profiling::ProfilingPushScope(__dynmarker##cat##__); }
#define N_MARKER_END()                  { Profiling::ProfilingPopScope(); }
#define N_COUNTER_INCR(name, value)     Profiling::ProfilingIncreaseCounter(name, value);
#define N_COUNTER_DECR(name, value)     Profiling::ProfilingDecreaseCounter(name, value);
#define N_BUDGET_COUNTER_SETUP(name, budget) Profiling::ProfilingSetupBudgetCounter(name, budget);
#define N_BUDGET_COUNTER_INCR(name, value) Profiling::ProfilingBudgetIncreaseCounter(name, value);
#define N_BUDGET_COUNTER_DECR(name, value) Profiling::ProfilingBudgetDecreaseCounter(name, value);
#define N_BUDGET_COUNTER_RESET(name) Profiling::ProfilingBudgetResetCounter(name);
#define N_DECLARE_COUNTER(name, label)  static const char* name = #label;
#else
#define N_SCOPE(name, cat)
#define N_SCOPE_DYN(str, cat)
#define N_SCOPE_ACCUM(name, cat)
#define N_SCOPE_DYN_ACCUM(name, cat)
#define N_MARKER_BEGIN(name, cat)
#define N_MARKER_END()
#define N_COUNTER_INCR(name, value)
#define N_COUNTER_DECR(name, value)
#define N_DECLARE_COUNTER(name, label)
#endif

N_DECLARE_COUNTER(N_GPU_MEMORY_COUNTER, GPU Allocated Memory);


namespace Profiling
{

struct ProfilingScope;
struct ProfilingContext;

/// push scope to scope stack
void ProfilingPushScope(const ProfilingScope& scope);
/// pop scope from scope stack
void ProfilingPopScope();
/// pushes an 'end of frame' marker, only available on the main thread
void ProfilingNewFrame();
/// get current frametime
Timing::Time ProfilingGetTime();

/// register a new thread for the profiling
void ProfilingRegisterThread();

/// get all top level scopes based on thread, only run when you know the thread is finished
const Util::Array<ProfilingScope>& ProfilingGetScopes(Threading::ThreadId thread);
/// get all profiling contexts
const Util::Array<ProfilingContext> ProfilingGetContexts();
/// clear all scopes
void ProfilingClear();

extern Util::Array<ProfilingContext> profilingContexts;
extern Util::Array<Threading::AssertingMutex> contextMutexes;
extern Util::Dictionary<Util::StringAtom, Util::Array<ProfilingScope>> scopesByCategory;
extern Threading::CriticalSection categoryLock;

/// atomic counter used to give each thread a unique id
extern Threading::AtomicCounter ProfilingContextCounter;

/// increment profiling counter
void ProfilingIncreaseCounter(const char* id, uint64 value);
/// decrement profiling counter
void ProfilingDecreaseCounter(const char* id, uint64 value);
/// return table of counters
const Util::Dictionary<const char*, uint64>& ProfilingGetCounters();

/// Setup a profiling budget counter
void ProfilingSetupBudgetCounter(const char* id, uint64 budget);
/// Increment budget counter
void ProfilingBudgetIncreaseCounter(const char* id, uint64 value);
/// Decrement budget counter
void ProfilingBudgetDecreaseCounter(const char* id, uint64 value);
/// Reset budget counter
void ProfilingBudgetResetCounter(const char* id);
/// Return set of budget counters
const Util::Dictionary<const char*, Util::Pair<uint64, uint64>>& ProfilingGetBudgetCounters();

extern Threading::CriticalSection counterLock;
extern Util::Dictionary<const char*, Util::Pair<uint64, uint64>> budgetCounters;
extern Util::Dictionary<const char*, uint64> counters;

struct ProfilingScope
{
    /// default constructor
    ProfilingScope()
        : name(nullptr)
        , category(nullptr)
        , file(nullptr)
        , line(-1)
        , duration(0)
        , start(0)
        , accum(false)
    {};

    /// constructor
    ProfilingScope(const char* name, Util::StringAtom category, const char* file, int line, bool accum)
        : name(name)
        , category(category)
        , file(file)
        , line(line)
        , duration(0)
        , start(0)
        , accum(accum)
    {};

    const char* name;
    Util::StringAtom category;
    const char* file;
    int line;
    bool accum;

    Timing::Time start;
    Timing::Time duration;
    Util::Array<ProfilingScope> children;
};

/// convenience class used to automatically push and pop scopes
struct ProfilingScopeLock
{
    /// constructor
    ProfilingScopeLock(const char* name, Util::StringAtom category, const char* file, int line, bool accum = false)
    {
        scope.name = name;
        scope.category = category;
        scope.file = file;
        scope.line = line;
        scope.accum = accum;
        ProfilingPushScope(scope);
    }

    /// destructor
    ~ProfilingScopeLock()
    {
        ProfilingPopScope();
    }
    ProfilingScope scope;
};

/// thread context of profiling
struct ProfilingContext
{
    ProfilingContext()
        : threadName(Threading::Thread::GetMyThreadName())
        , threadId(Threading::Thread::GetMyThreadId())
    {};
    Util::Stack<ProfilingScope> scopes;
    Util::Array<ProfilingScope> topLevelScopes;

    Timing::Timer timer;
    Util::StringAtom threadName;
    Threading::ThreadId threadId;
};

} // namespace Profiling
