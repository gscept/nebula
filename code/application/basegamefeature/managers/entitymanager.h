#pragma once
//------------------------------------------------------------------------------
/**
	Entity Manager

	Keeps track of all existing entites

	(C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "core/singleton.h"
#include "ids/idgenerationpool.h"
#include "game/entity.h"
#include "game/manager.h"

namespace Game {

class EntityManager : public Game::Manager
{
	__DeclareClass(EntityManager)
	__DeclareSingleton(EntityManager)
public:
	/// constructor
	EntityManager();
	/// destructor
	~EntityManager();

	/// Generate a new entity.
	Entity NewEntity();
	
	/// Delete an entity.
	void DeleteEntity(const Entity& e);
	
	/// Check if an entity ID is still valid.
	bool IsAlive(const Entity& e) const;

private:
	Ids::IdGenerationPool pool;
};

} // namespace Game