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

	/// instance id point into a category table. Entities are mapped to instanceids
	ID_32_TYPE(InstanceId);

	/// 8+24 bits entity id (generation+index)
	ID_32_TYPE(Entity);
} // namespace Game



