//------------------------------------------------------------------------------
//  gameapplication.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
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
using namespace Util;

__ImplementSingleton(App::GameApplication);

using namespace Util;
using namespace Core;
using namespace IO;

//------------------------------------------------------------------------------
/**
*/
GameApplication::GameApplication()
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
               
        // setup basic Nebula3 runtime system
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
#endif
				
		//n_assert2(System::NebulaSettings::ReadString("gscept", "ToolkitShared", "workdir"), "No working directory defined!");

        this->coreServer->SetRootDirectory(root);
        this->coreServer->Open();

        // setup the job system
        this->jobSystem = Jobs::JobSystem::Create();
        this->jobSystem->Setup();

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

        // attach a log file console handler
#if __WIN32__
        Ptr<LogFileConsoleHandler> logFileHandler = LogFileConsoleHandler::Create();
        Console::Instance()->AttachHandler(logFileHandler.upcast<ConsoleHandler>());
#endif

        // create our game server and open it
        this->gameServer = Game::GameServer::Create();
        this->gameServer->Open();

        // create and add new game features
        this->SetupGameFeatures();

        // setup profiling stuff
        _setup_timer(GameApplicationFrameTimeAll);

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

    // shutdown basic Nebula3 runtime
    this->CleanupGameFeatures();
    this->gameServer->Close();
    this->gameServer = nullptr;

    this->gameContentServer->Discard();
    this->gameContentServer = nullptr;

    this->ioInterface->Close();
    this->ioInterface = nullptr;
    this->ioServer = nullptr;

    this->jobSystem->Discard();
    this->jobSystem = nullptr;

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
        _start_timer(GameApplicationFrameTimeAll);

        // trigger core server
        this->coreServer->Trigger();

        // trigger beginning of frame for feature units
        this->gameServer->OnBeginFrame();

		// trigger frame for feature units
		this->gameServer->OnFrame();

        // call the app's Run() method
        Application::Run();

		// trigger end of frame for feature units
		this->gameServer->OnEndFrame();

        _stop_timer(GameApplicationFrameTimeAll);
    }
}

//------------------------------------------------------------------------------
/**
    Setup new game features which should be used by this application.
    Overwrite for all features which have to be used.
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
