//------------------------------------------------------------------------------
//  @file clientconnection.cc
//  @copyright (C) 2025 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "clientconnection.h"
#include "basemultiplayerserver.h"
#include "GameNetworkingSockets/steam/steamnetworkingsockets.h"
#include "steam/isteamnetworkingutils.h"

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
    SteamNetworkingUtils()->SetConnectionConfigValueInt32(this->connectionId, k_ESteamNetworkingConfig_SendBufferSize, 4_KB );
    SteamNetworkingUtils()->SetConnectionConfigValueInt32(this->connectionId, k_ESteamNetworkingConfig_RecvBufferSize, 4_KB );
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

} // namespace Multiplayer
