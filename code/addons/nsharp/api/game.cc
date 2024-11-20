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
*/
uint64_t
EntityCreateFromTemplate(uint32_t worldId, const char* tmpl)
{
    Game::World* world = Game::GetWorld(worldId);
    Game::EntityCreateInfo info;
    info.immediate = true;
    info.templateId = Game::GetTemplateId(tmpl);
    Game::Entity entity = world->CreateEntity(info);
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
    world->DeleteEntity(e);
}

//------------------------------------------------------------------------------
/**
*/
bool
EntityHasComponent(uint64_t entity, uint32_t componentId)
{
    Game::Entity e = Game::Entity::FromId(entity);
    Game::World* world = Game::GetWorld(e.world);
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
void
ComponentGetData(uint64_t entityId, uint32_t componentId, void* outData, int dataSize)
{
    Game::Entity entity = Game::Entity::FromId(entityId);
    Game::World* world = Game::GetWorld(entity.world);

    n_assert2(
        dataSize == MemDb::AttributeRegistry::TypeSize(componentId),
        "ComponentGetData: Provided component size in bytes is not the correct size for the given ComponentId."
    );

    Game::EntityMapping mapping = world->GetEntityMapping(entity);
    byte* ptr = (byte*)world->GetInstanceBuffer(mapping.table, mapping.instance.partition, componentId);
    ptr += (mapping.instance.index * dataSize);
    Memory::Copy(ptr, outData, dataSize);
}

//------------------------------------------------------------------------------
/**
*/
void
ComponentSetData(uint64_t entityId, uint32_t componentId, void* data, int dataSize)
{
    Game::Entity entity = Game::Entity::FromId(entityId);
    Game::World* world = Game::GetWorld(entity.world);

    n_assert2(
        dataSize == MemDb::AttributeRegistry::TypeSize(componentId),
        "ComponentSetData: Provided component size in bytes is not the correct size for the given ComponentId."
    );

    Game::EntityMapping mapping = world->GetEntityMapping(entity);
    byte* ptr = (byte*)world->GetInstanceBuffer(mapping.table, mapping.instance.partition, componentId);
    ptr += (mapping.instance.index * dataSize);
    Memory::Copy(data, ptr, dataSize);
}

//------------------------------------------------------------------------------
/**
*/
uint32_t
WorldGetDefaultWorldId()
{
    return Game::GetWorld(WORLD_DEFAULT)->GetWorldId();
}

} // namespace Api
} // namespace Scripting
