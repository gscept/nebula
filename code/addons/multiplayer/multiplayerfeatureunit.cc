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
#include "core/cvar.h"

namespace Multiplayer
{
__ImplementClass(Multiplayer::MultiplayerFeatureUnit, 'MPFU', Game::FeatureUnit);
__ImplementSingleton(MultiplayerFeatureUnit);

SteamNetworkingMicroseconds logTimeZero;

Core::CVar* net_fake_packet_lag_send        = Core::CVarCreate(Core::CVarType::CVar_Int, "net_fake_packet_lag_send",        "0", "Delay all outbound packets by N ms");
Core::CVar* net_fake_packet_lag_recv        = Core::CVarCreate(Core::CVarType::CVar_Int, "net_fake_packet_lag_recv",        "0", "Delay all inbound packets by N ms");
Core::CVar* net_fake_packet_loss_send       = Core::CVarCreate(Core::CVarType::CVar_Int, "net_fake_packet_loss_send",       "0", "[0--100] Randomly discard N percent of packets sent");
Core::CVar* net_fake_packet_loss_recv       = Core::CVarCreate(Core::CVarType::CVar_Int, "net_fake_packet_loss_recv",       "0", "[0--100] Randomly discard N percent of packets received");
Core::CVar* net_fake_packet_jitter_send_avg = Core::CVarCreate(Core::CVarType::CVar_Int, "net_fake_packet_jitter_send_avg", "0", "A random jitter time is generated using an exponential distribution using this value as the mean (ms).  The default is zero, which disables random jitter.");
Core::CVar* net_fake_packet_jitter_send_pct = Core::CVarCreate(Core::CVarType::CVar_Int, "net_fake_packet_jitter_send_pct", "0", "Odds (0-100) that a random jitter value for the packet will be generated.  Otherwise, a jitter value of zero is used, and the packet will only be delayed by the jitter system if necessary to retain order, due to the jitter of a previous packet.");
Core::CVar* net_fake_packet_jitter_send_max = Core::CVarCreate(Core::CVarType::CVar_Int, "net_fake_packet_jitter_send_max", "0", "Limit the random jitter time to this value (ms). Default 0.");
Core::CVar* net_fake_packet_jitter_recv_avg = Core::CVarCreate(Core::CVarType::CVar_Int, "net_fake_packet_jitter_recv_avg", "0", "A random jitter time is generated using an exponential distribution using this value as the mean (ms).  The default is zero, which disables random jitter.");
Core::CVar* net_fake_packet_jitter_recv_pct = Core::CVarCreate(Core::CVarType::CVar_Int, "net_fake_packet_jitter_recv_pct", "0", "Odds (0-100) that a random jitter value for the packet will be generated.  Otherwise, a jitter value of zero is used, and the packet will only be delayed by the jitter system if necessary to retain order, due to the jitter of a previous packet.");
Core::CVar* net_fake_packet_jitter_recv_max = Core::CVarCreate(Core::CVarType::CVar_Int, "net_fake_packet_jitter_recv_max", "0", "Limit the random jitter time to this value (ms). Default 0.");
Core::CVar* net_fake_packet_reorder_send    = Core::CVarCreate(Core::CVarType::CVar_Int, "net_fake_packet_reorder_send",    "0", "0-100 Percentage of packets to add additional delay to. If other packet(s) are sent/received within this delay window (that doesn't also randomly receive the same extra delay), then the packets become reordered.");
Core::CVar* net_fake_packet_reorder_recv    = Core::CVarCreate(Core::CVarType::CVar_Int, "net_fake_packet_reorder_recv",    "0", "0-100 Percentage of packets to add additional delay to. If other packet(s) are sent/received within this delay window (that doesn't also randomly receive the same extra delay), then the packets become reordered.");

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
    this->RegisterComponentType<Multiplayer::NetworkId>();
    this->RegisterComponentType<Multiplayer::NetworkTransform>({
        .OnInit=[](Game::World* world, Game::Entity entity, Multiplayer::NetworkTransform* netTransform)
        {
            //Game::TimeSource* timeSource = Game::Time::GetTimeSource(TIMESOURCE_GAMEPLAY);
            //Game::Position pos = world->GetComponent<Game::Position>(entity);
            //netTransform->positionExtrapolator.Reset(timeSource->time, timeSource->time, pos);
        }
    });
}

//--------------------------------------------------------------------------
/**
*/
void
RefreshConfigCVars(bool force = false)
{
    ISteamNetworkingUtils* utils = SteamNetworkingUtils();

    if (Core::CVarModified(net_fake_packet_lag_send) || force)
    {
        utils->SetGlobalConfigValueInt32(k_ESteamNetworkingConfig_FakePacketLag_Send, Core::CVarReadInt(net_fake_packet_lag_send));
    }
    if (Core::CVarModified(net_fake_packet_lag_recv) || force)
    {
        utils->SetGlobalConfigValueInt32(k_ESteamNetworkingConfig_FakePacketLag_Recv, Core::CVarReadInt(net_fake_packet_lag_recv));
    }
    if (Core::CVarModified(net_fake_packet_loss_send) || force)
    {
        utils->SetGlobalConfigValueFloat(k_ESteamNetworkingConfig_FakePacketLoss_Send, (float)Core::CVarReadInt(net_fake_packet_loss_send));
    }
    if (Core::CVarModified(net_fake_packet_loss_recv) || force)
    {
        utils->SetGlobalConfigValueFloat(k_ESteamNetworkingConfig_FakePacketLoss_Recv, (float)Core::CVarReadInt(net_fake_packet_loss_recv));
    }
    if (Core::CVarModified(net_fake_packet_jitter_send_avg) || force)
    {
        utils->SetGlobalConfigValueFloat(k_ESteamNetworkingConfig_FakePacketJitter_Send_Avg, (float)Core::CVarReadInt(net_fake_packet_jitter_send_avg));
    }
    if (Core::CVarModified(net_fake_packet_jitter_send_pct) || force)
    {
        utils->SetGlobalConfigValueFloat(k_ESteamNetworkingConfig_FakePacketJitter_Send_Pct, (float)Core::CVarReadInt(net_fake_packet_jitter_send_pct));
    }
    if (Core::CVarModified(net_fake_packet_jitter_send_max) || force)
    {
        utils->SetGlobalConfigValueFloat(k_ESteamNetworkingConfig_FakePacketJitter_Send_Max, (float)Core::CVarReadInt(net_fake_packet_jitter_send_max));
    }
    if (Core::CVarModified(net_fake_packet_jitter_recv_avg) || force)
    {
        utils->SetGlobalConfigValueFloat(k_ESteamNetworkingConfig_FakePacketJitter_Recv_Avg, (float)Core::CVarReadInt(net_fake_packet_jitter_recv_avg));
    }
    if (Core::CVarModified(net_fake_packet_jitter_recv_pct) || force)
    {
        utils->SetGlobalConfigValueFloat(k_ESteamNetworkingConfig_FakePacketJitter_Recv_Pct, (float)Core::CVarReadInt(net_fake_packet_jitter_recv_pct));
    }
    if (Core::CVarModified(net_fake_packet_jitter_recv_max) || force)
    {
        utils->SetGlobalConfigValueFloat(k_ESteamNetworkingConfig_FakePacketJitter_Recv_Max, (float)Core::CVarReadInt(net_fake_packet_jitter_recv_max));
    }
    if (Core::CVarModified(net_fake_packet_reorder_send) || force)
    {
        utils->SetGlobalConfigValueFloat(k_ESteamNetworkingConfig_FakePacketReorder_Send, (float)Core::CVarReadInt(net_fake_packet_reorder_send));
    }
    if (Core::CVarModified(net_fake_packet_reorder_recv) || force)
    {
        utils->SetGlobalConfigValueFloat(k_ESteamNetworkingConfig_FakePacketReorder_Recv, (float)Core::CVarReadInt(net_fake_packet_reorder_recv));
    }
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

    RefreshConfigCVars(true);

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

    RefreshConfigCVars();
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
