#pragma once
//------------------------------------------------------------------------------
/**
    @class Test::FModAudioApplication
    
    Simple audio test application.
    
    (C) 2008 Radon Labs GmbH
*/

#define USE_HTTP                                1


#include "util/array.h"
#include "core/ptr.h"
#include "input/inputserver.h"
#include "timing/timer.h"
#include "core/coreserver.h"
#if USE_HTTP
#include "http/httpinterface.h"
#include "debug/debuginterface.h"
#include "http/httpserverproxy.h"
#endif
#include "io/gamecontentserver.h"
#include "io/assignregistry.h"
#include "io/ioserver.h"
#include "scripting/scriptserver.h"


//------------------------------------------------------------------------------
namespace Test
{
class ScriptTestApplication
{
public:
    /// constructor
    ScriptTestApplication();
    /// destructor
    virtual ~ScriptTestApplication();
    /// open the application
    virtual bool Open();
    /// close the application
    virtual void Close();
    /// close the application
    virtual void Run();

private:
    

private:
	Ptr<Core::CoreServer> coreServer;

#if USE_HTTP
    Ptr<Http::HttpInterface> httpInterface;
	Ptr<Http::HttpServerProxy> httpServerProxy;
	Ptr<Debug::DebugInterface> debugInterface;
#endif
    Timing::Timer masterTime;

	Ptr<Scripting::ScriptServer> scriptServer;
	
};

} // namespace Test
//------------------------------------------------------------------------------
