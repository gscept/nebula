//------------------------------------------------------------------------------
//  @file serverprocessors.cc
//  @copyright (C) 2025 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "basegamefeature/managers/timemanager.h"
#include "components/multiplayer.h"
#include "foundation/stdneb.h"
#include "game/api.h"
#include "game/world.h"
#include "multiplayer/server/basemultiplayerserver.h"
#include "serverprocessors.h"
#include "nflatbuffer/nebula_flat.h"
#include "nflatbuffer/flatbufferinterface.h"
#include "flat/addons/multiplayer/standardprotocol.h"

namespace Multiplayer
{

struct ServerProcessorContext
{
    BaseMultiplayerServer* server;
    Util::Array<Game::Processor*> processors;
    Game::TimeSource const* timeSource;
    flatbuffers::FlatBufferBuilder builder = flatbuffers::FlatBufferBuilder(4_KB);
};

static ServerProcessorContext* context;
static Util::StringAtom frameEvent;

//--------------------------------------------------------------------------
/**
*/
void
SyncPositions(Game::World* world,
              NetworkId const& netId,
              NetworkTransform& netTransform,
              Game::Position& pos)
{
    context->builder.Clear();
    flatbuffers::Offset<StandardProtocol::MsgSyncPosition> msgPos;
    flatbuffers::Offset<StandardProtocol::Message> message;
    
    netTransform.tickNumber++;
    // Reusing position extrapolators last packet pos, to save some memory.
    Math::vec3 instantVelocity = (pos - netTransform.positionExtrapolator.lastPacketPos) * (1.0f / context->server->GetTickInterval());
    netTransform.positionExtrapolator.lastPacketPos = pos;

    msgPos = StandardProtocol::CreateMsgSyncPosition(context->builder, netId.identifier, (Flat::Vec3*)&pos, (Flat::Vec3*)&instantVelocity, netTransform.tickNumber);
    message = StandardProtocol::CreateMessage(context->builder, StandardProtocol::MessageData_SyncPosition, msgPos.Union());
    
    context->builder.Finish(message);
    
    uint8_t* buf = context->builder.GetBufferPointer();
    int size = context->builder.GetSize();

    context->server->Broadcast(buf, size);
}

//--------------------------------------------------------------------------
/**
*/
#define CREATE_NET_PROCESSOR(FUNC, ORDER) context->processors.Append(Game::ProcessorBuilder(world, #FUNC).On(frameEvent).Order(ORDER).Func(FUNC).Build()); context->processors.Back()->active = false;
void
SetupServerProcessors(BaseMultiplayerServer* server)
{
    Game::World* world = Game::GetWorld(WORLD_DEFAULT);
    frameEvent = "OnNetworkUpdate"_atm;

    context = new ServerProcessorContext();
    context->server = server;
    context->timeSource = Game::Time::GetTimeSource(TIMESOURCE_GAMEPLAY);
    

    CREATE_NET_PROCESSOR(SyncPositions, 100);
}
#undef CREATE_NET_PROCESSOR

//--------------------------------------------------------------------------
/**
*/
void
ShutdownServerProcessors()
{
    Game::World* world = Game::GetWorld(WORLD_DEFAULT);
    Game::FramePipeline& pipeline = world->GetFramePipeline();
    Game::FrameEvent* event = pipeline.GetFrameEvent(frameEvent);
    for (int i = 0; i < context->processors.Size(); i++)
    {
        event->RemoveProcessor(context->processors[i]);
    }
    delete context;
}

//--------------------------------------------------------------------------
/**
*/
void
SetServerProcessorsActive(bool active)
{
    for (int i = 0; i < context->processors.Size(); i++)
    {
        context->processors[i]->active = active;
    }
}

} // namespace Multiplayer