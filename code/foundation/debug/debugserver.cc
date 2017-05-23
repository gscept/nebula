//------------------------------------------------------------------------------
//  debugserver.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "debug/debugserver.h"
#include "debug/debugtimer.h"
#include "debug/debugcounter.h"

namespace Debug
{
__ImplementClass(Debug::DebugServer, 'DBGS', Core::RefCounted);
__ImplementInterfaceSingleton(Debug::DebugServer);

using namespace Util;

//------------------------------------------------------------------------------
/**
*/
DebugServer::DebugServer() :
    isOpen(false)
{
    __ConstructInterfaceSingleton;
}

//------------------------------------------------------------------------------
/**
*/
DebugServer::~DebugServer()
{
    n_assert(!this->isOpen);
    __DestructInterfaceSingleton;
}

//------------------------------------------------------------------------------
/**
*/
void
DebugServer::Open()
{
    this->critSect.Enter();
    n_assert(!this->isOpen);
    this->isOpen = true;
    this->critSect.Leave();
}

//------------------------------------------------------------------------------
/**
*/
void
DebugServer::Close()
{
    this->critSect.Enter();
    n_assert(this->isOpen);
    this->debugTimers.Clear();
    this->debugCounters.Clear();
    this->isOpen = false;
    this->critSect.Leave();
}

//------------------------------------------------------------------------------
/**
*/
void
DebugServer::RegisterDebugTimer(const Ptr<DebugTimer>& timer)
{
    this->critSect.Enter();
    n_assert(this->isOpen);
    n_assert(!this->debugTimers.Contains(timer->GetName()));
    this->debugTimers.Add(timer->GetName(), timer);
    this->critSect.Leave();
}

//------------------------------------------------------------------------------
/**
*/
void
DebugServer::UnregisterDebugTimer(const Ptr<DebugTimer>& timer)
{
    this->critSect.Enter();
    n_assert(this->isOpen);
    n_assert(this->debugTimers.Contains(timer->GetName()));
    this->debugTimers.Erase(timer->GetName());
    this->critSect.Leave();
}

//------------------------------------------------------------------------------
/**
*/
void
DebugServer::RegisterDebugCounter(const Ptr<DebugCounter>& counter)
{
    this->critSect.Enter();
    n_assert(this->isOpen);
    n_assert(!this->debugCounters.Contains(counter->GetName()));
    this->debugCounters.Add(counter->GetName(), counter);
    this->critSect.Leave();
}

//------------------------------------------------------------------------------
/**
*/
void
DebugServer::UnregisterDebugCounter(const Ptr<DebugCounter>& counter)
{
    this->critSect.Enter();
    n_assert(this->isOpen);
    n_assert(this->debugCounters.Contains(counter->GetName()));
    this->debugCounters.Erase(counter->GetName());
    this->critSect.Leave();
}

//------------------------------------------------------------------------------
/**
*/
Array<Ptr<DebugTimer> >
DebugServer::GetDebugTimers() const
{
    this->critSect.Enter();
    Array<Ptr<DebugTimer> > result = this->debugTimers.ValuesAsArray();
    this->critSect.Leave();
    return result;
}

//------------------------------------------------------------------------------
/**
*/
Array<Ptr<DebugCounter> >
DebugServer::GetDebugCounters() const
{
    this->critSect.Enter();
    Array<Ptr<DebugCounter> > result = this->debugCounters.ValuesAsArray();
    this->critSect.Leave();
    return result;
}

//------------------------------------------------------------------------------
/**
*/
Ptr<DebugTimer>
DebugServer::GetDebugTimerByName(const StringAtom& name) const
{
    Ptr<DebugTimer> result;
    this->critSect.Enter();
    if (this->debugTimers.Contains(name))
    {
        result = this->debugTimers[name];
    }
    this->critSect.Leave();
    return result;
}

//------------------------------------------------------------------------------
/**
*/
Ptr<DebugCounter>
DebugServer::GetDebugCounterByName(const StringAtom& name) const
{
    Ptr<DebugCounter> result;
    this->critSect.Enter();
    if (this->debugCounters.Contains(name))
    {
        result = this->debugCounters[name];
    }
    this->critSect.Leave();
    return result;
}

} // namespace Debug
