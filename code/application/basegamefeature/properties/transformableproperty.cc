//------------------------------------------------------------------------------
//  transformableproperty.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "transformableproperty.h"
#include "basegamefeature/managers/categorymanager.h"

namespace Attr
{
__DefineAttribute(LocalTransform, Math::matrix44, 'Lm44', Math::matrix44::identity());
__DefineAttribute(WorldTransform, Math::matrix44, 'Wm44', Math::matrix44::identity());
__DefineAttribute(Parent, Game::Entity, 'TFPT', Game::Entity::Invalid());
}

namespace Game
{

__ImplementClass(Game::TransformableProperty, 'TRPR', Game::Property);

//------------------------------------------------------------------------------
/**
*/
TransformableProperty::TransformableProperty()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
TransformableProperty::~TransformableProperty()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
TransformableProperty::SetupExternalAttributes()
{
	SetupAttr(Attr::LocalTransform::Id());
	SetupAttr(Attr::WorldTransform::Id());
	SetupAttr(Attr::Parent::Id());
}

} // namespace Game
