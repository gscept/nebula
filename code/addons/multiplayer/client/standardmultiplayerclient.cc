//------------------------------------------------------------------------------
//  @file standardmultiplayerclient.cc
//  @copyright (C) 2025 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "basegamefeature/managers/timemanager.h"
#include "foundation/stdneb.h"
#include "standardmultiplayerclient.h"
#include "components/multiplayer.h"
#include "game/api.h"
#include "game/world.h"
#include "memdb/database.h"
#include "multiplayer/client/clientprocessors.h"
#include "multiplayer/server/clientconnection.h"
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
StandardMultiplayerClient::Open()
{
    SetupClientProcessors(this);
    return BaseMultiplayerClient::Open();
}

//--------------------------------------------------------------------------
/**
*/
void
StandardMultiplayerClient::Close()
{
    ShutdownClientProcessors();
    BaseMultiplayerClient::Close();
}

//--------------------------------------------------------------------------
/**
*/
void
StandardMultiplayerClient::OnIsConnecting()
{

}

//--------------------------------------------------------------------------
/**
*/
void
StandardMultiplayerClient::OnConnected()
{

}

//--------------------------------------------------------------------------
/**
*/
void
StandardMultiplayerClient::OnDisconnected()
{

}

//--------------------------------------------------------------------------
/**
*/
Game::Entity
UnpackEntity(Game::World* world, Util::Blob const& data)
{
    size_t bytesRead = 0;
    byte const* const basePtr = (byte*)data.GetPtr();
    size_t const numBytes = data.Size();
    
    Util::FixedArray<MemDb::AttributeId> attributes = Util::FixedArray<MemDb::AttributeId>(20);

    byte const* ptr = basePtr;
    int numAttributes = 0;
    while ((size_t)(ptr - basePtr) < numBytes)
    {
        MemDb::AttributeId const attribute = *reinterpret_cast<MemDb::AttributeId const*>(ptr);
        ptr += sizeof(MemDb::AttributeId);
        SizeT const typeSize = MemDb::AttributeRegistry::TypeSize(attribute);
        ptr += typeSize;
        attributes[numAttributes++] = attribute;
    }

    attributes.Resize(numAttributes);

    Game::EntityTableCreateInfo info;
    info.components = attributes;
    info.name = "net_table";
    MemDb::TableId tableId = world->CreateEntityTable(info);
    
    Game::Entity entity = world->AllocateEntityId();
    world->AllocateInstance(entity, tableId, &data);
    return entity;
}

//--------------------------------------------------------------------------
/**
*/
void
StandardMultiplayerClient::OnMessageReceived(SteamNetworkingMessage_t* msg)
{
    void const* data = msg->GetData();
    size_t size = msg->GetSize();
    
    Game::World* world = Game::GetWorld(WORLD_DEFAULT);
    Game::TimeSource const* const timeSource = Game::Time::GetTimeSource(TIMESOURCE_GAMEPLAY);

    StandardProtocol::Message const* protocolMessage = StandardProtocol::GetMessage(data);

    switch (protocolMessage->data_type())   
    {
        case StandardProtocol::MessageData::MessageData_SyncPosition:
        {
            StandardProtocol::MsgSyncPosition const* syncPosMsg = protocolMessage->data_as_SyncPosition();

            IndexT hashIndex = this->networkEntities.FindIndex(syncPosMsg->network_id());
            if (hashIndex == InvalidIndex)
                break;

            Game::Entity entity = this->networkEntities.ValueAtIndex(syncPosMsg->network_id(), hashIndex);
            NetworkTransform netTransform = world->GetComponent<NetworkTransform>(entity);
            Math::vec3 newPos = { syncPosMsg->pos()->x(), syncPosMsg->pos()->y(), syncPosMsg->pos()->z() };
            Math::vec3 newVel = { syncPosMsg->vel()->x(), syncPosMsg->vel()->y(), syncPosMsg->vel()->z() };
            if (syncPosMsg->tick() > netTransform.tickNumber)
            {
                netTransform.tickNumber = syncPosMsg->tick();
                netTransform.positionExtrapolator.AddSample(timeSource->time - (this->GetCurrentPing() / 2.0), timeSource->time, newPos, newVel);
            }
            world->SetComponent<NetworkTransform>(entity, netTransform);
            break;
        }
        case StandardProtocol::MessageData::MessageData_ReplicateObject:
        {
            n_printf("Got message: ReplicateObject\n");
            StandardProtocol::MsgReplicateObject const* repObjMsg = protocolMessage->data_as_ReplicateObject();
            Util::Blob blob = Util::Blob(repObjMsg->blob()->data(), repObjMsg->blob()->size());
            Game::Entity ent = UnpackEntity(world, blob);
            Multiplayer::NetworkId netId = world->GetComponent<Multiplayer::NetworkId>(ent);
            this->networkEntities.Add(netId.identifier, ent);

            NetworkTransform trs = world->GetComponent<NetworkTransform>(ent);
            trs.positionExtrapolator.Reset(timeSource->time, timeSource->time, world->GetComponent<Game::Position>(ent));
            n_printf("eid = %i, netid = %i\n", ent.HashCode(), netId.identifier);
            break;
        }
    }
}

} // namespace Multiplayer