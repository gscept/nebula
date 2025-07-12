//------------------------------------------------------------------------------
//  multiplayerclient.cc
//  (C) 2025 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "multiplayerclient.h"
#include "core/debug.h"
#include "GameNetworkingSockets/steam/steamnetworkingsockets.h"
#include "GameNetworkingSockets/steam/isteamnetworkingutils.h"
#include "core/sysfunc.h"
#include "flat/addons/multiplayer/protocol.h"
#include "multiplayer/multiplayerfeatureunit.h"
#include "net/socket/ipaddress.h"
#include "steam/steamnetworkingtypes.h"

namespace Multiplayer
{
__ImplementClass(Multiplayer::MultiplayerClient, 'MPCL', Core::RefCounted);

using namespace Util;

//--------------------------------------------------------------------------
/**
*/
// HACK: should probably be a singleton
static MultiplayerClient* callbackInstance = nullptr;
static void
SteamNetConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t* info)
{
    callbackInstance->OnNetConnectionStatusChanged(info);
}

//------------------------------------------------------------------------------
/**
*/
MultiplayerClient::MultiplayerClient()
    : isOpen(false)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
MultiplayerClient::~MultiplayerClient()
{
    n_assert(!this->IsOpen());
}

//------------------------------------------------------------------------------
/**
*/
bool
MultiplayerClient::Open()
{
    n_assert(!this->isOpen);
    this->netInterface = SteamNetworkingSockets();
    if (this->netInterface == nullptr)
    {
        n_error("Failed to initialize SteamNetworkingSockets!\n");
        return false;
    }
    this->isOpen = true;
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
MultiplayerClient::Close()
{
    n_assert(this->isOpen);
    this->isOpen = false;
}

//--------------------------------------------------------------------------
/**
*/
bool
MultiplayerClient::TryConnect()
{
    if (this->connectionStatus != ConnectionStatus::Disconnected)
    {
        // Already trying to connect, or already connected.
        return true;
    }

    const double TIMEOUT_INTERVAL = 1.0;
    if (this->timeoutTimer.Running() && this->timeoutTimer.GetTime() < TIMEOUT_INTERVAL)
    {
        return false;
    }
    
    const int NEBULA_DEFAULT_PORT = 61111;

    SteamNetworkingIPAddr serverAddr;
    serverAddr.Clear();
    // TODO: Implement support for casting ip to uint32_t address and uint16_t port
    //Net::IpAddress ipAddress = Net::IpAddress()
    serverAddr.SetIPv4(N_IP_ADDR(127, 0, 0, 1), NEBULA_DEFAULT_PORT);

    // Start connecting
    char address[SteamNetworkingIPAddr::k_cchMaxString];
    serverAddr.ToString(address, sizeof(address), true);
    n_printf("Trying to connect to server at %s...\n", address);
    
    SteamNetworkingConfigValue_t opt;
    opt.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, (void*)SteamNetConnectionStatusChangedCallback);

    this->connectionId = this->netInterface->ConnectByIPAddress(serverAddr, 1, &opt);
    
    if (this->connectionId == k_HSteamNetConnection_Invalid)
    {
        n_printf("Failed to connect by IP. (Invalid ip address or port?)\n");
        this->timeoutTimer.Reset();
        if (!this->timeoutTimer.Running())
        {
            this->timeoutTimer.Start();
        }
        return false;
    }

    this->connectionStatus = ConnectionStatus::TryingToConnect;
    return true;
}

//--------------------------------------------------------------------------
/**
*/
ConnectionStatus
MultiplayerClient::GetConnectionStatus() const
{
    return this->connectionStatus;
}

//--------------------------------------------------------------------------
/**
*/
void
MultiplayerClient::OnNetConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* info)
{
    // What's the state of the connection?
    switch ( info->m_info.m_eState )
    {
        case k_ESteamNetworkingConnectionState_None:
            // NOTE: We will get callbacks here when we destroy connections. We can ignore these for now.
            break;

        case k_ESteamNetworkingConnectionState_ClosedByPeer:
        case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
        {
            // Print an appropriate message
            if ( info->m_eOldState == k_ESteamNetworkingConnectionState_Connecting )
            {
                // Note: we could distinguish between a timeout, a rejected connection,
                // or some other transport problem.
                n_printf("%s\n", info->m_info.m_szEndDebug);
            }
            else if (info->m_info.m_eState == k_ESteamNetworkingConnectionState_ProblemDetectedLocally)
            {
                n_printf("Lost connection with server (%s)\n", info->m_info.m_szEndDebug);
            }
            else
            {
                // NOTE: We could check the reason code for a normal disconnection
                n_printf("Disconnected by host (%s)", info->m_info.m_szEndDebug);
            }

            // Clean up the connection.  This is important!
            // The connection is "closed" in the network sense, but
            // it has not been destroyed.  We must close it on our end, too
            // to finish up.  The reason information do not matter in this case,
            // and we cannot linger because it's already closed on the other end,
            // so we just pass 0's.
            this->netInterface->CloseConnection(info->m_hConn, 0, nullptr, false);
            if (this->connectionId == info->m_hConn)
            {
                this->connectionId = k_HSteamNetConnection_Invalid;
                this->connectionStatus = ConnectionStatus::Disconnected;
                this->timeoutTimer.Reset();
                if (!this->timeoutTimer.Running())
                {
                    this->timeoutTimer.Start();
                }
            }
            break;
        }

        case k_ESteamNetworkingConnectionState_Connecting:
            // We will get this callback when we start connecting.
            this->connectionStatus = ConnectionStatus::TryingToConnect;
            break;

        case k_ESteamNetworkingConnectionState_Connected:
            this->connectionStatus = ConnectionStatus::Connected;
            n_printf("Connected to server successfully!\n");
            break;

        default:
            break;
    }
}

//--------------------------------------------------------------------------
/**
*/
void
MultiplayerClient::SyncAll()
{
    this->PollIncomingMessages();
    this->PollConnectionChanges();
    this->PushPendingMessages();
}

//--------------------------------------------------------------------------
/**
*/
void
MultiplayerClient::PollIncomingMessages()
{
    ISteamNetworkingMessage* incomingMsg[this->maxMessagesPerFrame];
    int numMsgs = this->netInterface->ReceiveMessagesOnConnection(this->connectionId, incomingMsg, this->maxMessagesPerFrame);
    if (numMsgs == 0)
    {
        return;
    }
    if (numMsgs < 0)
    {
        n_error("Error checking for messages");
    }

    n_assert(numMsgs <= this->maxMessagesPerFrame && incomingMsg);
    for (int i = 0; i < numMsgs; i++)
    {
        void* data = incomingMsg[i]->m_pData;
        int bytes = incomingMsg[i]->m_cbSize;

        if (data != nullptr && bytes > 0)
        {
            Multiplayer::Protocol::Message const* protocolMessage = Multiplayer::Protocol::GetMessage(data);

            switch (protocolMessage->data_type())
            {
                case Multiplayer::Protocol::Data::Data_Test:
                {
                    Multiplayer::Protocol::MsgTest const* test = protocolMessage->data_as_Test();
                    Multiplayer::MultiplayerFeatureUnit::Instance()->temp_f = test->value();
                    n_printf("Got message: %f\n", test->value());
                    break;
                }
                case Multiplayer::Protocol::Data::Data_Connected:
                {
                    n_printf("Got message: CONNECTED\n");
                    break;
                }
            }
            incomingMsg[i]->Release();
        }
    }
    
}

//--------------------------------------------------------------------------
/**
*/
void
MultiplayerClient::PollConnectionChanges()
{
    callbackInstance = this;
    this->netInterface->RunCallbacks();
}

//--------------------------------------------------------------------------
/**
*/
void
MultiplayerClient::PushPendingMessages()
{
}

} // namespace Multiplayer
