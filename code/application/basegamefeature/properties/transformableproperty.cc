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
TransformableProperty::Init()
{
	this->data = {
		Game::GetPropertyData<Attr::Owner>(this->category),
		Game::GetPropertyData<Attr::LocalTransform>(this->category),
		Game::GetPropertyData<Attr::WorldTransform>(this->category),
		Game::GetPropertyData<Attr::Parent>(this->category)
	}
}

//------------------------------------------------------------------------------
/**
*/
void
TransformableProperty::OnActivate(Game::InstanceId instance)
{
	//TransformManager::Instance()->RegisterEntity(this->data.owner[instance.id], this->data.localTransform[instance.id], this->data.parent[instance.id])
}

//------------------------------------------------------------------------------
/**
*/
void
TransformableProperty::OnDeactivate(Game::InstanceId instance)
{
	//TransformManager::Instance()->DeregisterEntity(this->data.owner[instance.id])
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
