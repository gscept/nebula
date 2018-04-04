//------------------------------------------------------------------------------
//  entitymanager.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "entitymanager.h"

namespace Game
{


__ImplementClass(Game::EntityManager, 'EnMr', Core::RefCounted);
__ImplementSingleton(EntityManager)

//------------------------------------------------------------------------------
/**
*/
EntityManager::EntityManager()
{
	__ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
EntityManager::~EntityManager()
{
	__DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
Entity EntityManager::NewEntity()
{
	Entity e;
	this->pool.Allocate(e.id);
	return e;
}

//------------------------------------------------------------------------------
/**
*/
void EntityManager::DeleteEntity(const Entity & e)
{
	this->pool.Deallocate(e.id);
}

//------------------------------------------------------------------------------
/**
*/
bool EntityManager::IsAlive(const Entity & e) const
{
	return this->pool.IsValid(e.id);
}

} // namespace Game


