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

    /// Called when client is trying to connect. Override and return true if the connection should be accepted.
    virtual bool OnClientIsConnecting(ClientConnection* connection);
    /// Called when client has successfully connected to the server.
    virtual void OnClientConnected(ClientConnection* connection);
    /// Called when client has disconnected from the server.
    virtual void OnClientDisconnected(ClientConnection* connection);
    /// Called when a message has been received from a client.
    virtual void OnMessageReceived(ClientConnection* connection, Timing::Time recvTime, byte* data, size_t size);
    
    /// Called every "frame". 
    virtual void OnFrame();
    /// Called every `tickInterval` seconds, after OnFrame.
    virtual void OnTick();

    /// Returns the interval between ticks, in seconds.
    Timing::Time GetTickInterval() const;
    /// Sets the interval (seconds) between ticks
    void SetTickInterval(Timing::Time interval);

    ///
    void SetClientGroupPollInterval(ClientGroup group, Timing::Time msBetweenTicks);

    /// Checks for connection changes, polls messages and calls OnFrame/OnTick.
    void SyncAll();

    /// Used internally by GameNetworkingSockets at connection status changes.
    void OnNetConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* info);
    
    SizeT maxMessagesPerFrame = 1024;
    
protected:
    friend ClientConnection;
    /// add a client connection (called by the listener thread)
    void AddClientConnection(ClientConnection* connection);
    
    void PollIncomingMessages();
    void PollConnectionChanges();
    
    bool isOpen;
    Util::HashTable<HSteamNetConnection, ClientConnection*, 128> clientConnections;
    
	ISteamNetworkingSockets* netInterface;
    HSteamListenSocket listenSock;
	HSteamNetPollGroup pollGroups[(int)ClientGroup::NumClientGroups];
    Timing::Timer pollGroupTimers[(int)ClientGroup::NumClientGroups];
    Timing::Time pollGroupIntervals[(int)ClientGroup::NumClientGroups];
    
    Timing::Timer tickTimer;
    Timing::Time tickInterval;
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
BaseMultiplayerServer::GetTickInterval() const
{
    return this->tickInterval;
}

//--------------------------------------------------------------------------
/**
*/
inline void
BaseMultiplayerServer::SetTickInterval(Timing::Time interval)
{
    this->tickInterval = interval;
}

} // namespace Multiplayer
//------------------------------------------------------------------------------
