#pragma once
//------------------------------------------------------------------------------
/**
    @class Multiplayer::BaseMultiplayerServer

    @copyright
    (C) 2025 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "clientconnection.h"
#include "GameNetworkingSockets/steam/steamnetworkingtypes.h"
#include "timing/timer.h"

class ISteamNetworkingSockets;

//------------------------------------------------------------------------------
namespace Multiplayer
{
class BaseMultiplayerServer
{
public:
    /// constructor
    BaseMultiplayerServer();
    /// destructor
    virtual ~BaseMultiplayerServer();
    /// open the server
    virtual bool Open();
    /// close the server
    virtual void Close();
    /// return true if server is open
    bool IsOpen() const;
    /// broadcast message to all clients
    void Broadcast(void* buf, int size);

    virtual bool OnClientIsConnecting(ClientConnection* connection);
    virtual void OnClientConnected(ClientConnection* connection);
    virtual void OnClientDisconnected(ClientConnection* connection);
    virtual void OnMessageReceived(Timing::Time recvTime, uint32_t connectionId, byte* data, size_t size);

    void SetClientGroupPollInterval(ClientGroup group, Timing::Time msBetweenTicks);

    void SyncAll();

    void OnNetConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* info);
    
    Timing::Time GetSendTickInterval() const;

    SizeT maxMessagesPerFrame = 1024;

protected:
    friend ClientConnection;
    /// add a client connection (called by the listener thread)
    void AddClientConnection(ClientConnection* connection);

    void PollIncomingMessages();
    void PollConnectionChanges();
    void PushPendingMessages();

    bool isOpen;
    Util::HashTable<HSteamNetConnection, ClientConnection*, 128> clientConnections;

	ISteamNetworkingSockets* netInterface;
    HSteamListenSocket listenSock;
	HSteamNetPollGroup pollGroups[(int)ClientGroup::NumClientGroups];
    Timing::Timer pollGroupTimers[(int)ClientGroup::NumClientGroups];
    Timing::Time pollGroupIntervals[(int)ClientGroup::NumClientGroups];
    
    Timing::Timer sendTimer;
    Timing::Time sendTickInterval;
};

//------------------------------------------------------------------------------
/**
*/
inline bool
BaseMultiplayerServer::IsOpen() const
{
    return this->isOpen;
}

//--------------------------------------------------------------------------
/**
*/
inline Timing::Time
BaseMultiplayerServer::GetSendTickInterval() const
{
    return this->sendTickInterval;
}

} // namespace Multiplayer
//------------------------------------------------------------------------------
