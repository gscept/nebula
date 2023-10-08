//------------------------------------------------------------------------------
//  game.cc
//  (C) 2019 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "game.h"
#include "game/api.h"
#include "game/world.h"

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
uint32_t
WorldGetDefaultWorldId()
{
    return WORLD_DEFAULT;
}

} // namespace Api
} // namespace Scripting
