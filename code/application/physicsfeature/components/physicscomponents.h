#pragma once
//------------------------------------------------------------------------------
/**
	PhysicsComponents

	(C) 2019 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "game/component/component.h"
#include "game/component/attribute.h"
#include "physicsfeature/components/physicsdata.h"

namespace PhysicsFeature
{

class ActorComponent
{
    __DeclareComponent(ActorComponent)
public:
    static void SetupAcceptedMessages();

    static void OnActivate(Game::InstanceId instance);
    static void OnDeactivate(Game::InstanceId instance);

    static void SetWorldTransform(Game::Entity entity, const Math::matrix44& val);
    /// Return this components fourcc
    static Util::FourCC GetFourCC();
private:

};

} // namespace PhysicsFeature
