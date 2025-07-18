#pragma once
//------------------------------------------------------------------------------
/**
    @class Multiplayer::ClientConnection

    @copyright
    (C) 2025 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "GameNetworkingSockets/steam/steamnetworkingtypes.h"

namespace Multiplayer
{

class BaseMultiplayerServer;

//--------------------------------------------------------------------------
/**
    Clients can be bucketet into client groups that can be polled at differing rates.
*/
enum class ClientGroup
{
    DontCare = 0,
    Lobby,
    Game,
    Monitoring,
    NumClientGroups
};


//--------------------------------------------------------------------------
/**
*/
class ClientConnection
{
public:
    /// constructor
    ClientConnection();
    /// destructor
    virtual ~ClientConnection();
    ///// connect using provided socket
    void Initialize(BaseMultiplayerServer* server, HSteamNetConnection connectionId);

    /// get the connection status
    bool IsConnected() const;
    /// shutdown the connection
    virtual void Shutdown();
    ///
    void SetClientGroup(ClientGroup group);
    ///
    ClientGroup GetClientGroup() const;
    ///
    uint32_t GetConnectionId() const;    
protected:
    BaseMultiplayerServer* server;
    ClientGroup group = ClientGroup::DontCare;
    HSteamNetConnection connectionId = k_HSteamNetConnection_Invalid;
};

//--------------------------------------------------------------------------
/**
*/
inline uint32_t
ClientConnection::GetConnectionId() const
{
    return this->connectionId;
}

//--------------------------------------------------------------------------
/**
*/
inline ClientGroup
ClientConnection::GetClientGroup() const
{
    return this->group;
}

} // namespace Multiplayer