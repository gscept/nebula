#pragma once
//------------------------------------------------------------------------------
/**
	@class	Game::EntityManager

	Keeps track of all existing entites.

	Components can register deletion callbacks to

	(C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "core/singleton.h"
#include "ids/idgenerationpool.h"
#include "game/entity.h"
#include "game/manager.h"
#include "util/delegate.h"
#include "game/component/componentinterface.h"

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
	
	/// Create n amount of entities at the same time.
	Util::Array<Entity> CreateEntities(uint n);

	/// Delete an entity.
	void DeleteEntity(const Entity& e);
	
	/// Check if an entity ID is still valid.
	bool IsAlive(const Entity& e) const;

	static uint32_t GetIndex(const Entity& entity);

	/// Returns number of active entities
	uint GetNumEntities() const;

	/// Invalidates all entities and essentially resets the manager.
	void InvalidateAllEntities();

	/// Register a deletion callback to an entity
	void RegisterDeletionCallback(const Entity& e, ComponentInterface* component);
	
	/// Deregister a deletion callback to an entity. Note that this is not super fast.
	void DeregisterDeletionCallback(const Entity& e, ComponentInterface* component);
private:
	/// Register a deletion callback to when a specific entity is deleted
	void RegisterDeletionCallback(const Entity& e, const Util::Delegate<void(Entity)>& callback);

	/// Generation pool
	Ids::IdGenerationPool pool;

	/// Number of entities alive
	SizeT numEntities;

	/// Contains all callbacks for deletion to components for each entity
	Util::HashTable<Entity, Util::Array<Util::Delegate<void(Entity)>>> deletionCallbacks;
};

} // namespace Game