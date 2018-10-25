#pragma once
//------------------------------------------------------------------------------
/**
    @class Debug::DebugServer
  
    The debug server singleton is visible from all threads and keeps track
    of debug timer and debug counters.
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "core/singleton.h"
#include "threading/criticalsection.h"
#include "util/stringatom.h"

//------------------------------------------------------------------------------
namespace Debug
{
class DebugCounter;
class DebugTimer;

class DebugServer : public Core::RefCounted
{
    __DeclareClass(DebugServer);
    __DeclareInterfaceSingleton(DebugServer);
public:
    /// constructor
    DebugServer();
    /// destructor
    virtual ~DebugServer();
    
    /// return true if debug server is open
    bool IsOpen() const;
    /// register a debug timer
    void RegisterDebugTimer(const Ptr<DebugTimer>& timer);
    /// unregister a debug timer
    void UnregisterDebugTimer(const Ptr<DebugTimer>& timer);
    /// register a debug counter
    void RegisterDebugCounter(const Ptr<DebugCounter>& counter);
    /// unregister a debug counter
    void UnregisterDebugCounter(const Ptr<DebugCounter>& counter);
    /// get all registered debug timers
    Util::Array<Ptr<DebugTimer>> GetDebugTimers() const;
    /// get all registered debug counters
    Util::Array<Ptr<DebugCounter>> GetDebugCounters() const;    
    /// get debug timer by name, returns invalid ptr if not exists
    Ptr<DebugTimer> GetDebugTimerByName(const Util::StringAtom& name) const;
    /// get debug counter by name, returns invalid ptr if not exists
    Ptr<DebugCounter> GetDebugCounterByName(const Util::StringAtom& name) const;
    
private:
    friend class DebugHandler;
    friend class DebugPageHandler;

    /// open the debug server
    void Open();
    /// close the debug server
    void Close();

    bool isOpen;
    Threading::CriticalSection critSect;
    Util::Dictionary<Util::StringAtom, Ptr<DebugTimer>> debugTimers;
    Util::Dictionary<Util::StringAtom, Ptr<DebugCounter>> debugCounters;
};

//------------------------------------------------------------------------------
/**
*/
inline bool
DebugServer::IsOpen() const
{
    bool retval;
    this->critSect.Enter();
    retval = this->isOpen;
    this->critSect.Leave();
    return retval;
}

} // namespace Debug
//------------------------------------------------------------------------------
