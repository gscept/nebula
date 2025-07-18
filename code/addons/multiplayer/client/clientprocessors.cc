//------------------------------------------------------------------------------
//  @file clientprocessors.cc
//  @copyright (C) 2025 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "basegamefeature/managers/timemanager.h"
#include "components/multiplayer.h"
#include "foundation/stdneb.h"
#include "game/api.h"
#include "game/world.h"
#include "multiplayer/client/basemultiplayerclient.h"
#include "clientprocessors.h"
#include "nflatbuffer/nebula_flat.h"
#include "nflatbuffer/flatbufferinterface.h"
#include "flat/addons/multiplayer/protocol.h"

namespace Multiplayer
{

struct ClientProcessorContext
{
    BaseMultiplayerClient* client;
    Util::Array<Game::Processor*> processors;
    Game::TimeSource const* timeSource;
};

static ClientProcessorContext* context;
static Util::StringAtom frameEvent;

//--------------------------------------------------------------------------
/**
*/
void
InterpolatePositions(Game::World* world,
                     NetworkId const& netId,
                     NetworkTransform& s,
                     Game::Position& pos)
{
    s.positionExtrapolator.ReadValue(context->timeSource->time, pos);
}

//--------------------------------------------------------------------------
/**
*/
#define CREATE_NET_PROCESSOR(FUNC, ORDER) context->processors.Append(Game::ProcessorBuilder(world, #FUNC).On(frameEvent).Order(ORDER).Func(FUNC).Build());
void
SetupClientProcessors(BaseMultiplayerClient* client)
{
    Game::World* world = Game::GetWorld(WORLD_DEFAULT);
    frameEvent = "OnNetworkUpdate"_atm;

    context = new ClientProcessorContext();
    context->client = client;
    context->timeSource = Game::Time::GetTimeSource(TIMESOURCE_GAMEPLAY);

    CREATE_NET_PROCESSOR(InterpolatePositions, 100);
}
#undef CREATE_NET_PROCESSOR

//--------------------------------------------------------------------------
/**
*/
void
ShutdownClientProcessors()
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

} // namespace Multiplayer