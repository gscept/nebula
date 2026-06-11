#pragma once
//------------------------------------------------------------------------------
/**
    @class App::GameApplication

    Nebula's default game application. It creates and triggers the GameServer.
    For game features it creates the core and graphicsfeature which is used in every
    gamestate (such as level gamestates or only gui gamestates).

    @copyright
    (C) 2007 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file 
*/
#include "app/application.h"
#include "game/gameserver.h"
#include "core/singleton.h"
#include "core/coreserver.h"
#include "debug/debuginterface.h"
#include "io/ioserver.h"
#include "io/iointerface.h"
#include "io/gamecontentserver.h"
#include "resources/resourceserver.h"
#include "http/httpinterface.h"
#include "http/httpserverproxy.h"     
#include "basegamefeature/basegamefeatureunit.h"
#include "game/modulemanager.h"

//------------------------------------------------------------------------------
namespace App
{
class GameApplication : public Application
{
    __DeclareSingleton(GameApplication)

public:
    /// constructor
    GameApplication();
    /// destructor
    virtual ~GameApplication();
    /// open the application
    virtual bool Open();
    /// close the application
    virtual void Close();
    /// run the application
    virtual void Run();
    /// step one frame
    virtual void StepFrame();

    static IndexT FrameIndex;
    ///
    static bool IsEditorEnabled();

    /// return the module manager (may be nullptr if no runtime modules are configured)
    Ptr<Game::ModuleManager> GetModuleManager() const;


protected:
    /// setup game features
    virtual void SetupGameFeatures();
    /// cleanup game features
    virtual void CleanupGameFeatures(); 
    /// setup app from cmd lines
    virtual void SetupAppFromCmdLineArgs();
    /// parse runtime module startup options from command line
    virtual void SetupRuntimeModulesFromCmdLineArgs();
    /// enable runtime module
    void EnableRuntimeModule(const Util::String& moduleName, bool required = false);
    
    Ptr<Core::CoreServer> coreServer;   
    Ptr<IO::GameContentServer> gameContentServer;
    Ptr<Resources::ResourceServer> resourceServer;
    Ptr<IO::IoServer> ioServer;
    Ptr<IO::IoInterface> ioInterface;  
    Ptr<BaseGameFeature::BaseGameFeatureUnit> baseGameFeature;
    Ptr<Game::ModuleManager> moduleManager;
    Util::Dictionary<Util::String, Game::RuntimeModuleConfig> runtimeModuleConfigs;
    bool runtimeModuleStrictMode;


    static bool editorEnabled;
    
#if __NEBULA_HTTP__
    Ptr<Debug::DebugInterface> debugInterface;
    Ptr<Http::HttpInterface> httpInterface;
    Ptr<Http::HttpServerProxy> httpServerProxy;

    ushort defaultTcpPort;
#endif

    // game server
    Ptr<Game::GameServer> gameServer;

    // profiling
    _declare_timer(GameApplicationFrameTimeAll);

    class GameAppExitHandler : public Core::ExitHandler
    {
    public:
        GameAppExitHandler(GameApplication* app) : Core::ExitHandler(), app(app)
        {
        }
    private:
        virtual void OnExit() const
        {
            // make sure we gracefully shutdown everything
            if (app->isOpen)
                app->Close();
        }

        GameApplication* app;
    };

    GameAppExitHandler exitHandler;
};

//------------------------------------------------------------------------------
/**
*/
inline 
bool GameApplication::IsEditorEnabled()
{
    return editorEnabled;
}

} // namespace App
//------------------------------------------------------------------------------
