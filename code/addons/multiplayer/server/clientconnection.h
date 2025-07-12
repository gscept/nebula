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

class MultiplayerServer;

class ClientConnection
{
public:
    /// constructor
    ClientConnection();
    /// destructor
    virtual ~ClientConnection();
    ///// connect using provided socket
    void Initialize(Ptr<MultiplayerServer> server, HSteamNetConnection connectionId);
    /// get the connection status
    bool IsConnected() const;
    /// shutdown the connection
    virtual void Shutdown();
    /// get the client's ip address
    //const IpAddress& GetClientAddress() const;
    /// send accumulated content of send stream to server
    //virtual bool Send();
    
    HSteamNetConnection connectionId = k_HSteamNetConnection_Invalid;
protected:
    Ptr<MultiplayerServer> server;
};

} // namespace Multiplayer