#pragma once
//------------------------------------------------------------------------------
/**
    Profiling interface

    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "timing/time.h"
#include "timing/timer.h"
namespace Profiling
{

#define N_MARKER(name, cat) ProfilingScopeLock __##name##cat##scope__(#name, #cat, __FILE__, __LINE__);
struct ProfilingScope;

/// push scope to scope stack
void ProfilingPushScope(const ProfilingScope& scope);
/// pop scope from scope stack
void ProfilingPopScope();

/// get all top level scopes
const Util::Array<ProfilingScope>& ProfilingGetScopes();
/// clear all scopes
void ProfilingClear();

extern Util::Stack<ProfilingScope> scopeStack;
extern Util::Stack<Timing::Timer> timers;
extern Util::Dictionary<Util::StringAtom, Util::Array<ProfilingScope>> scopesByCategory;
extern Util::Array<ProfilingScope> topLevelScopes;

struct ProfilingScope
{
    /// default constructor
    ProfilingScope()
        : name(nullptr)
        , category(nullptr)
        , file(nullptr)
        , line(-1)
    {};

    const char* name;
    Util::StringAtom category;
    const char* file;
    int line;
    Timing::Time duration;

    Util::Array<ProfilingScope> children;
};

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

} // namespace Profiling
