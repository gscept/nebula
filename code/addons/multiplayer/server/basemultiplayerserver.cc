//------------------------------------------------------------------------------
//  basemultiplayerserver.cc
//  (C) 2025 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "basemultiplayerserver.h"
#include "components/multiplayer.h"
#include "core/debug.h"
#include "GameNetworkingSockets/steam/steamnetworkingsockets.h"
#include "game/api.h"
#include "game/filter.h"
#include "game/world.h"
#include "memdb/database.h"
#include "nflatbuffer/nebula_flat.h"
#include "nflatbuffer/flatbufferinterface.h"
#include "flat/addons/multiplayer/protocol.h"
#include "flatbuffers/buffer.h"
#include "flatbuffers/flatbuffer_builder.h"
#include "steam/isteamnetworkingutils.h"
#include "multiplayer/multiplayerfeatureunit.h"
#include "serverprocessors.h"

namespace Multiplayer
{

using namespace Util;

//--------------------------------------------------------------------------
/**
*/
// HACK: MP server should probably be a singleton
static BaseMultiplayerServer* callbackServerInstance = nullptr;
static void
SvSteamNetConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t* info)
{
    callbackServerInstance->OnNetConnectionStatusChanged(info);
}

//------------------------------------------------------------------------------
/**
*/
BaseMultiplayerServer::BaseMultiplayerServer()
    : isOpen(false),
    sendTickInterval(0.1)
{
    for (int i = 0; i < (int)ClientGroup::NumClientGroups; i++)
    {
        this->pollGroupIntervals[i] = 1.0/60.0;
        this->pollGroupTimers[i].Start();
    }
}

//------------------------------------------------------------------------------
/**
*/
BaseMultiplayerServer::~BaseMultiplayerServer()
{
    n_assert(!this->IsOpen());
}

//------------------------------------------------------------------------------
/**
*/
bool
BaseMultiplayerServer::Open()
{
    n_assert(!this->isOpen);
    n_assert(this->clientConnections.IsEmpty());

    const int NEBULA_DEFAULT_PORT = 61111;
    this->netInterface = SteamNetworkingSockets();

    if (this->netInterface == nullptr)
    {
        n_error("Failed to initialize SteamNetworkingSockets!\n");
        return false;
    }

#ifndef PUBLIC_BUILD
    if (false) // poor connection
    {
        SteamNetworkingUtils()->SetGlobalConfigValueInt32(k_ESteamNetworkingConfig_FakePacketLag_Send, 100);
        SteamNetworkingUtils()->SetGlobalConfigValueInt32(k_ESteamNetworkingConfig_FakePacketLag_Recv, 100);
        SteamNetworkingUtils()->SetGlobalConfigValueFloat(k_ESteamNetworkingConfig_FakePacketLoss_Send, 2);
        SteamNetworkingUtils()->SetGlobalConfigValueFloat(k_ESteamNetworkingConfig_FakePacketJitter_Send_Avg, 30);
        SteamNetworkingUtils()->SetGlobalConfigValueFloat(k_ESteamNetworkingConfig_FakePacketJitter_Send_Pct, 100);
        SteamNetworkingUtils()->SetGlobalConfigValueFloat(k_ESteamNetworkingConfig_FakePacketJitter_Send_Max, 100);
        SteamNetworkingUtils()->SetGlobalConfigValueFloat(k_ESteamNetworkingConfig_FakePacketReorder_Send, 10);
    }
    else // good connection
    {
        SteamNetworkingUtils()->SetGlobalConfigValueInt32(k_ESteamNetworkingConfig_FakePacketLag_Send, 25);
        SteamNetworkingUtils()->SetGlobalConfigValueInt32(k_ESteamNetworkingConfig_FakePacketLag_Recv, 25);
        SteamNetworkingUtils()->SetGlobalConfigValueFloat(k_ESteamNetworkingConfig_FakePacketLoss_Send, 0.5);
        SteamNetworkingUtils()->SetGlobalConfigValueFloat(k_ESteamNetworkingConfig_FakePacketJitter_Send_Avg, 5);
        SteamNetworkingUtils()->SetGlobalConfigValueFloat(k_ESteamNetworkingConfig_FakePacketJitter_Send_Pct, 100);
        SteamNetworkingUtils()->SetGlobalConfigValueFloat(k_ESteamNetworkingConfig_FakePacketJitter_Send_Max, 30);
        SteamNetworkingUtils()->SetGlobalConfigValueFloat(k_ESteamNetworkingConfig_FakePacketReorder_Send, 0.5);
    }
#endif

    SteamNetworkingIPAddr serverLocalAddr;
    serverLocalAddr.Clear();
    serverLocalAddr.m_port = NEBULA_DEFAULT_PORT;
    
    SteamNetworkingConfigValue_t opt;
    // setup callback for changes in connections
    opt.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, (void*)SvSteamNetConnectionStatusChangedCallback);
    
    this->listenSock = this->netInterface->CreateListenSocketIP(serverLocalAddr, 1, &opt);
    if (this->listenSock == k_HSteamListenSocket_Invalid)
    {
        n_error("Failed to listen on port %d\n", serverLocalAddr.m_port);
        return false;
    }

    for (int i = 0; i < (int)ClientGroup::NumClientGroups; i++)
    {
        this->pollGroups[i] = this->netInterface->CreatePollGroup();
        if (this->pollGroups[i] == k_HSteamNetPollGroup_Invalid)
        {
            n_error("Failed to listen on port %d\n", serverLocalAddr.m_port);
            return false;
        }
    }   
    n_printf("MultiplayerServer listening on port %d\n", serverLocalAddr.m_port);
    
    SetupServerProcessors(this);

    this->sendTimer.Start();
    this->isOpen = true;
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
BaseMultiplayerServer::Close()
{
    n_assert(this->isOpen);
    // disconnect client connections
    IndexT clientIndex;
    for (clientIndex = 0; clientIndex < this->clientConnections.Size(); clientIndex++)
    {
        ClientConnection* curConnection = this->clientConnections[clientIndex];
        curConnection->Shutdown();
    }
    this->clientConnections.Clear();

    this->netInterface->CloseListenSocket(this->listenSock);
    this->listenSock = k_HSteamListenSocket_Invalid;

    for (int i = 0; i < (int)ClientGroup::NumClientGroups; i++)
    {
        this->netInterface->DestroyPollGroup(this->pollGroups[i]);
        this->pollGroups[i] = k_HSteamNetPollGroup_Invalid;
    }
    this->isOpen = false;
}

//------------------------------------------------------------------------------
/**
*/
void
BaseMultiplayerServer::AddClientConnection(ClientConnection* conn)
{
    n_assert(conn != nullptr);
    this->clientConnections.Add(conn->GetConnectionId(), conn);
}

//--------------------------------------------------------------------------
/**
*/
void
BaseMultiplayerServer::Broadcast(void* buf, int size)
{
    const int numClients = this->clientConnections.Size();
    if (numClients == 0)
        return;

    ISteamNetworkingMessage** netMsgs = new ISteamNetworkingMessage*[numClients];
    int i;
    for (i = 0; i < numClients; i++)
    {
        netMsgs[i] = SteamNetworkingUtils()->AllocateMessage(size);
    }
        
    auto it = this->clientConnections.Begin();
    i = 0;
    while (it != this->clientConnections.End())
    {
        // TODO: Can we construct this inplace in the steamnetworkingmessage?
        Memory::Copy(buf, netMsgs[i]->m_pData, size);

        netMsgs[i]->m_conn = (*(this->clientConnections.Begin().val))->GetConnectionId();
        // HACK: Unreliable for now
        netMsgs[i]->m_nFlags = 0; 
        i++;
        it++;
    }
    
    int64* outMessageNumberOrResult = nullptr;
    this->netInterface->SendMessages(numClients, netMsgs, outMessageNumberOrResult);
}

//--------------------------------------------------------------------------
/**
*/
bool
BaseMultiplayerServer::OnClientIsConnecting(ClientConnection* connection)
{
    // Override in subclass
    return false;
}

//--------------------------------------------------------------------------
/**
*/
void
BaseMultiplayerServer::OnClientConnected(ClientConnection* connection)
{
    // Override in subclass
}

//--------------------------------------------------------------------------
/**
*/
void
BaseMultiplayerServer::OnClientDisconnected(ClientConnection* connection)
{
    // Override in subclass
}

//--------------------------------------------------------------------------
/**
*/
void
BaseMultiplayerServer::OnMessageReceived(Timing::Time recvTime, uint32_t connectionId, byte* data, size_t size)
{
    // Override in subclass
}

//--------------------------------------------------------------------------
/**
*/
void
BaseMultiplayerServer::OnNetConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* info)
{
    switch (info->m_info.m_eState)
    {
        case k_ESteamNetworkingConnectionState_None:
            // NOTE: We will get callbacks here when we destroy connections. We can ignore these.
            break;

        case k_ESteamNetworkingConnectionState_ClosedByPeer:
        case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
        {
            // Ignore if they were not previously connected. (If they disconnected
            // before we accepted the connection.)
            if (info->m_eOldState == k_ESteamNetworkingConnectionState_Connected)
            {
                // Locate the client.  Note that it should have been found, because this
                // is the only codepath where we remove clients (except on shutdown),
                // and connection change callbacks are dispatched in queue order.
                IndexT clientIndex = this->clientConnections.FindIndex(info->m_hConn);
                n_assert(clientIndex != InvalidIndex);

                ClientConnection* connection = this->clientConnections.ValueAtIndex(info->m_hConn, clientIndex);

                // Select appropriate log messages
                const char* pszDebugLogAction;
                if (info->m_info.m_eState == k_ESteamNetworkingConnectionState_ProblemDetectedLocally)
                {
                    pszDebugLogAction = "problem detected locally";
                }
                else
                {
                    // Note that here we could check the reason code to see if
                    // it was a "usual" connection or an "unusual" one.
                    pszDebugLogAction = "closed by peer";
                }

                // Spew something to our own log.  Note that because we put their nick
                // as the connection description, it will show up, along with their
                // transport-specific data (e.g. their IP address)
                n_printf(
                    "Connection %s %s, reason %d: %s\n",
                    info->m_info.m_szConnectionDescription,
                    pszDebugLogAction,
                    info->m_info.m_eEndReason,
                    info->m_info.m_szEndDebug
                );

                this->OnClientDisconnected(this->clientConnections[info->m_hConn]);
                
                this->clientConnections.EraseIndex(info->m_hConn, clientIndex);

                // Send a message so everybody else knows what happened?
                //Broadcast(temp);
            }
            else
            {
                n_assert(info->m_eOldState == k_ESteamNetworkingConnectionState_Connecting);
            }

            // Clean up the connection.  This is important!
            // The connection is "closed" in the network sense, but
            // it has not been destroyed.  We must close it on our end, too
            // to finish up.  The reason information do not matter in this case,
            // and we cannot linger because it's already closed on the other end,
            // so we just pass 0's.
            this->netInterface->CloseConnection(info->m_hConn, 0, nullptr, false);
            break;
        }

        case k_ESteamNetworkingConnectionState_Connecting:
        {
            // This must be a new connection
            n_assert(this->clientConnections.FindIndex(info->m_hConn) == InvalidIndex);

            n_printf("Connection request from %s.", info->m_info.m_szConnectionDescription);

            // A client is attempting to connect
            // Try to accept the connection.
            if (this->netInterface->AcceptConnection(info->m_hConn) != k_EResultOK)
            {
                // This could fail.  If the remote host tried to connect, but then
                // disconnected, the connection may already be half closed.  Just
                // destroy whatever we have on our side.
                this->netInterface->CloseConnection(info->m_hConn, 0, nullptr, false);
                n_printf("Can't accept connection. (It was already closed?)\n");
                break;
            }

            
            ClientConnection* connection = new ClientConnection();
            connection->Initialize(this, info->m_hConn);
            
            bool accepted = this->OnClientIsConnecting(connection);

            if (!accepted)
            {
                n_printf("Connection %s denied by protocol.\n", info->m_info.m_szConnectionDescription);
                delete connection;
                break;
            }

            // Assign the poll group
            if (!this->netInterface->SetConnectionPollGroup(info->m_hConn, this->pollGroups[(int)connection->GetClientGroup()]))
            {
                this->netInterface->CloseConnection(info->m_hConn, 0, nullptr, false);
                delete connection;
                n_printf("Failed to set poll group?\n");
                break;
            }

            this->AddClientConnection(connection);
            n_printf("Accepted connection from %s.\n", info->m_info.m_szConnectionDescription);

            // Let everybody else know there's a new player in game?
            //Broadcast(temp, info->m_hConn);
            break;
        }

        case k_ESteamNetworkingConnectionState_Connected:
            this->OnClientConnected(this->clientConnections[info->m_hConn]);
            break;

        default:
            // Silences -Wswitch
            break;
    }
}

//--------------------------------------------------------------------------
/**
*/
void
BaseMultiplayerServer::SyncAll()
{
    this->PollIncomingMessages();
    this->PollConnectionChanges();
    this->PushPendingMessages();
}

//--------------------------------------------------------------------------
/**
*/
void
BaseMultiplayerServer::PollIncomingMessages()
{
    for (int i = 0; i < (int)ClientGroup::NumClientGroups; i++)
    {
        if (this->pollGroupTimers[i].GetTime() < this->pollGroupIntervals[i])
        {
            continue;
        }

        this->pollGroupTimers[i].Reset();

        // TODO: Don't allocate this per frame, just reuse the old buffer.
        ISteamNetworkingMessage** incomingMsg = new ISteamNetworkingMessage*[this->maxMessagesPerFrame];
        int numMsgs = this->netInterface->ReceiveMessagesOnPollGroup(this->pollGroups[i], incomingMsg, this->maxMessagesPerFrame);
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
            IndexT clientIndex = this->clientConnections.FindIndex(incomingMsg[i]->m_conn);
            n_assert(clientIndex != InvalidIndex);
            void* data = incomingMsg[i]->m_pData;
            int bytes = incomingMsg[i]->m_cbSize;
            double recvTime = incomingMsg[i]->m_usecTimeReceived;


            if (data != nullptr && bytes > 0)
            {
                this->OnMessageReceived(recvTime, incomingMsg[i]->m_conn, (byte*)data, bytes);
                incomingMsg[i]->Release();
            }
        }
        delete[] incomingMsg;
    }
}

//--------------------------------------------------------------------------
/**
*/
void
BaseMultiplayerServer::PollConnectionChanges()
{
    callbackServerInstance = this;
    this->netInterface->RunCallbacks();
}

//--------------------------------------------------------------------------
/**
*/
void
BaseMultiplayerServer::PushPendingMessages()
{
    if (this->clientConnections.IsEmpty())
        return;

    if (this->sendTimer.GetTime() < this->sendTickInterval)
    {
        SetServerProcessorsActive(false);
        return;
    }

    this->sendTimer.Reset();

    SetServerProcessorsActive(true);

    //SteamNetworkingMessage_t* netMsgs[4] = { nullptr, nullptr, nullptr, nullptr };
//
    //// TODO: do not make a new builder every time
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
    //    netMsgs[i]->m_conn = (*(this->clientConnections.Begin().val))->connectionId;
    //    netMsgs[i]->m_nFlags = 0;
    //}
    //
    //int64* outMessageNumberOrResult = nullptr;
    //this->netInterface->SendMessages(4, netMsgs, outMessageNumberOrResult);
}

} // namespace Multiplayer
