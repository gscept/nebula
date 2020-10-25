#pragma once
//------------------------------------------------------------------------------
/**
	Entity

	An entity is essentially just an Id with some utility functions attached.
	What actually makes up the entities are their properties and attributes.
	
	@see	property.h
	@see	game/database/attribute.h
	
	(C) 2018-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/id.h"

namespace Game
{
	/// category id
	ID_32_TYPE(CategoryId);

	/// category hash
	struct CategoryHash
	{
		uint32_t id = 0;

		const bool operator==(CategoryHash const rhs) const { return id == rhs.id; } \
		const bool operator!=(CategoryHash const rhs) const { return id != rhs.id; } \
		const bool operator<(CategoryHash const rhs) const { return HashCode() < rhs.HashCode(); } \
		const bool operator>(CategoryHash const rhs) const { return HashCode() > rhs.HashCode(); } \

		void AddToHash(uint32_t i)
		{
			this->id += i;
			this->id = Hash(this->id);
		}
		static uint32_t Hash(uint32_t i)
		{
			i = ((i >> 16) ^ i) * 0x45d9f3b;
			i = ((i >> 16) ^ i) * 0x45d9f3b;
			i = (i >> 16) ^ i;
			return i;
		}
		static uint32_t UnHash(uint32_t i)
		{
			i = ((i >> 16) ^ i) * 0x119de1f3;
			i = ((i >> 16) ^ i) * 0x119de1f3;
			i = (i >> 16) ^ i;
			return i;
		}
		uint32_t HashCode() const { return id; }
	};

	/// instance id point into a category table. Entities are mapped to instanceids
	ID_32_TYPE(InstanceId);

	/// 8+24 bits entity id (generation+index)
	ID_32_TYPE(Entity);
} // namespace Game



