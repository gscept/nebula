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
#include "entity.h"

namespace Game {

class EntityManager : public Core::RefCounted
{
	
	__DeclareClass(EntityManager)
	__DeclareSingleton(EntityManager)
public:
	/// constructor
	EntityManager();
	/// destructor
	~EntityManager();

	Entity NewEntity();
	
	void DeleteEntity(const Entity& e);
	
	bool IsAlive(const Entity& e) const;	

private:
	Ids::IdGenerationPool pool;
};

} // namespace Game