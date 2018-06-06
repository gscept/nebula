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
};

} // namespace Game



