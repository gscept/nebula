#pragma once
//------------------------------------------------------------------------------
/**
    Profiling interface

    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "timing/time.h"
#include "timing/timer.h"
#include "util/stack.h"
#include "util/dictionary.h"
#include "util/stringatom.h"
#include "threading/criticalsection.h"
#include "threading/assertingmutex.h"
#include <atomic>

// use these macros to insert markers
#if NEBULA_ENABLE_PROFILING
#define N_SCOPE(name, cat) Profiling::ProfilingScopeLock __##name##cat##scope__(#name, #cat, __FILE__, __LINE__);
#define N_MARKER_BEGIN(name, cat) { Profiling::ProfilingScope __##name##cat##scope__ = {#name, #cat, __FILE__, __LINE__}; Profiling::ProfilingPushScope(__##name##cat##scope__); }
#define N_MARKER_END() { Profiling::ProfilingPopScope(); }
#else
#define N_SCOPE(name, cat)
#define N_MARKER_BEGIN(name, cat)
#define N_MARKER_END()
#endif

namespace Profiling
{

struct ProfilingScope;
struct ProfilingContext;

/// push scope to scope stack
void ProfilingPushScope(const ProfilingScope& scope);
/// pop scope from scope stack
void ProfilingPopScope();

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
extern std::atomic_uint ProfilingContextCounter;

struct ProfilingScope
{
    /// default constructor
    ProfilingScope()
        : name(nullptr)
        , category(nullptr)
        , file(nullptr)
        , line(-1)
        , duration(0)
    {};

    /// constructor
    ProfilingScope(const char* name, Util::StringAtom category, const char* file, int line)
        : name(name)
        , category(category)
        , file(file)
        , line(line)
        , duration(0)
    {};

    const char* name;
    Util::StringAtom category;
    const char* file;
    int line;

    Timing::Time duration;
    Util::Array<ProfilingScope> children;
};

/// convenience class used to automatically push and pop scopes
struct ProfilingScopeLock
{
    /// constructor
    ProfilingScopeLock(const char* name, Util::StringAtom category, const char* file, int line)
    {
        scope.name = name;
        scope.category = category;
        scope.file = file;
        scope.line = line;
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
    Util::Stack<Timing::Timer> timers;
    Util::Array<ProfilingScope> topLevelScopes;

    Util::StringAtom threadName;
    Threading::ThreadId threadId;
};

} // namespace Profiling
