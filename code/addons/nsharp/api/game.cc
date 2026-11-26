//------------------------------------------------------------------------------
//  game.cc
//  (C) 2019 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "game.h"
#include "game/api.h"
#include "game/world.h"
#include "basegamefeature/components/basegamefeature.h"
#include "util/typepunning.h"
#include "basegamefeature/components/position.h"
#include "basegamefeature/components/orientation.h"
#include "basegamefeature/components/scale.h"

namespace Scripting
{
namespace Api
{

//------------------------------------------------------------------------------
/**
*/
bool
EntityIsValid(uint64_t entity)
{
    Game::Entity e = Game::Entity::FromId(entity);
    Game::World* world = Game::GetWorld(e.world);

    if (world == nullptr)
        return false;

    return world->IsValid(Game::Entity::FromId(entity));
}

//------------------------------------------------------------------------------
/**
    Whether the entity has an instance. IsValid only says the id is allocated, and an id can be held by
    something that has taken the instance out of the world, which is what an editor command does while it
    waits for a redo. A mirror entity that is only an id has nothing for a component access to read.
*/
bool
EntityHasInstance(uint64_t entity)
{
    Game::Entity e = Game::Entity::FromId(entity);
    Game::World* world = Game::GetWorld(e.world);

    if (world == nullptr)
        return false;

    return world->IsValid(e) && world->HasInstance(e);
}

//------------------------------------------------------------------------------
/**
*/
uint64_t
EntityCreate(uint32_t worldId)
{
    Game::World* world = Game::GetWorld(worldId);
    if (world == nullptr)
        return uint64_t(Game::Entity::Invalid());

    Game::Entity entity = world->CreateEntity(true);
    return uint64_t(entity);
}

//------------------------------------------------------------------------------
/**
*/
void
EntityDelete(uint64_t entity)
{
    Game::Entity e = Game::Entity::FromId(entity);
    Game::World* world = Game::GetWorld(e.world);
    if (world == nullptr)
        return;

    if (world->IsValid(e))
    {
        world->DeleteEntity(e);
    }
}

//------------------------------------------------------------------------------
/**
*/
bool
EntityHasComponent(uint64_t entity, uint32_t componentId)
{
    Game::Entity e = Game::Entity::FromId(entity);
    Game::World* world = Game::GetWorld(e.world);
    if (world == nullptr)
        return false;

    return world->HasComponent(e, componentId);
}

//------------------------------------------------------------------------------
/**
*/
Math::float4
EntityGetPosition(uint64_t entity)
{
    Game::Entity e = Game::Entity::FromId(entity);
    Game::World* world = Game::GetWorld(e.world);
    if (world == nullptr)
        return {0, 0, 0, 0};

    Math::vec3 const p = world->GetComponent<Game::Position>(e);
    return {p.x, p.y, p.z, 0};
}

//------------------------------------------------------------------------------
/**
*/
void
EntitySetPosition(uint64_t entity, Math::vec3 pos)
{
    Game::Entity e = Game::Entity::FromId(entity);
    Game::World* world = Game::GetWorld(e.world);
    if (world == nullptr)
        return;

    world->SetComponent<Game::Position>(e, pos);
}

//------------------------------------------------------------------------------
/**
*/
Math::float4
EntityGetOrientation(uint64_t entity)
{
    Game::Entity e = Game::Entity::FromId(entity);
    Game::World* world = Game::GetWorld(e.world);
    if (world == nullptr)
        return {0, 0, 0, 0};

    Math::quat const q = world->GetComponent<Game::Orientation>(e);
    return {q.x, q.y, q.z, q.w};
}

//------------------------------------------------------------------------------
/**
*/
void
EntitySetOrientation(uint64_t entity, Math::quat orientation)
{
    Game::Entity e = Game::Entity::FromId(entity);
    Game::World* world = Game::GetWorld(e.world);
    if (world == nullptr)
        return;

    world->SetComponent<Game::Orientation>(e, orientation);
}

//------------------------------------------------------------------------------
/**
*/
Math::float4
EntityGetScale(uint64_t entity)
{
    Game::Entity e = Game::Entity::FromId(entity);
    Game::World* world = Game::GetWorld(e.world);
    if (world == nullptr)
        return {0, 0, 0, 0};

    Math::vec3 const s = world->GetComponent<Game::Scale>(e);
    return {s.x, s.y, s.z, 0};
}

//------------------------------------------------------------------------------
/**
*/
void
EntitySetScale(uint64_t entity, Math::vec3 scale)
{
    Game::Entity e = Game::Entity::FromId(entity);
    Game::World* world = Game::GetWorld(e.world);
    if (world == nullptr)
        return;

    world->SetComponent<Game::Scale>(e, scale);
}

//------------------------------------------------------------------------------
/**
*/
uint32_t
ComponentGetId(const char* name)
{
    return (uint32_t)Game::GetComponentId(name).id;
}

//------------------------------------------------------------------------------
/**
*/
uint32_t
ComponentGetIdUtf8(const char* name, uint32_t length)
{
    Util::String componentName(name, length);
    return (uint32_t)Game::GetComponentId(componentName.AsCharPtr()).id;
}

//------------------------------------------------------------------------------
/**
*/
void
ComponentGetData(uint64_t entityId, uint32_t componentId, void* outData, int dataSize)
{
    Game::Entity entity = Game::Entity::FromId(entityId);
    Game::World* world = Game::GetWorld(entity.world);
    if (world == nullptr)
        return;

    n_assert_fmt(
        dataSize == MemDb::AttributeRegistry::TypeSize(componentId),
        "ComponentGetData: Provided component size (%i) bytes is not the correct size (%i) for the given ComponentId (%s).",
        dataSize,
        MemDb::AttributeRegistry::TypeSize(componentId),
        static_cast<Game::ComponentInterface*>(MemDb::AttributeRegistry::GetAttribute(componentId))->GetFullyQualifiedName()
    );

    // Read through the same accessor the write uses, so a component which is only staged is still found
    byte* const data = world->FindComponentData(entity, componentId);
#if NEBULA_DEBUG
    n_assert2(data != nullptr, "ComponentGetData: Entity does not have the component, staged or in a table!");
#endif
    if (data == nullptr)
        return;

    Memory::Copy(data, outData, dataSize);
}

//------------------------------------------------------------------------------
/**
*/
void
ComponentSetData(uint64_t entityId, uint32_t componentId, void* data, int dataSize)
{
    Game::Entity entity = Game::Entity::FromId(entityId);
    Game::World* world = Game::GetWorld(entity.world);
    if (world == nullptr)
        return;

    n_assert2(
        dataSize == MemDb::AttributeRegistry::TypeSize(componentId),
        "ComponentSetData: Provided component size in bytes is not the correct size for the given ComponentId."
    );

    world->SetComponentValue(entity, componentId, data, dataSize);
}

//------------------------------------------------------------------------------
/**
*/
uint32_t
WorldGetDefaultWorldId()
{
    Game::World* world = Game::GetWorld(WORLD_DEFAULT);
    if (world == nullptr)
        return 0;

    return world->GetWorldId();
}

//------------------------------------------------------------------------------
/**
*/
bool
WorldHasWorld(uint32_t worldId)
{
    return Game::GetWorld(worldId) != nullptr;
}

//------------------------------------------------------------------------------
/**
    Whether the world is simulating. A world that is not simulating is one the editor is only authoring,
    and nothing that runs gameplay should run against it
*/
bool
WorldIsSimulating(uint32_t worldId)
{
    Game::World* world = Game::GetWorld(worldId);
    if (world == nullptr)
        return false;

    return world->mode == Game::PlayMode;
}

//------------------------------------------------------------------------------
/**
    The layout the managed time source mirror is built from. The eight byte fields come first and then the
    four byte ones, which is the order C# declares as well and the only order that puts the entry at 64
    bytes without padding on either side. Ticks and the time factor are widened here, because Timing::Tick
    is 32 bit and a source that runs long enough wraps.
*/
struct TimeSourceSnapshot
{
    double frameTime;
    double time;
    int64_t ticks;
    double fixedFrameTime;
    double accumulator;
    double timeFactor;
    uint32_t hash;
    int32_t availableSteps;
    int32_t maxStepsPerFrame;
    int32_t pauseCounter;
};

static_assert(sizeof(TimeSourceSnapshot) == 64, "managed time source mirror layout");

//------------------------------------------------------------------------------
/**
*/
int
TimeSourceSync(void* buffer, int maxEntries)
{
    TimeSourceSnapshot* out = (TimeSourceSnapshot*)buffer;

    uint32_t num = Game::Time::GetNumTimeSources();
    if ((int)num > maxEntries)
        num = (uint32_t)maxEntries;

    for (uint32_t i = 0; i < num; i++)
    {
        Game::TimeSource* source = Game::Time::GetTimeSourceByIndex(i);

        out[i].frameTime = source->frameTime;
        out[i].time = source->time;
        out[i].ticks = int64_t(source->ticks);
        out[i].fixedFrameTime = source->fixedFrameTime;
        out[i].accumulator = source->accumulator;
        out[i].timeFactor = source->timeFactor;
        out[i].hash = source->hash;
        out[i].availableSteps = source->availableSteps;
        out[i].maxStepsPerFrame = source->maxStepsPerFrame;
        out[i].pauseCounter = source->pauseCounter;
    }

    return (int)num;
}

//------------------------------------------------------------------------------
/**
*/
void
TimeSourceSetTimeFactor(uint32_t hash, float factor)
{
    if (!Game::Time::HasTimeSource(hash))
        return;

    Game::Time::GetTimeSource(hash)->timeFactor = factor;
}

//------------------------------------------------------------------------------
/**
*/
void
TimeSourceSetPauseCounter(uint32_t hash, int counter)
{
    if (!Game::Time::HasTimeSource(hash))
        return;

    Game::Time::GetTimeSource(hash)->pauseCounter = counter;
}

//------------------------------------------------------------------------------
/**
*/
uint32_t
TimeSourceCreate(uint32_t hash, double fixedFrameTime, int maxStepsPerFrame)
{
    if (Game::Time::HasTimeSource(hash))
        return hash;

    Game::TimeSourceCreateInfo info;
    info.hash = hash;
    info.fixedFrameTime = fixedFrameTime;
    info.maxStepsPerFrame = maxStepsPerFrame;
    Game::Time::CreateTimeSource(info);
    return hash;
}

//------------------------------------------------------------------------------
/**
*/
void
TimeSetGlobalTimeFactor(float factor)
{
    Game::Time::SetGlobalTimeFactor(factor);
}

//------------------------------------------------------------------------------
/**
*/
float
TimeGetGlobalTimeFactor()
{
    return Game::Time::GetGlobalTimeFactor();
}

} // namespace Api
} // namespace Scripting
