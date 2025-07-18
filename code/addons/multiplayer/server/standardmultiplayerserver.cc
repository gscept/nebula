//------------------------------------------------------------------------------
//  @file standardmultiplayerserver.cc
//  @copyright (C) 2025 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "standardmultiplayerserver.h"
#include "components/multiplayer.h"
#include "game/api.h"
#include "game/world.h"
#include "memdb/database.h"
#include "multiplayer/server/clientconnection.h"
#include "multiplayer/server/serverprocessors.h"
#include "nflatbuffer/nebula_flat.h"
#include "nflatbuffer/flatbufferinterface.h"
#include "flat/addons/multiplayer/standardprotocol.h"
#include "steam/isteamnetworkingutils.h"
#include "steam/steamnetworkingtypes.h"
#include "steam/isteamnetworkingsockets.h"

namespace Multiplayer
{

//--------------------------------------------------------------------------
/**
*/
bool
StandardMultiplayerServer::Open()
{
    SetupServerProcessors(this);
    return BaseMultiplayerServer::Open();
}

//--------------------------------------------------------------------------
/**
*/
void
StandardMultiplayerServer::Close()
{
    ShutdownServerProcessors();
    BaseMultiplayerServer::Close();
}

//--------------------------------------------------------------------------
/**
*/
bool 
StandardMultiplayerServer::OnClientIsConnecting(ClientConnection* connection)
{
    return true; // default is accepting all incoming connections
}

//--------------------------------------------------------------------------
/**
*/
void 
StandardMultiplayerServer::OnClientConnected(ClientConnection* client)
{
    // TEMP: Replicate object on client
    flatbuffers::FlatBufferBuilder builder(1024);
    
    Game::World* world = Game::GetWorld(WORLD_DEFAULT);

    Game::Filter filter = Game::FilterBuilder()
        .Including<Game::Entity, NetworkId>()
        .Build();
    Game::Dataset data = world->Query(filter);

    int numMsgs = 0;
    for (int v = 0; v < data.numViews; v++)
    {
        numMsgs += data.views[v].numInstances;
    }

    if (numMsgs > 0)
    {
        ISteamNetworkingMessage** outgoingMsgs = new ISteamNetworkingMessage*[numMsgs];

        for (int v = 0; v < data.numViews; v++)
        {
            Game::Dataset::View const& view = data.views[v];
            Game::Entity const* const entities = (Game::Entity*)view.buffers[0];
            NetworkId const* const netIds = (NetworkId*)view.buffers[1];
            
            MemDb::Table const& table = world->GetDatabase()->GetTable(view.tableId);
            
            for (IndexT i = 0; i < view.numInstances; ++i)
            {
                builder.Clear();

                Game::EntityMapping const& mapping = world->GetEntityMapping(entities[0]);
                Util::Blob blob = table.SerializeInstance(mapping.instance);

                flatbuffers::Offset<StandardProtocol::MsgReplicateObject> msgRep;
                flatbuffers::Offset<StandardProtocol::Message> message;
                auto vector_bytes = builder.CreateVector((ubyte*)blob.GetPtr(), blob.Size());
                msgRep = StandardProtocol::CreateMsgReplicateObject(builder, vector_bytes);
                message = StandardProtocol::CreateMessage(builder, StandardProtocol::MessageData_ReplicateObject, msgRep.Union());
                
                builder.Finish(message);
                
                uint8_t* buf = builder.GetBufferPointer();
                int size = builder.GetSize();
    
                outgoingMsgs[i] = SteamNetworkingUtils()->AllocateMessage(size);
                
                // TODO: Can we construct this inplace in the steamnetworkingmessage?
                Memory::Copy(buf, outgoingMsgs[i]->m_pData, size);
                
                outgoingMsgs[i]->m_conn = client->GetConnectionId();
                outgoingMsgs[i]->m_nFlags = k_nSteamNetworkingSend_Reliable;
            }
        }
        int64* outMessageNumberOrResult = nullptr;
        this->netInterface->SendMessages(numMsgs, outgoingMsgs, outMessageNumberOrResult);
        delete[] outgoingMsgs;
    }
    Game::DestroyFilter(filter);
}

//--------------------------------------------------------------------------
/**
*/
void 
StandardMultiplayerServer::OnClientDisconnected(ClientConnection* connection)
{

}

//--------------------------------------------------------------------------
/**
*/
void 
StandardMultiplayerServer::OnMessageReceived(Timing::Time recvTime, uint32_t connectionId, byte* data, size_t size)
{
    StandardProtocol::Message const* protocolMessage = StandardProtocol::GetMessage(data);

    switch (protocolMessage->data_type())
    {
        //case StandardProtocol::Data::Data_Test:
        //{
        //    StandardProtocol::MsgTest const* test = protocolMessage->data_as_Test();
        //    MultiplayerFeatureUnit::Instance()->temp_f = test->value();
        //    n_printf("Got message: %f\n", test->value());
        //    break;
        //}
        //case StandardProtocol::Data::Data_Connected:
        //{
        //    n_printf("Got message: CONNECTED\n");
        //    break;
        //}
    }
}

} // namespace Multiplayer