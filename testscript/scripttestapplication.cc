//------------------------------------------------------------------------------
//  ScriptTestApplication.cc
//  (C) 2009 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "scripttestapplication.h"
#include "math/matrix44.h"
#include "input/gamepad.h"
#include "io/ioserver.h"
#include "util/stringatom.h"
#include "io/fswrapper.h"
#include "math/vector.h"
#include "io/logfileconsolehandler.h"
#include "jobs/jobsystem.h"
#include "commands/stdlibrary.h"
#include "scripting/lua/luaserver.h"
#include "scripting/debug/scriptingpagehandler.h"



using namespace Math;
using namespace Base;
using namespace Test;
#if USE_HTTP
using namespace Http;
#endif


//------------------------------------------------------------------------------
/*
*/
ScriptTestApplication::ScriptTestApplication()
{
}

//------------------------------------------------------------------------------
/*
*/
ScriptTestApplication::~ScriptTestApplication()
{
}

//------------------------------------------------------------------------------
/*
*/
bool 
ScriptTestApplication::Open()
{
    this->coreServer = Core::CoreServer::Create();
    this->coreServer->SetCompanyName(Util::StringAtom("Radon Labs GmbH"));
    this->coreServer->SetAppName(Util::StringAtom("PS3 Audio Test Simple"));
    this->coreServer->Open();

	this->scriptServer = Scripting::LuaServer::Create();
	this->scriptServer->Open();
	Commands::StdLibrary::Register();
	
	this->scriptServer->PrintCommandList();
	this->scriptServer->Eval("listcmds()");
	
    // attach a log file console handler
    

#if USE_HTTP
    // setup HTTP server
    this->httpInterface = Http::HttpInterface::Create();
    this->httpInterface->Open();
    this->httpServerProxy = HttpServerProxy::Create();
    this->httpServerProxy->Open();

	this->httpServerProxy->AttachRequestHandler(Debug::ScriptingPageHandler::Create());
#endif
	this->debugInterface = Debug::DebugInterface::Create();
	this->debugInterface->Open();

    this->masterTime.Start();

    

    return true;
}

//------------------------------------------------------------------------------
/*
*/
void 
ScriptTestApplication::Close()
{    

    this->masterTime.Stop();

#if USE_HTTP
    this->httpServerProxy->Close();
    this->httpServerProxy = 0;

    this->httpInterface->Close();
    this->httpInterface = 0;
#endif


    this->coreServer->Close();
    this->coreServer = 0;
}

//------------------------------------------------------------------------------
/*
*/
void 
ScriptTestApplication::Run()
{
    // waiting for game pad
    

    while(true)
    {
    
        Core::SysFunc::Sleep(0.01);
    }
}

//------------------------------------------------------------------------------
/*
*/
