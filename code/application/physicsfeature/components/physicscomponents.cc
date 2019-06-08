//------------------------------------------------------------------------------
//  physicscomponent.cc
//  (C) 2019 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "physicscomponents.h"
#include "basegamefeature/messages/basegameprotocol.h"
#include "basegamefeature/components/transformcomponent.h"
#include "basegamefeature/managers/componentmanager.h"
#include "game/component/componentserialization.h"
#include "physicsfeature/components/physicsdata.h"
#include "physicsinterface.h"
#include "physics/streamactorpool.h"
#include "physics/actorcontext.h"
#include "resources/resourcemanager.h"

namespace PhysicsFeature
{

static ActorComponentAllocator* component;

__ImplementComponent(PhysicsFeature::ActorComponent, component)

//------------------------------------------------------------------------------
/**
*/
void
ActorComponent::Create()
{
	if (component != nullptr)
	{
		component->DestroyAll();
	}
	else
	{
        component = n_new(ActorComponentAllocator);
	}

	component->DestroyAll();

	__SetupDefaultComponentBundle(component);
	component->functions.OnActivate = OnActivate;
	component->functions.OnDeactivate = OnDeactivate;
	__RegisterComponent(component, "ActorComponent"_atm);

	SetupAcceptedMessages();
}

//------------------------------------------------------------------------------
/**
*/
void
ActorComponent::Discard()
{
	
}

//------------------------------------------------------------------------------
/**
*/
void
ActorComponent::SetupAcceptedMessages()
{
	__RegisterMsg(Msg::SetWorldTransform, SetWorldTransform);	
}

//------------------------------------------------------------------------------
/**
*/
void
MoveCallback(Physics::ActorId id, Math::matrix44 const& trans)
{
    Physics::Actor& actor = Physics::ActorContext::GetActor(id);
    Game::InstanceId gameId = (Game::InstanceId)actor.userData;
    Game::TransformComponent::SetWorldTransform(component->GetOwner(gameId), trans);
}
//------------------------------------------------------------------------------
/**
*/
void
ActorComponent::OnActivate(Game::InstanceId instance)
{
    auto trans = Game::TransformComponent::GetWorldTransform(component->GetOwner(instance));
    auto actorResource = Resources::CreateResource(component->Get<Attr::PhysicsResource>(instance),"NONE",
        [trans, instance](Resources::ResourceId id)
        {
            bool dynamic = component->Get<Attr::Dynamic>(instance);
            Physics::ActorId actorid = Physics::CreateActorInstance(id, trans, dynamic);
            component->Get<Attr::Actor>(instance) = (uint32_t)actorid.id;
            Physics::Actor& actor = Physics::ActorContext::GetActor(actorid);
            actor.userData = instance;            
            actor.moveCallback = Util::Delegate<void(Physics::ActorId, Math::matrix44 const&)>::FromFunction<&MoveCallback>();
        },[instance](Resources::ResourceId id) 
        {
            n_warning("failed to load physics actor from %s\n", component->Get<Attr::PhysicsResource>(instance).AsCharPtr());
        }, true);

}

//------------------------------------------------------------------------------
/**
*/
void
ActorComponent::OnDeactivate(Game::InstanceId instance)
{
    // FIXME	
}

//------------------------------------------------------------------------------
/**
*/
void
ActorComponent::SetWorldTransform(Game::Entity entity, const Math::matrix44& val)
{
    Physics::ActorId actorId = component->Get<Attr::Actor>(component->GetInstance(entity));
    Physics::ActorContext::SetTransform(actorId, val);
}


//------------------------------------------------------------------------------
/**
*/
Util::FourCC
ActorComponent::GetFourCC()
{
	return component->GetIdentifier();
}

} // namespace PhysicsFeature
