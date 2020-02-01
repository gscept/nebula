//------------------------------------------------------------------------------
//  profiling.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "foundation/profiling.h"
#include "util/stack.h"
#include "util/dictionary.h"
#include "util/stringatom.h"
namespace Profiling
{

Util::Stack<ProfilingScope> scopeStack;
Util::Stack<Timing::Timer> timers;
Util::Dictionary<Util::StringAtom, Util::Array<ProfilingScope>> scopesByCategory;
Util::Array<ProfilingScope> topLevelScopes;

//------------------------------------------------------------------------------
/**
*/
void 
ProfilingPushScope(const ProfilingScope& scope)
{	
	scopeStack.Push(scope);
	timers.Push(Timing::Timer());
	timers.Peek().Start();	
}

//------------------------------------------------------------------------------
/**
*/
void
ProfilingPopScope()
{
	Timing::Timer& timer = timers.Pop();
	timer.Stop();
	ProfilingScope& scope = scopeStack.Pop();

	// add to category lookup
	scope.duration = timer.GetTime();
	scopesByCategory.AddUnique(scope.category).Append(scope);

	// add to top level scopes if necessary
	if (scopeStack.IsEmpty())
		topLevelScopes.Append(scope);
	else
	{
		// add as child scope
		scopeStack.Peek().children.Append(scope);
	}
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<ProfilingScope>& 
ProfilingGetScopes()
{
	return topLevelScopes;
}

//------------------------------------------------------------------------------
/**
*/
void 
ProfilingClear()
{
	topLevelScopes.Clear();
}

} // namespace Profiling
