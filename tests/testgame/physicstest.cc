//------------------------------------------------------------------------------
//  @file physicstest.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "physicstest.h"
#include "game/gameserver.h"
#include "game/world.h"
#include "basegamefeature/components/basegamefeature.h"
#include "basegamefeature/components/position.h"
#include "basegamefeature/components/orientation.h"
#include "physicsfeature/components/physicsfeature.h"
#include "physicsfeature/managers/physicsmanager.h"
#include "physics/actorcontext.h"

namespace Test
{

__ImplementClass(Test::PhysicsManagerTest, 'PMTs', Test::TestCase);

//------------------------------------------------------------------------------
/**
*/
static Physics::ActorId
BodyActorOf(Game::World* world, Game::Entity entity)
{
    PhysicsFeature::PhysicsActorId const handle = world->GetComponent<PhysicsFeature::PhysicsActorId>(entity);
    return Physics::ActorId(handle.actorId);
}

//------------------------------------------------------------------------------
/**
    The physics manager drains its rebuild queue from its OnFrame, which runs through the game
    server, so the test drives the game server itself. A physics entity needs one frame to get its
    handle and one more for the handle to be drained, so most steps use two frames.
*/
static void
StepPhysicsFrames(int numFrames)
{
    for (int i = 0; i < numFrames; i++)
    {
        Game::GameServer::Instance()->OnBeginFrame();
        Game::GameServer::Instance()->OnFrame();
        Game::GameServer::Instance()->OnEndFrame();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
PhysicsManagerTest::Run()
{
    Game::World* world = Game::GetWorld(WORLD_DEFAULT);
    n_assert(world != nullptr);

    // a static body with a collider child builds one actor with one shape, and the handle of the
    // collider remembers whose actor its shape hangs off
    Game::Entity const body = world->CreateEntity();
    world->SetComponent<Game::Position>(body, Game::Position(1.0f, 2.0f, 3.0f));
    world->SetComponent<Game::Orientation>(body, Game::Orientation(Math::quat()));
    world->AddComponent<PhysicsFeature::StaticBody>(body);

    Game::Entity const collider = world->CreateEntity();
    Game::HTransform* transform = world->AddComponent<Game::HTransform>(collider);
    transform->parent = body;
    transform->localPosition = Math::vec3(1.0f, 2.0f, 3.0f);
    PhysicsFeature::BoxCollider* box = world->AddComponent<PhysicsFeature::BoxCollider>(collider);
    box->extents = Math::vec3(0.5f, 0.5f, 0.5f);

    StepPhysicsFrames(3);

    VERIFY(world->HasComponent<PhysicsFeature::PhysicsActorId>(body));
    VERIFY(world->HasComponent<PhysicsFeature::PhysicsActorId>(collider));

    Physics::ActorId const bodyActor = BodyActorOf(world, body);
    VERIFY(Physics::ActorContext::IsValid(bodyActor));

    PhysicsFeature::PhysicsActorId const colliderHandle = world->GetComponent<PhysicsFeature::PhysicsActorId>(collider);
    VERIFY(colliderHandle.actorId == Ids::InvalidId32);
    VERIFY(colliderHandle.body == body);

    VERIFY(Physics::ActorContext::GetActor(bodyActor).shapes.Size() == 1);
    Physics::ShapeId const shapeId = Physics::ActorContext::GetActor(bodyActor).shapes[0];
    Physics::Shape const& shape = Physics::ActorContext::GetShape(shapeId);
    VERIFY(Game::Entity::FromId((Ids::Id32)shape.userData) == collider);

    Math::vec3 actorPosition;
    Math::quat actorOrientation;
    Physics::ActorContext::GetPositionOrientation(bodyActor, actorPosition, actorOrientation);
    VERIFY(Math::nearequal(actorPosition.x, 1.0f, 0.0001f));
    VERIFY(Math::nearequal(actorPosition.y, 2.0f, 0.0001f));
    VERIFY(Math::nearequal(actorPosition.z, 3.0f, 0.0001f));

    // the shape of a collider is built with the transforms from the body down to the collider
    // composed into its local pose on the actor of the body
    Math::transform const shapePose = Physics::ActorContext::GetShapeTransform(Physics::ShapeHandle(shape.shape));
    VERIFY(Math::nearequal(shapePose.position.x, 1.0f, 0.0001f));
    VERIFY(Math::nearequal(shapePose.position.y, 2.0f, 0.0001f));
    VERIFY(Math::nearequal(shapePose.position.z, 3.0f, 0.0001f));

    // moving the collider to another body and pinging it detaches the shape from the actor of the
    // old body and attaches it to the actor of the new one
    Game::Entity const otherBody = world->CreateEntity();
    world->SetComponent<Game::Position>(otherBody, Game::Position(10.0f, 0.0f, 0.0f));
    world->SetComponent<Game::Orientation>(otherBody, Game::Orientation(Math::quat()));
    world->AddComponent<PhysicsFeature::StaticBody>(otherBody);

    Game::HTransform moved = world->GetComponent<Game::HTransform>(collider);
    moved.parent = otherBody;
    world->SetComponent<Game::HTransform>(collider, moved);
    PhysicsFeature::PhysicsManager::MarkShapeChanged(world, collider);

    StepPhysicsFrames(3);

    VERIFY(Physics::ActorContext::GetActor(bodyActor).shapes.Size() == 0);

    Physics::ActorId const otherActor = BodyActorOf(world, otherBody);
    VERIFY(Physics::ActorContext::IsValid(otherActor));
    VERIFY(Physics::ActorContext::GetActor(otherActor).shapes.Size() == 1);

    PhysicsFeature::PhysicsActorId const movedHandle = world->GetComponent<PhysicsFeature::PhysicsActorId>(collider);
    VERIFY(movedHandle.body == otherBody);

    Math::transform const movedShapePose = Physics::ActorContext::GetShapeTransform(
        Physics::ShapeHandle(Physics::ActorContext::GetShape(Physics::ActorContext::GetActor(otherActor).shapes[0]).shape));
    VERIFY(Math::nearequal(movedShapePose.position.x, 1.0f, 0.0001f));
    VERIFY(Math::nearequal(movedShapePose.position.y, 2.0f, 0.0001f));
    VERIFY(Math::nearequal(movedShapePose.position.z, 3.0f, 0.0001f));

    // a static body only moves when it is pinged, since nothing else polls it
    world->SetComponent<Game::Position>(otherBody, Game::Position(20.0f, 0.0f, 0.0f));
    PhysicsFeature::PhysicsManager::MarkPoseChanged(world, otherBody);

    StepPhysicsFrames(1);

    Physics::ActorContext::GetPositionOrientation(otherActor, actorPosition, actorOrientation);
    VERIFY(Math::nearequal(actorPosition.x, 20.0f, 0.0001f));
    VERIFY(Math::nearequal(actorPosition.y, 0.0f, 0.0001f));
    VERIFY(Math::nearequal(actorPosition.z, 0.0f, 0.0001f));

    // a collider component which goes away releases its shape from the actor that owns it
    world->RemoveComponent<PhysicsFeature::BoxCollider>(collider);
    StepPhysicsFrames(2);
    VERIFY(Physics::ActorContext::GetActor(otherActor).shapes.Size() == 0);

    // a body component which goes away releases the actor itself
    world->RemoveComponent<PhysicsFeature::StaticBody>(otherBody);
    StepPhysicsFrames(2);
    VERIFY(!Physics::ActorContext::IsValid(otherActor));

    // an entity whose handle goes away takes its actor with it through the decay of the handle
    world->DeleteEntity(collider);
    world->DeleteEntity(body);
    StepPhysicsFrames(2);
    VERIFY(!Physics::ActorContext::IsValid(bodyActor));

    // the component inits create the handle for a fresh physics entity. A component which is added
    // to an entity whose setup already happened is a change, and nothing rebuilds it until it is
    // pinged
    Game::Entity const late = world->CreateEntity();
    world->SetComponent<Game::Position>(late, Game::Position(5.0f, 5.0f, 5.0f));
    world->SetComponent<Game::Orientation>(late, Game::Orientation(Math::quat()));
    world->AddComponent<PhysicsFeature::StaticBody>(late);

    StepPhysicsFrames(3);

    VERIFY(world->HasComponent<PhysicsFeature::PhysicsActorId>(late));
    Physics::ActorId const lateActor = BodyActorOf(world, late);
    VERIFY(Physics::ActorContext::IsValid(lateActor));
    VERIFY(Physics::ActorContext::GetActor(lateActor).shapes.Size() == 0);

    world->AddComponent<PhysicsFeature::BoxCollider>(late);
    StepPhysicsFrames(2);
    VERIFY(Physics::ActorContext::GetActor(lateActor).shapes.Size() == 0);

    PhysicsFeature::PhysicsManager::MarkShapeChanged(world, late);
    StepPhysicsFrames(2);
    VERIFY(Physics::ActorContext::GetActor(lateActor).shapes.Size() == 1);

    world->DeleteEntity(late);
    StepPhysicsFrames(1);
    VERIFY(!Physics::ActorContext::IsValid(lateActor));
}

} // namespace Test
