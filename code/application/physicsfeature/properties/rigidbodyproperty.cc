//------------------------------------------------------------------------------
//  physicscomponent.cc
//  (C) 2019-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "rigidbodyproperty.h"
#include "physicsinterface.h"
#include "physics/streamactorpool.h"
#include "physics/actorcontext.h"
#include "resources/resourceserver.h"
#include "basegamefeature/managers/entitymanager.h"
#include <type_traits>

namespace Attr
{
	__DefineAttribute(IsDynamic);
	__DefineAttribute(PhysicsResource);
}

namespace PhysicsFeature
{

__ImplementClass(PhysicsFeature::RigidBodyProperty, 'PRBP', Game::Property);


//------------------------------------------------------------------------------
/**
*/
void
RigidBodyProperty::SetupExternalAttributes()
{
	SetupAttr(Attr::IsDynamic::Id());
	SetupAttr(Attr::PhysicsResource::Id());
}

//------------------------------------------------------------------------------
/**
*/
void
RigidBodyProperty::MoveCallback(Physics::ActorId id, Math::matrix44 const& trans)
{
    Physics::Actor& actor = Physics::ActorContext::GetActor(id);
    Game::InstanceId instance = (Game::InstanceId)actor.userData;
	this->data.worldTransform[instance.id] = trans;
}

//------------------------------------------------------------------------------
/**
*/
void
RigidBodyProperty::Init()
{
	this->data = {
		Game::CreatePropertyState<RigidBodyState>(this->category),
		Game::GetPropertyData<Attr::WorldTransform>(this->category),
		Game::GetPropertyData<Attr::IsDynamic>(this->category),
		Game::GetPropertyData<Attr::PhysicsResource>(this->category),
	};
}

//------------------------------------------------------------------------------
/**
*/
void
RigidBodyProperty::OnActivate(Game::InstanceId instance)
{
	auto trans = this->data.worldTransform[instance.id];
	auto dynamic = this->data.isDynamic[instance.id];
	auto& actorid = this->data.state[instance.id].actorid;
	auto const& resource = this->data.resource[instance.id];
    this->data.state[instance.id].actorResource = Resources::CreateResource(this->data.resource[instance.id],"NONE",
        [this, trans, dynamic, &actorid, instance](Resources::ResourceId id)
        {
            actorid = Physics::CreateActorInstance(id, trans, dynamic);
            Physics::Actor& actor = Physics::ActorContext::GetActor(actorid);
            actor.userData = instance.id;
            actor.moveCallback = Util::Delegate<void(Physics::ActorId, Math::matrix44 const&)>::FromMethod<RigidBodyProperty, &RigidBodyProperty::MoveCallback>(this);
        },[&resource, instance](Resources::ResourceId id) 
        {
            n_warning("failed to load physics actor from %s\n", resource);
        }, true);

}

//------------------------------------------------------------------------------
/**
*/
void
RigidBodyProperty::OnDeactivate(Game::InstanceId instance)
{
    // FIXME	
}

//------------------------------------------------------------------------------
/**
*/
void
RigidBodyProperty::SetWorldTransform(Game::Entity entity, const Math::matrix44& val)
{
	//n_assert(Game::GetEntityCategory(entity) == this->category);
	Physics::ActorId actorId = this->data.state[Game::GetInstanceId(entity).id].actorid;
    Physics::ActorContext::SetTransform(actorId, val);
}

} // namespace PhysicsFeature
