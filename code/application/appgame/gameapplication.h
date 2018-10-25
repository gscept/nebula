#pragma once
//------------------------------------------------------------------------------
/**
    @class App::GameApplication

    Nebula's default game application. It creates and triggers the GameServer.
    For game features it creates the core and graphicsfeature which is used in every
    gamestate (such as level gamestates or only gui gamestates).

    (C) 2007 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file	
*/
#include "app/application.h"
#include "game/gameserver.h"
#include "core/singleton.h"
#include "util/dictionary.h"
#include "util/stringatom.h"
#include "core/coreserver.h"
#include "debug/debuginterface.h"
#include "io/ioserver.h"
#include "io/iointerface.h"
#include "io/gamecontentserver.h"
#include "http/httpinterface.h"
#include "http/httpserverproxy.h"     
#include "http/httpclientregistry.h"

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
	/// step one frame, mainly for debugging purposes
	virtual void StepFrame();

protected:
    /// setup game features
    virtual void SetupGameFeatures();
    /// cleanup game features
    virtual void CleanupGameFeatures(); 
    /// setup app from cmd lines
    virtual void SetupAppFromCmdLineArgs();

    Ptr<Core::CoreServer> coreServer;   
    Ptr<IO::GameContentServer> gameContentServer;
    Ptr<IO::IoServer> ioServer;
    Ptr<IO::IoInterface> ioInterface;  

#if __NEBULA_HTTP__
	Ptr<Debug::DebugInterface> debugInterface;
	Ptr<Http::HttpInterface> httpInterface;
	Ptr<Http::HttpServerProxy> httpServerProxy;

	ushort defaultTcpPort;
#endif

#if __NEBULA_HTTP_FILESYSTEM__
	Ptr<Http::HttpClientRegistry> httpClientRegistry;
#endif      

    // game server
    Ptr<Game::GameServer> gameServer;

    // profiling
	_declare_timer(GameApplicationFrameTimeAll);
};

} // namespace App
//------------------------------------------------------------------------------
