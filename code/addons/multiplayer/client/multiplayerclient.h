#pragma once
//------------------------------------------------------------------------------
/**
    @class Multiplayer::MultiplayerClient

    @copyright
    (C) 2025 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "GameNetworkingSockets/steam/steamnetworkingtypes.h"
#include "timing/timer.h"

struct ISteamNetworkingSockets;

//------------------------------------------------------------------------------
namespace Multiplayer
{

enum class ConnectionStatus
{
    Disconnected = 0,
    TryingToConnect = 1,
    Connected = 2
};

class MultiplayerClient : public Core::RefCounted
{
    __DeclareClass(MultiplayerClient);
public:
    /// constructor
    MultiplayerClient();
    /// destructor
    virtual ~MultiplayerClient();
    /// open the server
    bool Open();
    /// close the server
    void Close();
    /// return true if server is open
    bool IsOpen() const;

    bool TryConnect();

    ConnectionStatus GetConnectionStatus() const;

    void SyncAll();

    void OnNetConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* info);
    
    SizeT maxMessagesPerFrame = 1024;

private:
    void PollIncomingMessages();
    void PollConnectionChanges();
    void PushPendingMessages();

    bool isOpen;
    
    Timing::Timer timeoutTimer;

    ConnectionStatus connectionStatus = ConnectionStatus::Disconnected;

    HSteamNetConnection connectionId = k_HSteamNetConnection_Invalid;

    HSteamListenSocket listenSock;
	HSteamNetPollGroup pollGroup;
	ISteamNetworkingSockets* netInterface;
};

//------------------------------------------------------------------------------
/**
*/
inline bool
MultiplayerClient::IsOpen() const
{
    return this->isOpen;
}

} // namespace Multiplayer
//------------------------------------------------------------------------------
