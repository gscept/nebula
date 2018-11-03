#pragma once
//------------------------------------------------------------------------------
/**
	Attr::Owner

	Attributedefinition for a components owner entity.
	This attribute is always added to all components that derive from Component.

	(C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "attr/attrid.h"

namespace Attr
{
	DeclareEntity(Owner, 'OWNR', Attr::ReadOnly);
} // namespace Attr
