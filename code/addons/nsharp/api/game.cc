//------------------------------------------------------------------------------
//  game.cc
//  (C) 2019 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "game.h"
#include "game/api.h"
#include "game/world.h"
#include "basegamefeature/components/transform.h"

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
    return Game::IsValid(world, Game::Entity::FromId(entity));
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
    Game::Entity entity = Game::CreateEntity(world, info);
    return uint32_t(entity);
}

//------------------------------------------------------------------------------
/**
*/
void
EntityDelete(uint32_t worldId, uint32_t entity)
{
    Game::World* world = Game::GetWorld(worldId);
    Game::DeleteEntity(world, Game::Entity::FromId(entity));
    return;
}

//------------------------------------------------------------------------------
/**
*/
bool
EntityHasComponent(uint32_t worldId, uint32_t entity, uint32_t componentId)
{
    Game::World* world = Game::GetWorld(worldId);
    return Game::HasComponent(world, Game::Entity::FromId(entity), componentId);
}

//------------------------------------------------------------------------------
/**
*/
Math::mat4
EntityGetTransform(uint32_t worldId, uint32_t entity)
{
    Game::World* world = Game::GetWorld(worldId);
    return Game::GetComponent<Game::WorldTransform>(world, Game::Entity::FromId(entity)).value;
}

//------------------------------------------------------------------------------
/**
*/
void
EntitySetTransform(uint32_t worldId, uint32_t entity, Math::mat4 transform)
{
    Game::World* world = Game::GetWorld(worldId);
    Game::WorldTransform wt = {.value = transform};
    Game::SetComponent<Game::WorldTransform>(world, Game::Entity::FromId(entity), wt);
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
        dataSize == MemDb::TypeRegistry::TypeSize(componentId),
        "ComponentGetData: Provided component size in bytes is not the correct size for the given ComponentId."
    );

    Game::EntityMapping mapping = Game::GetEntityMapping(world, entity);
    byte* ptr = (byte*)Game::GetInstanceBuffer(world, mapping.table, mapping.instance.partition, componentId);
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
        dataSize == MemDb::TypeRegistry::TypeSize(componentId),
        "ComponentSetData: Provided component size in bytes is not the correct size for the given ComponentId."
    );

    Game::EntityMapping mapping = Game::GetEntityMapping(world, entity);
    byte* ptr = (byte*)Game::GetInstanceBuffer(world, mapping.table, mapping.instance.partition, componentId);
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
