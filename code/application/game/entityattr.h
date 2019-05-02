#pragma once
//------------------------------------------------------------------------------
/**
	Attr::Owner

	Attributedefinition for a components owner entity.
	This attribute is always added to all components that derive from Component.

	(C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "game/entity.h"
#include "game/component/attribute.h"

namespace Attr
{
	__DeclareAttribute(Owner, Game::Entity, 'OWNR', Attr::ReadOnly, uint(-1));
} // namespace Attr
