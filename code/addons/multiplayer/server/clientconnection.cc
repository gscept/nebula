//------------------------------------------------------------------------------
//  @file clientconnection.cc
//  @copyright (C) 2025 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "clientconnection.h"
#include "basemultiplayerserver.h"
#include "GameNetworkingSockets/steam/steamnetworkingsockets.h"
#include "steam/isteamnetworkingutils.h"
#include "imgui.h"

namespace Multiplayer
{

//--------------------------------------------------------------------------
/**
*/
ClientConnection::ClientConnection()
{
    // empty
}

//--------------------------------------------------------------------------
/**
*/
ClientConnection::~ClientConnection()
{
    // empty
}

//--------------------------------------------------------------------------
/**
*/
void
ClientConnection::Initialize(BaseMultiplayerServer* server, HSteamNetConnection connectionId)
{
    this->server = server;
    this->connectionId = connectionId;
    SteamNetworkingUtils()->SetConnectionConfigValueInt32(this->connectionId, k_ESteamNetworkingConfig_SendBufferSize, 1_MB );
    SteamNetworkingUtils()->SetConnectionConfigValueInt32(this->connectionId, k_ESteamNetworkingConfig_RecvBufferSize, 1_MB );
}

//--------------------------------------------------------------------------
/**
*/
bool
ClientConnection::IsConnected() const
{
    return this->connectionId != k_HSteamNetConnection_Invalid;
}

//--------------------------------------------------------------------------
/**
*/
void
ClientConnection::Shutdown()
{
    this->server->netInterface->CloseConnection(this->connectionId, 0, "Disconnected from server", true);
    this->connectionId = k_HSteamNetConnection_Invalid;
}

//--------------------------------------------------------------------------
/**
*/
void
ClientConnection::SetClientGroup(ClientGroup group)
{
    this->group = group;
}

//--------------------------------------------------------------------------
/**
*/
Timing::Time
ClientConnection::GetCurrentPing() const
{
    SteamNetConnectionRealTimeStatus_t status;
    EResult res = this->server->netInterface->GetConnectionRealTimeStatus(this->connectionId, &status, 0, nullptr);
    if (res == k_EResultNoConnection)
    {
        return 0.0f;
    }
    
    return (double)status.m_nPing / 1000.0;
}

//------------------------------------------------------------------------------
/**
*/
void
ClientConnection::DrawNetworkDebugInfo()
{
    ImGui::Text("Connection %u", (uint32_t)this->connectionId);

    SteamNetConnectionRealTimeStatus_t status;
    EResult res = this->server->netInterface->GetConnectionRealTimeStatus(this->connectionId, &status, 0, nullptr);
    if (res != k_EResultOK)
    {
        ImGui::Text("Connection status unavailable: %i", (int)res);
        return;
    }

    ImGui::Text("Ping: %i ms", status.m_nPing);
    ImGui::Text("Out: %.1f KB/s (%.1f pkt/s)", status.m_flOutBytesPerSec / 1024.0f, status.m_flOutPacketsPerSec);
    ImGui::Text("In:  %.1f KB/s (%.1f pkt/s)", status.m_flInBytesPerSec / 1024.0f, status.m_flInPacketsPerSec);
    ImGui::Text("Send capacity: %.1f KB/s", (float)status.m_nSendRateBytesPerSecond / 1024.0f);
    ImGui::Text("Pending: reliable=%i B unreliable=%i B", status.m_cbPendingReliable, status.m_cbPendingUnreliable);
    ImGui::Text("Unacked reliable: %i B", status.m_cbSentUnackedReliable);
    ImGui::Text("Queue time: %.2f ms", (double)status.m_usecQueueTime / 1000.0);
    ImGui::Text("Quality: local=%.2f remote=%.2f", status.m_flConnectionQualityLocal, status.m_flConnectionQualityRemote);
}

} // namespace Multiplayer
