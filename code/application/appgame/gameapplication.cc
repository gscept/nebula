//------------------------------------------------------------------------------
//  gameapplication.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "appgame/gameapplication.h"
#include "core/debug/corepagehandler.h"
#include "threading/debug/threadpagehandler.h"
#include "memory/debug/memorypagehandler.h"
#include "io/debug/iopagehandler.h"
#include "io/logfileconsolehandler.h"
#include "io/debug/consolepagehandler.h"
#include "messaging/messagecallbackhandler.h"
#include "system/nebulasettings.h"
#include "io/fswrapper.h"

namespace App
{
__ImplementSingleton(App::GameApplication);
IndexT GameApplication::FrameIndex = -1;

using namespace Util;
using namespace Core;
using namespace IO;
using namespace Http;
using namespace Debug;

//------------------------------------------------------------------------------
/**
*/
GameApplication::GameApplication()
#if __NEBULA_HTTP__
	:defaultTcpPort(2100)
#endif
{
    __ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
GameApplication::~GameApplication()
{
    n_assert(!this->IsOpen());
    __DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
bool
GameApplication::Open()
{
    n_assert(!this->IsOpen());
    if (Application::Open())
    {
        // setup from cmd line args
        this->SetupAppFromCmdLineArgs();
               
        // setup basic Nebula runtime system
        this->coreServer = CoreServer::Create();
        this->coreServer->SetCompanyName(Application::Instance()->GetCompanyName());
        this->coreServer->SetAppName(Application::Instance()->GetAppTitle());
                
		Util::String root = IO::FSWrapper::GetHomeDirectory();

#if PUBLIC_BUILD
		if(System::NebulaSettings::Exists(Application::Instance()->GetCompanyName(),Application::Instance()->GetAppTitle(),"path"))
		{
			root = System::NebulaSettings::ReadString(Application::Instance()->GetCompanyName(),Application::Instance()->GetAppTitle(),"path");
		}
#else 
		if(System::NebulaSettings::Exists("gscept", "ToolkitShared", "workdir"))
		{
			root = System::NebulaSettings::ReadString("gscept", "ToolkitShared", "workdir");
		}
        if (System::NebulaSettings::Exists("gscept", "ToolkitShared", "path"))
        {
            this->coreServer->SetToolDirectory(System::NebulaSettings::ReadString("gscept", "ToolkitShared", "path"));
        }
#endif
				
		//n_assert2(System::NebulaSettings::ReadString("gscept", "ToolkitShared", "workdir"), "No working directory defined!");

        this->coreServer->SetRootDirectory(root);
        this->coreServer->Open();        

        // setup game content server
        this->gameContentServer = GameContentServer::Create();
        this->gameContentServer->SetTitle(this->GetAppTitle());
        this->gameContentServer->SetTitleId(this->GetAppID());
        this->gameContentServer->SetVersion(this->GetAppVersion());
        this->gameContentServer->Setup();

        // setup io subsystem
        this->ioServer = IoServer::Create();
        this->ioServer->MountStandardArchives();
        this->ioInterface = IoInterface::Create();
        this->ioInterface->Open();

        this->resourceServer = Resources::ResourceServer::Create();
        this->resourceServer->Open();        

        // attach a log file console handler
#if __WIN32__
        Ptr<LogFileConsoleHandler> logFileHandler = LogFileConsoleHandler::Create();
        Console::Instance()->AttachHandler(logFileHandler.upcast<ConsoleHandler>());
#endif

#if __NEBULA_HTTP_FILESYSTEM__
		// setup http subsystem
		this->httpClientRegistry = Http::HttpClientRegistry::Create();
		this->httpClientRegistry->Setup();
#endif

#if __NEBULA_HTTP__
		// setup http subsystem
		this->httpInterface = Http::HttpInterface::Create();
		this->httpInterface->SetTcpPort(this->defaultTcpPort);
		this->httpInterface->Open();
		this->httpServerProxy = Http::HttpServerProxy::Create();
		this->httpServerProxy->Open();
		this->httpServerProxy->AttachRequestHandler(Debug::CorePageHandler::Create());
		this->httpServerProxy->AttachRequestHandler(Debug::ThreadPageHandler::Create());
		this->httpServerProxy->AttachRequestHandler(Debug::MemoryPageHandler::Create());
		this->httpServerProxy->AttachRequestHandler(Debug::ConsolePageHandler::Create());
		this->httpServerProxy->AttachRequestHandler(Debug::IoPageHandler::Create());
		//this->httpServerProxy->AttachRequestHandler(Debug::GamePageHandler::Create());

		// setup debug subsystem
		this->debugInterface = DebugInterface::Create();
		this->debugInterface->Open();
#endif

        // create our game server and open it
        this->gameServer = Game::GameServer::Create();
        this->gameServer->Open();

        // create and add new game features
        this->SetupGameFeatures();

        // setup profiling stuff
	    _setup_grouped_timer(GameApplicationFrameTimeAll, "Game Subsystem");
        

        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
void
GameApplication::Close()
{
    n_assert(this->IsOpen());

    _discard_timer(GameApplicationFrameTimeAll);

    // shutdown basic Nebula runtime
    this->CleanupGameFeatures();
    this->gameServer->Close();
    this->gameServer = nullptr;

    this->gameContentServer->Discard();
    this->gameContentServer = nullptr;

	this->ioInterface->Close();
	this->ioInterface = nullptr;
	this->ioServer = nullptr;

	this->resourceServer->Close();
	this->resourceServer = nullptr;

#if __NEBULA_HTTP__
	this->debugInterface->Close();
	this->debugInterface = nullptr;

	this->httpServerProxy->Close();
	this->httpServerProxy = nullptr;
	this->httpInterface->Close();
	this->httpInterface = nullptr;
#endif

#if __NEBULA_HTTP_FILESYSTEM__
	this->httpClientRegistry->Discard();
	this->httpClientRegistry = nullptr;
#endif

    this->coreServer->Close();
    this->coreServer = nullptr;

    Application::Close();
}

//------------------------------------------------------------------------------
/**
    Run the application. This method will return when the application wishes
    to exit.
*/
void
GameApplication::Run()
{
    while (true)
    {
		this->StepFrame();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
GameApplication::StepFrame()
{
    _start_timer(GameApplicationFrameTimeAll);

#if __NEBULA_HTTP__
	this->httpServerProxy->HandlePendingRequests();
#endif

	// trigger core server
	this->coreServer->Trigger();

    // update resources
    this->resourceServer->Update(GameApplication::FrameIndex);
	// trigger beginning of frame for feature units
	this->gameServer->OnBeginFrame();

	// trigger frame for feature units
	this->gameServer->OnFrame();

	// call the app's Run() method
	Application::Run();

	// trigger end of frame for feature units
	this->gameServer->OnEndFrame();
    
    GameApplication::FrameIndex++;

    _stop_timer(GameApplicationFrameTimeAll);
}

//------------------------------------------------------------------------------
/**
    Setup new game features which should be used by this application.
    Overwride for all features which have to be used.

	Make sure that features are setup ONLY in this method, since other
	systems might not expect otherwise.
*/
void
GameApplication::SetupGameFeatures()
{
    // create any features in derived class
}

//------------------------------------------------------------------------------
/**
    Cleanup all added game features
*/
void
GameApplication::CleanupGameFeatures()
{
    // cleanup your features in derived class
}

//------------------------------------------------------------------------------
/**
*/
void
GameApplication::SetupAppFromCmdLineArgs()
{
    // allow rename of application
    const Util::CommandLineArgs& args = this->GetCmdLineArgs();
    if (args.HasArg("-appname"))
    {
        this->SetAppTitle(args.GetString("-appname"));
    }
}

} // namespace App
