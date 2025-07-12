#pragma once
//------------------------------------------------------------------------------
/**
    @class Multiplayer::MultiplayerServer

    @copyright
    (C) 2025 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "clientconnection.h"
#include "GameNetworkingSockets/steam/steamnetworkingtypes.h"
#include "timing/timer.h"

struct ISteamNetworkingSockets;

//------------------------------------------------------------------------------
namespace Multiplayer
{
class MultiplayerServer : public Core::RefCounted
{
    __DeclareClass(MultiplayerServer);
public:
    /// constructor
    MultiplayerServer();
    /// destructor
    virtual ~MultiplayerServer();
    /// open the server
    bool Open();
    /// close the server
    void Close();
    /// return true if server is open
    bool IsOpen() const;

    void SyncAll();

    void OnNetConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* info);
    
    SizeT maxMessagesPerFrame = 1024;

private:
    friend ClientConnection;
    /// add a client connection (called by the listener thread)
    void AddClientConnection(ClientConnection* connection);

    void PollIncomingMessages();
    void PollConnectionChanges();
    void PushPendingMessages();

    bool isOpen;
    Util::HashTable<HSteamNetConnection, ClientConnection*, 128> clientConnections;

    HSteamListenSocket listenSock;
	HSteamNetPollGroup pollGroup;
	ISteamNetworkingSockets* netInterface;
    Timing::Timer sendTimer;
    double tickRate;
};

//------------------------------------------------------------------------------
/**
*/
inline bool
MultiplayerServer::IsOpen() const
{
    return this->isOpen;
}

} // namespace Multiplayer
//------------------------------------------------------------------------------
