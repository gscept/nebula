//------------------------------------------------------------------------------
//  gameapplication.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "appgame/gameapplication.h"

#include "core/sysfunc.h"
#include "options.h"
#include "core/debug/corepagehandler.h"
#include "threading/debug/threadpagehandler.h"
#include "memory/debug/memorypagehandler.h"
#include "io/debug/iopagehandler.h"
#include "io/logfileconsolehandler.h"
#include "io/debug/consolepagehandler.h"
#include "messaging/messagecallbackhandler.h"
#include "system/nebulasettings.h"
#include "io/fswrapper.h"
#include "jobs2/jobs2.h"
#include "input/inputserver.h"
#include "basegamefeature/basegamefeatureunit.h"
#include "nflatbuffer/flatbufferinterface.h"
#include "profiling/profiling.h"

namespace App
{
__ImplementSingleton(App::GameApplication);
IndexT GameApplication::FrameIndex = 1;
bool GameApplication::editorEnabled = false;

using namespace Util;
using namespace Core;
using namespace IO;
using namespace Http;
using namespace Debug;

//------------------------------------------------------------------------------
/**
*/
GameApplication::GameApplication() :
#if __NEBULA_HTTP__
    defaultTcpPort(2100),
#endif
    runtimeModuleStrictMode(false),
    exitHandler(this)
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

        Flat::FlatbufferInterface::Init();

        Options::InitOptions();

        this->SetupRuntimeModulesFromCmdLineArgs();

        Jobs2::JobSystemInitInfo jobSystemInfo;
        jobSystemInfo.numThreads = System::NumCpuCores;
        jobSystemInfo.name = "JobSystem";
        jobSystemInfo.scratchMemorySize = 16_MB;
        Jobs2::JobSystemInit(jobSystemInfo);

        this->resourceServer = Resources::ResourceServer::Create();
        this->resourceServer->Open();


#if NEBULA_ENABLE_PROFILING
        Profiling::ProfilingRegisterThread(15);
#endif

        // attach a log file console handler
#if __WIN32__
        Ptr<LogFileConsoleHandler> logFileHandler = LogFileConsoleHandler::Create();
        Console::Instance()->AttachHandler(logFileHandler.upcast<ConsoleHandler>());
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
        this->gameServer->SetCmdLineArgs(this->GetCmdLineArgs());

        // always attach the base game feature
        this->baseGameFeature = BaseGameFeature::BaseGameFeatureUnit::Create();
        this->gameServer->AttachGameFeature(this->baseGameFeature);

        // create and add new game features
        this->SetupGameFeatures();

        if (this->runtimeModuleConfigs.Size() > 0)
        {
            this->moduleManager = Game::ModuleManager::Create();
            const bool loaded = this->moduleManager->LoadModules(this->runtimeModuleConfigs, this->gameServer, this->runtimeModuleStrictMode);
            if (!loaded && this->runtimeModuleStrictMode)
            {
                n_warning("GameApplication::Open(): runtime module loading failed in strict mode\n");
                this->moduleManager->UnloadModules(this->gameServer);
                this->moduleManager = nullptr;
                return false;
            }
        }

        // open the game server
        this->gameServer->Open();
        // start the game
        this->gameServer->Start();
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

    //_discard_timer(GameApplicationFrameTimeAll);

    // shutdown basic Nebula runtime
    this->gameServer->Stop();
    this->gameServer->CleanupWorld(Game::GetWorld(WORLD_DEFAULT));

    if (this->moduleManager.isvalid())
    {
        this->moduleManager->UnloadModules(this->gameServer);
        this->moduleManager = nullptr;
    }

    this->gameServer->Close();

    this->CleanupGameFeatures();

    this->gameServer->RemoveGameFeature(this->baseGameFeature);
    this->baseGameFeature = nullptr;

    this->gameServer = nullptr;

    this->gameContentServer->Discard();
    this->gameContentServer = nullptr;

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

    this->ioInterface->Close();
    this->ioInterface = nullptr;
    this->ioServer = nullptr;

    this->coreServer->Close();
    this->coreServer = nullptr;

    Jobs2::JobSystemUninit();

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
    Input::InputServer* inputServer = Input::InputServer::Instance();
    while (!inputServer->IsQuitRequested())
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

#if NEBULA_ENABLE_PROFILING
    Profiling::ProfilingNewFrame();
#endif

#if __NEBULA_HTTP__
    this->httpServerProxy->HandlePendingRequests();
#endif

    N_SCOPE(StepFrame, Game)

    Jobs2::JobNewFrame();

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
    editorEnabled = args.GetBoolFlag("-editor");
}

//------------------------------------------------------------------------------
/**
*/
void
GameApplication::SetupRuntimeModulesFromCmdLineArgs()
{
    this->runtimeModuleConfigs.Clear();
    this->runtimeModuleStrictMode = false;
    const Util::CommandLineArgs& args = this->GetCmdLineArgs();

    auto findModuleConfig = [this](const Util::String& moduleName) -> IndexT
    {
        Util::String check = moduleName;
        check.ToLower();
        for (IndexT i = 0; i < this->runtimeModuleConfigs.Size(); i++)
        {
            Util::String candidate = this->runtimeModuleConfigs[i].name;
            candidate.ToLower();
            if (candidate == check)
                return i;
        }
        return InvalidIndex;
    };

    // Initialize runtime modules from project settings. CLI flags can then
    // override these values on a per-module basis.
    for (IndexT i = 0; i < (IndexT)Options::ProjectSettings.runtime_modules.size(); i++)
    {
        const std::unique_ptr<App::RuntimeModuleSettingsT>& moduleSettings = Options::ProjectSettings.runtime_modules[i];
        if (!moduleSettings)
            continue;

        if (moduleSettings->name.empty())
            continue;

        Game::RuntimeModuleConfig config;
        config.name = moduleSettings->name.c_str();
        config.path = moduleSettings->path.c_str();
        config.enabled = moduleSettings->enabled;
        config.required = moduleSettings->required;
        this->runtimeModuleConfigs.Append(config);
    }

    this->runtimeModuleStrictMode = Options::ProjectSettings.runtime_modules_strict;

    if (args.HasArg("-module"))
    {
        Util::Array<Util::String> modules = args.GetStrings("-module");
        for (IndexT i = 0; i < modules.Size(); i++)
        {
            if (!modules[i].IsValid())
                continue;

            IndexT index = findModuleConfig(modules[i]);
            if (index == InvalidIndex)
            {
                Game::RuntimeModuleConfig config;
                config.name = modules[i];
                config.enabled = true;
                this->runtimeModuleConfigs.Append(config);
            }
            else
            {
                this->runtimeModuleConfigs[index].enabled = true;
            }
        }
    }

    if (args.HasArg("-modulepath"))
    {
        Util::Array<Util::String> mappings = args.GetStrings("-modulepath");
        for (IndexT i = 0; i < mappings.Size(); i++)
        {
            Util::Array<Util::String> pair;
            mappings[i].Tokenize("=", pair);
            if (pair.Size() != 2)
            {
                n_warning("GameApplication: ignoring invalid -modulepath value '%s' (expected name=path)\n", mappings[i].AsCharPtr());
                continue;
            }

            IndexT index = findModuleConfig(pair[0]);
            if (index == InvalidIndex)
            {
                Game::RuntimeModuleConfig config;
                config.name = pair[0];
                config.path = pair[1];
                config.enabled = true;
                this->runtimeModuleConfigs.Append(config);
            }
            else
            {
                this->runtimeModuleConfigs[index].path = pair[1];
            }
        }
    }

    if (args.HasArg("-nomodule"))
    {
        Util::Array<Util::String> disabledModules = args.GetStrings("-nomodule");
        for (IndexT i = 0; i < disabledModules.Size(); i++)
        {
            if (!disabledModules[i].IsValid())
                continue;

            IndexT index = findModuleConfig(disabledModules[i]);
            if (index == InvalidIndex)
            {
                Game::RuntimeModuleConfig config;
                config.name = disabledModules[i];
                config.enabled = false;
                this->runtimeModuleConfigs.Append(config);
            }
            else
            {
                this->runtimeModuleConfigs[index].enabled = false;
            }
        }
    }

    if (args.GetBoolFlag("-modulestrict"))
    {
        this->runtimeModuleStrictMode = true;
    }

}

} // namespace App
