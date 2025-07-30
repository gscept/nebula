//------------------------------------------------------------------------------
//  basemultiplayerclient.cc
//  (C) 2025 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "basemultiplayerclient.h"
#include "basegamefeature/managers/timemanager.h"
#include "components/multiplayer.h"
#include "core/debug.h"
#include "GameNetworkingSockets/steam/steamnetworkingsockets.h"
#include "GameNetworkingSockets/steam/isteamnetworkingutils.h"
#include "game/api.h"
#include "game/world.h"
#include "multiplayer/client/clientprocessors.h"
#include "nflatbuffer/nebula_flat.h"
#include "nflatbuffer/flatbufferinterface.h"
#include "flat/addons/multiplayer/standardprotocol.h"
#include "multiplayer/multiplayerfeatureunit.h"
#include "net/socket/ipaddress.h"
#include "steam/steamnetworkingtypes.h"
#include "components/multiplayer.h"

namespace Multiplayer
{

using namespace Util;

//--------------------------------------------------------------------------
/**
*/
// HACK: should probably be a singleton
static BaseMultiplayerClient* callbackInstance = nullptr;
static void
SteamNetConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t* info)
{
    callbackInstance->OnNetConnectionStatusChanged(info);
}

//------------------------------------------------------------------------------
/**
*/
BaseMultiplayerClient::BaseMultiplayerClient()
    : isOpen(false)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
BaseMultiplayerClient::~BaseMultiplayerClient()
{
    n_assert(!this->IsOpen());
}

//------------------------------------------------------------------------------
/**
*/
bool
BaseMultiplayerClient::Open()
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
BaseMultiplayerClient::Close()
{
    n_assert(this->isOpen);

    ShutdownClientProcessors();

    this->isOpen = false;
}

//--------------------------------------------------------------------------
/**
*/
bool
BaseMultiplayerClient::TryConnect()
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
void
BaseMultiplayerClient::OnIsConnecting()
{
    // Override in subclass
}

//--------------------------------------------------------------------------
/**
*/
void
BaseMultiplayerClient::OnConnected()
{
    // Override in subclass
}

//--------------------------------------------------------------------------
/**
*/
void
BaseMultiplayerClient::OnDisconnected()
{
    // Override in subclass
}

//--------------------------------------------------------------------------
/**
*/
void
BaseMultiplayerClient::OnMessageReceived(SteamNetworkingMessage_t* msg)
{
    // Override in subclass
}

//--------------------------------------------------------------------------
/**
*/
ConnectionStatus
BaseMultiplayerClient::GetConnectionStatus() const
{
    return this->connectionStatus;
}

//--------------------------------------------------------------------------
/**
*/
void
BaseMultiplayerClient::OnNetConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* info)
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

            this->OnDisconnected();

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
            this->connectionId = info->m_hConn;
            this->OnIsConnecting();
            break;

        case k_ESteamNetworkingConnectionState_Connected:
            this->connectionStatus = ConnectionStatus::Connected;
            this->OnConnected();
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
BaseMultiplayerClient::SyncAll()
{
    this->PollConnectionChanges();

    if (this->connectionStatus != ConnectionStatus::Connected)
        return;

    SteamNetConnectionRealTimeStatus_t status;
    EResult res = this->netInterface->GetConnectionRealTimeStatus(this->connectionId, &status, 0, nullptr);
    if (res == k_EResultNoConnection)
    {
        this->ping = 0.0;
    }
    else
    {
        this->ping = (double)status.m_nPing / 1000.0;
    }

    this->PollIncomingMessages();
    this->PushPendingMessages();
}

//--------------------------------------------------------------------------
/**
*/
void
BaseMultiplayerClient::PollIncomingMessages()
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
        double recvTime = incomingMsg[i]->m_usecTimeReceived;
        if (data != nullptr && bytes > 0)
        {
            this->OnMessageReceived(incomingMsg[i]);
            incomingMsg[i]->Release();
            incomingMsg[i] = nullptr;
        }
    }
}

//--------------------------------------------------------------------------
/**
*/
void
BaseMultiplayerClient::PollConnectionChanges()
{
    callbackInstance = this;
    this->netInterface->RunCallbacks();
}

//--------------------------------------------------------------------------
/**
*/
void
BaseMultiplayerClient::PushPendingMessages()
{
    //SteamNetworkingMessage_t* netMsgs[4] = { nullptr, nullptr, nullptr, nullptr };
//
    /// TODO: do not make a new builder every time
    //flatbuffers::FlatBufferBuilder builder(1024);
    //flatbuffers::Offset<Protocol::MsgTest> msgTest;
    //flatbuffers::Offset<Protocol::Message> message;
    //
    //for (int i = 0; i < 4; i++)
    //{
    //    builder.Reset();
    //    static float f = 0.0f;
    //    f += 0.01f;
    //    msgTest = Protocol::CreateMsgTest(builder, f);
    //    message = Protocol::CreateMessage(builder, Protocol::Data_Test, msgTest.Union());
//
    //    builder.Finish(message);
    //    
    //    uint8_t* buf = builder.GetBufferPointer();
    //    int size = builder.GetSize();
    //    
    //    netMsgs[i] = SteamNetworkingUtils()->AllocateMessage(size);
    //    
    //    // TODO: Can we construct this inplace in the steamnetworkingmessage?
    //    Memory::Copy(buf, netMsgs[i]->m_pData, size);
    //    
    //    netMsgs[i]->m_conn = this->connectionId;
    //    netMsgs[i]->m_nFlags = 0;
    //}
    //
    //int64* outMessageNumberOrResult = nullptr;
    //this->netInterface->SendMessages(4, netMsgs, outMessageNumberOrResult);
}

} // namespace Multiplayer
