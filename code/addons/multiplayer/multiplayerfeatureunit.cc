//------------------------------------------------------------------------------
//  multiplayer/multiplayerfeatureunit.cc
//  (C) 2025 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "basegamefeature/managers/timemanager.h"
#include "game/api.h"
#include "game/featureunit.h"
#include "multiplayerfeatureunit.h"
#include "game/world.h"
#include "imgui.h"
#include "GameNetworkingSockets/steam/steamnetworkingsockets.h"
#include "GameNetworkingSockets/steam/isteamnetworkingutils.h"
#include "multiplayer/client/basemultiplayerclient.h"
#include "multiplayer/components/multiplayer.h"
#include "game/processor.h"
#include "io/jsonreader.h"
#include "io/jsonwriter.h"

namespace Multiplayer
{
__ImplementClass(Multiplayer::MultiplayerFeatureUnit, 'MPFU', Game::FeatureUnit);
__ImplementSingleton(MultiplayerFeatureUnit);


SteamNetworkingMicroseconds logTimeZero;

//--------------------------------------------------------------------------
/**
*/
static void DebugOutput(ESteamNetworkingSocketsDebugOutputType eType, const char *pszMsg)
{
	SteamNetworkingMicroseconds time = SteamNetworkingUtils()->GetLocalTimestamp() - logTimeZero;
	n_printf( "%10.6f %s\n", time*1e-6, pszMsg );
	if ( eType == k_ESteamNetworkingSocketsDebugOutputType_Bug )
	{
        Core::SysFunc::Error("k_ESteamNetworkingSocketsDebugOutputType_Bug encountered! Exiting...");
		Core::SysFunc::Exit(1);
	}
}

//--------------------------------------------------------------------------
/**
*/
static void InitSteamDatagramConnectionSockets()
{
    SteamDatagramErrMsg errMsg;
    if (!GameNetworkingSockets_Init(nullptr, errMsg))
    {
        n_error("GameNetworkingSockets_Init failed.  %s", errMsg);
        return;
    }

    logTimeZero = SteamNetworkingUtils()->GetLocalTimestamp();
    SteamNetworkingUtils()->SetDebugOutputFunction(k_ESteamNetworkingSocketsDebugOutputType_Msg, DebugOutput);
}

//--------------------------------------------------------------------------
/**
*/
static void ShutdownSteamDatagramConnectionSockets()
{
	// TODO: We need to send a message and then either wait for the peer to close
    // the connection, or pool the connection to see if any reliable data is pending.

    // Give connections time to finish up.  This is an application layer protocol
	// here, it's not TCP.  Note that if you have an application and you need to be
	// more sure about cleanup, you won't be able to do this. 
    Core::SysFunc::Sleep(0.5);

    GameNetworkingSockets_Kill();
}

//------------------------------------------------------------------------------
/**
*/
MultiplayerFeatureUnit::MultiplayerFeatureUnit()
{
    __ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
MultiplayerFeatureUnit::~MultiplayerFeatureUnit()
{
    __DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
void
MultiplayerFeatureUnit::OnAttach()
{
    this->RegisterComponentType<Multiplayer::NetworkId>({.replicate=true});
    this->RegisterComponentType<Multiplayer::NetworkTransform>({
        .replicate=true, 
        .OnInit=[](Game::World* world, Game::Entity entity, Multiplayer::NetworkTransform* netTransform)
        {
            //Game::TimeSource* timeSource = Game::Time::GetTimeSource(TIMESOURCE_GAMEPLAY);
            //Game::Position pos = world->GetComponent<Game::Position>(entity);
            //netTransform->positionExtrapolator.Reset(timeSource->time, timeSource->time, pos);
        }
    });
}

//------------------------------------------------------------------------------
/**
*/
void
MultiplayerFeatureUnit::OnActivate()
{
    Game::World* world = Game::GetWorld(WORLD_DEFAULT);
    Util::StringAtom frameEvent = "OnNetworkUpdate"_atm;
    world->GetFramePipeline().RegisterFrameEvent(250, frameEvent);

    InitSteamDatagramConnectionSockets();
    
    if (this->server != nullptr)
    {
        this->server->Open();
    }
    if (this->client != nullptr)
    {
        this->client->Open();
    }

    FeatureUnit::OnActivate();
}

//------------------------------------------------------------------------------
/**
*/
void
MultiplayerFeatureUnit::OnDeactivate()
{
    FeatureUnit::OnDeactivate();

    if (this->server != nullptr)
    {
        this->server->Close();
    }
    if (this->client != nullptr)
    {
        this->client->Close();
    }

    ShutdownSteamDatagramConnectionSockets();
}

//------------------------------------------------------------------------------
/**
*/
void
MultiplayerFeatureUnit::OnBeginFrame()
{
    Game::FeatureUnit::OnBeginFrame();

    //ImGui::Begin("MultiplayerFeatureUnit");
    //ImGui::Text("const char *fmt, ...");
    //ImGui::End();
}

//------------------------------------------------------------------------------
/**
*/
void
MultiplayerFeatureUnit::OnFrame()
{   
    Game::FeatureUnit::OnFrame();
}

//------------------------------------------------------------------------------
/**
*/
void
MultiplayerFeatureUnit::OnEndFrame()
{
    Game::FeatureUnit::OnEndFrame();
    
    if (this->server != nullptr && this->server->IsOpen())
    {
        this->server->SyncAll();
    }
    
    if (this->client != nullptr)
    {
        if (this->client->GetConnectionStatus() == Multiplayer::ConnectionStatus::Disconnected)
        {
            this->client->TryConnect();
        }
        
        // Should not be else, or else if!
        if (this->client->GetConnectionStatus() != Multiplayer::ConnectionStatus::Disconnected)
        {
            this->client->SyncAll();
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
MultiplayerFeatureUnit::OnRenderDebug()
{
    Game::FeatureUnit::OnRenderDebug();
}

} // namespace Scripting

//--------------------------------------------------------------------------
namespace IO
{
template<> void JsonReader::Get<Math::Extrapolator<Math::vec3>>(Math::Extrapolator<Math::vec3>& ret, const char* attr)
{
    // Not serialized
}

template<> void JsonWriter::Add<Math::Extrapolator<Math::vec3>>(Math::Extrapolator<Math::vec3> const& value, Util::String const& attr)
{
    // Not serialized
}
} // namespace IO
