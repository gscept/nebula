#pragma once
//------------------------------------------------------------------------------
/**
	Entity

	An entity is essentially just an Id with some utility functions attached.

	(C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/id.h"
#include "attr/attrid.h"

namespace Attr
{
	DeclareEntity(Owner, 'OWNR', Attr::ReadOnly);
}

namespace Game
{

struct Entity
{
	Ids::Id32 id;

	//Default constructor
	Entity() : id(0)
	{
		// Empty
	}

	// Entity from id.
	Entity(const Ids::Id32& id) : id(id)
	{
		// empty
	}

	bool operator==(const Ids::Id32& rhs)
	{
		return this->id == rhs;
	}

	Entity& operator=(const Ids::Id32& rhs)
	{
		this->id = rhs;
		return *this;
	}

	/*static bool IsAlive() const
	{
		return false;
	}*/
};

} // namespace Game



