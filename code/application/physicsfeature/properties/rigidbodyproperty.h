#pragma once
//------------------------------------------------------------------------------
/**
	PhysicsComponents

	(C) 2019-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "game/property.h"
#include "game/database/attribute.h"
#include "basegamefeature/properties/transformableproperty.h"
#include "physicsinterface.h"

namespace Attr
{
    __DeclareAttribute(IsDynamic, bool, 'phDy', bool(true));
    __DeclareAttribute(PhysicsResource, Util::String, 'phRs', Util::String("phys:system/placeholder.np"));
}

namespace PhysicsFeature
{

class RigidBodyProperty : public Game::Property
{
    __DeclareClass(RigidBodyProperty)
public:
    void SetupExternalAttributes();

    void Init() override;

    void OnActivate(Game::InstanceId instance) override;
    void OnDeactivate(Game::InstanceId instance) override;

    void SetWorldTransform(Game::Entity entity, const Math::matrix44& val);
    void MoveCallback(Physics::ActorId id, Math::matrix44 const& trans);

    struct RigidBodyState
    {
        __DeclareState('RBST');
        Physics::ActorId actorid;
        Resources::ResourceId actorResource;
    };
private:
    struct Data
    {
        Game::PropertyData<RigidBodyState> state;
        Game::PropertyData<Math::matrix44> worldTransform;
        Game::PropertyData<bool> isDynamic;
        Game::PropertyData<Util::String> resource;
    } data;
};

} // namespace PhysicsFeature
