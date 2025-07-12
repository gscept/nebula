//------------------------------------------------------------------------------
//  @file clientconnection.cc
//  @copyright (C) 2025 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "clientconnection.h"
#include "multiplayerserver.h"
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
ClientConnection::Initialize(Ptr<MultiplayerServer> server, HSteamNetConnection connectionId)
{
    this->server = server;
    this->connectionId = connectionId;
    SteamNetworkingUtils()->SetConnectionConfigValueInt32(this->connectionId, k_ESteamNetworkingConfig_SendBufferSize, 40_KB );
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

} // namespace Multiplayer
