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

ID_32_TYPE(Entity)

/*
struct Entity
{
	Ids::Id32 id;

	//Default constructor
	constexpr Entity() : id(-1)
	{
		// Empty
	}

	// Entity from id.
	constexpr Entity(const Ids::Id32& id) : id(id)
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

	/// return a 32-bit hash code for the string
	constexpr IndexT HashCode() const
	{
		return this->id;
	}

	constexpr explicit operator Ids::Id32() const 
	{
		return this->id;
	}
};
*/

} // namespace Game



