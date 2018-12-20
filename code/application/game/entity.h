#pragma once
//------------------------------------------------------------------------------
/**
	Entity

	An entity is essentially just an Id with some utility functions attached.
	What actually makes up the entities are their components.
	
	@see	component/basecomponent.h
	@see	component/component.h

	(C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/id.h"

namespace Game
{
	ID_32_TYPE(Entity)
} // namespace Game



