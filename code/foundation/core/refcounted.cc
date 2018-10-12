//------------------------------------------------------------------------------
//  refcounted.cc
//  (C) 2006 RadonLabs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file (C) 2002 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "core/refcounted.h"

namespace Core
{
__ImplementRootClass(Core::RefCounted, 'REFC');

#if NEBULA_DEBUG
using namespace Util;

Threading::CriticalSection RefCounted::criticalSection;
ThreadLocal bool RefCounted::isInCreate = false;
RefCountedList RefCounted::list;

#endif

//------------------------------------------------------------------------------
/**
*/
RefCounted::~RefCounted()
{
    n_assert(0 == this->refCount);
    #if NEBULA_DEBUG
        n_assert(!this->destroyed);
        n_assert(0 != this->listIterator);
        criticalSection.Enter();
        list.Remove(this->listIterator);
        criticalSection.Leave();
        this->listIterator = 0;
        this->destroyed = true;
    #endif
}

//------------------------------------------------------------------------------
/**
    This method should be called as the very last before an application 
    exits.
*/
void
RefCounted::DumpRefCountingLeaks()
{
    #if NEBULA_DEBUG
    list.DumpLeaks();    
    #endif
}

//------------------------------------------------------------------------------
/**
    Gather per-class stats.
*/
#if NEBULA_DEBUG
Dictionary<String,RefCounted::Stats>
RefCounted::GetOverallStats()
{
    Dictionary<String,Stats> result;
    criticalSection.Enter();
    // iterate over refcounted list
    RefCountedList::Iterator iter;
    for (iter = list.Begin(); iter != list.End(); iter++)
    {
        RefCounted* cur = *iter;
        if (!result.Contains(cur->GetClassName()))
        {
            Stats newStats;
            newStats.className   = cur->GetClassName();
            newStats.classFourCC = cur->GetClassFourCC();
            newStats.numObjects  = 1;
            newStats.overallRefCount = cur->GetRefCount();
            newStats.instanceSize = cur->GetRtti()->GetInstanceSize();
            result.Add(cur->GetClassName(), newStats);
        }
        else
        {
            Stats& stats = result[cur->GetClassName()];
            stats.numObjects++;
            stats.overallRefCount += cur->GetRefCount();
        }
    }
    criticalSection.Leave();
    return result;
}
#endif

} // namespace Foundation