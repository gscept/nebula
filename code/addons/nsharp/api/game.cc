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
EntityIsValid(uint32_t worldId, uint32_t entity)
{
    Game::World* world = Game::GetWorld(worldId);
    return world->IsValid(Game::Entity::FromId(entity));
}

//------------------------------------------------------------------------------
/**
*/
uint32_t
EntityCreateFromTemplate(uint32_t worldId, const char* tmpl)
{
    Game::World* world = Game::GetWorld(worldId);
    Game::EntityCreateInfo info;
    info.immediate = true;
    info.templateId = Game::GetTemplateId(tmpl);
    Game::Entity entity = world->CreateEntity(info);
    return uint32_t(entity);
}

//------------------------------------------------------------------------------
/**
*/
void
EntityDelete(uint32_t worldId, uint32_t entity)
{
    Game::World* world = Game::GetWorld(worldId);
    world->DeleteEntity(Game::Entity::FromId(entity));
}

//------------------------------------------------------------------------------
/**
*/
bool
EntityHasComponent(uint32_t worldId, uint32_t entity, uint32_t componentId)
{
    Game::World* world = Game::GetWorld(worldId);
    return world->HasComponent(Game::Entity::FromId(entity), componentId);
}

//------------------------------------------------------------------------------
/**
*/
Math::vec3
EntityGetPosition(uint32_t worldId, uint32_t entity)
{
    Game::World* world = Game::GetWorld(worldId);
    Math::vec3 const p = world->GetComponent<Game::Position>(Game::Entity::FromId(entity));
    return p;
}

//------------------------------------------------------------------------------
/**
*/
void
EntitySetPosition(uint32_t worldId, uint32_t entity, Math::vec3 pos)
{
    Game::World* world = Game::GetWorld(worldId);
    world->SetComponent<Game::Position>(Game::Entity::FromId(entity), pos);
}

//------------------------------------------------------------------------------
/**
*/
Math::quat
EntityGetOrientation(uint32_t worldId, uint32_t entity)
{
    Game::World* world = Game::GetWorld(worldId);
    Math::quat const q = world->GetComponent<Game::Orientation>(Game::Entity::FromId(entity));
    return q;
}

//------------------------------------------------------------------------------
/**
*/
void
EntitySetOrientation(uint32_t worldId, uint32_t entity, Math::quat orientation)
{
    Game::World* world = Game::GetWorld(worldId);
    world->SetComponent<Game::Orientation>(Game::Entity::FromId(entity), orientation);
}

//------------------------------------------------------------------------------
/**
*/
Math::vec3
EntityGetScale(uint32_t worldId, uint32_t entity)
{
    Game::World* world = Game::GetWorld(worldId);
    Math::vec3 const s = world->GetComponent<Game::Scale>(Game::Entity::FromId(entity));
    return s;
}

//------------------------------------------------------------------------------
/**
*/
void
EntitySetScale(uint32_t worldId, uint32_t entity, Math::vec3 scale)
{
    Game::World* world = Game::GetWorld(worldId);
    world->SetComponent<Game::Scale>(Game::Entity::FromId(entity), scale);
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
ComponentGetData(uint32_t worldId, uint32_t eId, uint32_t componentId, void* outData, int dataSize)
{
    Game::Entity entity = Game::Entity::FromId(eId);
    Game::World* world = Game::GetWorld(worldId);

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
ComponentSetData(uint32_t worldId, uint32_t eId, uint32_t componentId, void* data, int dataSize)
{
    Game::Entity entity = Game::Entity::FromId(eId);
    Game::World* world = Game::GetWorld(worldId);

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
    return WORLD_DEFAULT;
}

} // namespace Api
} // namespace Scripting
