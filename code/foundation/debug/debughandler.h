#pragma once
//------------------------------------------------------------------------------
/**
    @class Debug::DebugHandler
    
    The message handler for the debug interface. Just wakes up from time
    to time to check for incoming Http requests.
    
    @copyright
    (C) 2008 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "interface/interfacehandlerbase.h"
#include "io/console.h"
#include "debug/debugserver.h"
#include "http/httpserverproxy.h"

//------------------------------------------------------------------------------
namespace Debug
{
class DebugHandler : public Interface::InterfaceHandlerBase
{
    __DeclareClass(DebugHandler);
public:
    /// constructor
    DebugHandler();
    /// destructor
    virtual ~DebugHandler();
    
    /// open the handler
    virtual void Open();
    /// close the handler
    virtual void Close();
    /// do per-frame work
    virtual void DoWork();

private:
    Ptr<DebugServer> debugServer;
    Ptr<Http::HttpServerProxy> httpServerProxy;
};

} // namespace Debug
//------------------------------------------------------------------------------
    