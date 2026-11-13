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
#include "memdb/attributeregistry.h"
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
    PhysicsFeature::PhysicsActor const handle = world->GetComponent<PhysicsFeature::PhysicsActor>(entity);
    return Physics::ActorId(handle.actorId);
}

//------------------------------------------------------------------------------
/**
    What the manager still has queued for an entity. Zero means the handle is settled, which is what
    a single pass leaves behind for an entity which resolves its own body.
*/
static uint32_t
PendingOf(Game::World* world, Game::Entity entity)
{
    PhysicsFeature::PhysicsActor const handle = world->GetComponent<PhysicsFeature::PhysicsActor>(entity);
    return handle.pending;
}

//------------------------------------------------------------------------------
/**
*/
static SizeT
ShapeCountOf(Game::World* world, Game::Entity entity)
{
    Physics::ActorId const actorId = BodyActorOf(world, entity);
    if (!Physics::ActorContext::IsValid(actorId))
        return 0;
    return Physics::ActorContext::GetActor(actorId).shapes.Size();
}

//------------------------------------------------------------------------------
/**
    The shape the handle of an entity names. The field is used when its record still names this entity, and the
    shapes of the actor are looked through when it names nothing.
*/
static Physics::ShapeId
ShapeOfBoxCollider(Game::World* world, Game::Entity entity)
{
    return PhysicsFeature::PhysicsManager::GetShapeForEntity(world, entity);
}

//------------------------------------------------------------------------------
/**
    The shape field as the handle holds it, without the check which validates it. This is what shows whether an
    entity is resolved through its own handle, which is the fast path, or through the shapes of its actor.
*/
static Physics::ShapeId
ShapeFieldOf(Game::World* world, Game::Entity entity)
{
    return Physics::ShapeId(world->GetComponent<PhysicsFeature::PhysicsActor>(entity).shapeId);
}

//------------------------------------------------------------------------------
/**
    The physics manager drains its rebuild queue from its OnFrame, which runs through the game
    server, so the test drives the game server itself. The adds of a frame are executed at the begin
    frame, and the drain which builds what they queued runs in the same frame, so a physics entity is
    built by one frame when nothing about it has to wait for something else.
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

    VERIFY(world->HasComponent<PhysicsFeature::PhysicsActor>(body));
    VERIFY(world->HasComponent<PhysicsFeature::PhysicsActor>(collider));

    Physics::ActorId const bodyActor = BodyActorOf(world, body);
    VERIFY(Physics::ActorContext::IsValid(bodyActor));

    PhysicsFeature::PhysicsActor const colliderHandle = world->GetComponent<PhysicsFeature::PhysicsActor>(collider);
    VERIFY(colliderHandle.actorId == Ids::InvalidId32);
    VERIFY(colliderHandle.body == body);

    VERIFY(Physics::ActorContext::GetActor(bodyActor).shapes.Size() == 1);
    Physics::ShapeId const shapeId = Physics::ActorContext::GetActor(bodyActor).shapes[0];
    Physics::Shape const& shape = Physics::ActorContext::GetShape(shapeId);
    VERIFY(Game::Entity::FromId((Ids::Id32)shape.userData) == collider);

    // the shape an entity built is named by its handle, which is what lets a change to the collider reach its
    // own shape instead of looking through the shapes of the actor
    VERIFY(ShapeOfBoxCollider(world, collider) == shapeId);
    VERIFY(ShapeFieldOf(world, collider) == shapeId);

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

    // the handle names the shape which moved with the entity, which is what detached it from the actor it used
    // to hang off
    VERIFY(ShapeOfBoxCollider(world, collider) == Physics::ActorContext::GetActor(otherActor).shapes[0]);
    VERIFY(ShapeFieldOf(world, collider) == Physics::ActorContext::GetActor(otherActor).shapes[0]);

    PhysicsFeature::PhysicsActor const movedHandle = world->GetComponent<PhysicsFeature::PhysicsActor>(collider);
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

    VERIFY(world->HasComponent<PhysicsFeature::PhysicsActor>(late));
    Physics::ActorId const lateActor = BodyActorOf(world, late);
    VERIFY(Physics::ActorContext::IsValid(lateActor));
    VERIFY(Physics::ActorContext::GetActor(lateActor).shapes.Size() == 0);

    world->AddComponent<PhysicsFeature::BoxCollider>(late);
    StepPhysicsFrames(2);
    VERIFY(Physics::ActorContext::GetActor(lateActor).shapes.Size() == 0);

    // an entity whose shape has not been built names nothing, which is the only case where the actor has to be
    // looked through when the component goes away
    VERIFY(ShapeFieldOf(world, late) == Physics::ShapeId(Ids::InvalidId32));

    PhysicsFeature::PhysicsManager::MarkShapeChanged(world, late);
    StepPhysicsFrames(2);
    VERIFY(Physics::ActorContext::GetActor(lateActor).shapes.Size() == 1);
    VERIFY(ShapeFieldOf(world, late) == Physics::ActorContext::GetActor(lateActor).shapes[0]);

    world->DeleteEntity(late);
    StepPhysicsFrames(1);
    VERIFY(!Physics::ActorContext::IsValid(lateActor));

    // the handle is gone with the entity, so the accessor names nothing. The shape went with the actor, which is
    // what the decay of the handle does, and an entity which is gone cannot be read through its handle
    VERIFY(ShapeOfBoxCollider(world, late) == Physics::ShapeId(Ids::InvalidId32));

    // A flat body with its own collider is built by a single pass. The pass which creates the actor
    // holds the handle it just created, and the collider on that same entity uses it instead of reading
    // the table again, which is what lets the entity settle in the pass which processes it
    Game::Entity const flat = world->CreateEntity();
    world->SetComponent<Game::Position>(flat, Game::Position(60.0f, 0.0f, 0.0f));
    world->SetComponent<Game::Orientation>(flat, Game::Orientation(Math::quat()));
    world->AddComponent<PhysicsFeature::StaticBody>(flat);
    world->AddComponent<PhysicsFeature::BoxCollider>(flat);

    StepPhysicsFrames(1);

    VERIFY(ShapeCountOf(world, flat) == 1);
    VERIFY(PendingOf(world, flat) == 0);
    VERIFY(ShapeFieldOf(world, flat) == Physics::ActorContext::GetActor(BodyActorOf(world, flat)).shapes[0]);

    // A hierarchical collider resolves in the same pass when its body was queued before it, which is the
    // order the adds of the same frame are executed in
    Game::Entity const parent = world->CreateEntity();
    world->SetComponent<Game::Position>(parent, Game::Position(70.0f, 0.0f, 0.0f));
    world->SetComponent<Game::Orientation>(parent, Game::Orientation(Math::quat()));
    world->AddComponent<PhysicsFeature::StaticBody>(parent);

    Game::Entity const child = world->CreateEntity();
    Game::HTransform* childTransform = world->AddComponent<Game::HTransform>(child);
    childTransform->parent = parent;
    childTransform->localPosition = Math::vec3(2.0f, 0.0f, 0.0f);
    PhysicsFeature::BoxCollider* childBox = world->AddComponent<PhysicsFeature::BoxCollider>(child);
    childBox->extents = Math::vec3(0.5f, 0.5f, 0.5f);

    StepPhysicsFrames(1);

    VERIFY(ShapeCountOf(world, parent) == 1);
    VERIFY(PendingOf(world, parent) == 0);
    VERIFY(PendingOf(world, child) == 0);
    VERIFY(world->GetComponent<PhysicsFeature::PhysicsActor>(child).body == parent);

    // the shape hangs off the actor of the parent, but the name of it lives on the entity which built it, which
    // is what makes a change to this collider reach its own shape without walking the actor
    VERIFY(ShapeFieldOf(world, child) == Physics::ActorContext::GetActor(BodyActorOf(world, parent)).shapes[0]);

    // the field is the fast path and not the only path. With it cleared the same shape is found by walking the
    // actor, and what is found is put back in the handle
    PhysicsFeature::PhysicsActor childHandle = world->GetComponent<PhysicsFeature::PhysicsActor>(child);
    childHandle.shapeId = Ids::InvalidId32;
    world->SetComponent(child, childHandle);
    VERIFY(ShapeFieldOf(world, child) == Physics::ShapeId(Ids::InvalidId32));
    VERIFY(ShapeOfBoxCollider(world, child) == Physics::ActorContext::GetActor(BodyActorOf(world, parent)).shapes[0]);
    VERIFY(ShapeFieldOf(world, child) == Physics::ActorContext::GetActor(BodyActorOf(world, parent)).shapes[0]);

    // A collider which is queued before its body cannot resolve in the pass which processes it, and is
    // the only case which legitimately needs a second pass. This is what the retry is for
    Game::Entity const lateParent = world->CreateEntity();
    world->SetComponent<Game::Position>(lateParent, Game::Position(80.0f, 0.0f, 0.0f));
    world->SetComponent<Game::Orientation>(lateParent, Game::Orientation(Math::quat()));

    Game::Entity const lateChild = world->CreateEntity();
    Game::HTransform* lateChildTransform = world->AddComponent<Game::HTransform>(lateChild);
    lateChildTransform->parent = lateParent;
    PhysicsFeature::BoxCollider* lateChildBox = world->AddComponent<PhysicsFeature::BoxCollider>(lateChild);
    lateChildBox->extents = Math::vec3(0.5f, 0.5f, 0.5f);

    // the body is added after the collider, so the handle of the child is queued before the handle of its
    // body and its first pass runs against an actor which does not exist yet
    world->AddComponent<PhysicsFeature::StaticBody>(lateParent);

    StepPhysicsFrames(1);

    VERIFY(PendingOf(world, lateChild) == PhysicsFeature::PendingShape);
    VERIFY(ShapeCountOf(world, lateParent) == 0);

    StepPhysicsFrames(1);

    VERIFY(ShapeCountOf(world, lateParent) == 1);
    VERIFY(PendingOf(world, lateChild) == 0);

    // A hierarchical collider only follows its node when the node is pinged. Writing the local transform
    // propagates the world transform components at the time the write happens, but the pose of the shape is
    // composed by the pass which builds it, so a node which moves without a ping leaves its shape where it
    // was. This is also why a static hierarchy costs nothing on the physics side when nothing pings it
    Game::Entity const chainBody = world->CreateEntity();
    world->SetComponent<Game::Position>(chainBody, Game::Position(100.0f, 0.0f, 0.0f));
    world->SetComponent<Game::Orientation>(chainBody, Game::Orientation(Math::quat()));
    world->AddComponent<PhysicsFeature::StaticBody>(chainBody);

    Game::Entity const level1 = world->CreateEntity();
    Game::HTransform* first = world->AddComponent<Game::HTransform>(level1);
    first->parent = chainBody;
    first->localPosition = Math::vec3(1.0f, 0.0f, 0.0f);

    Game::Entity const level2 = world->CreateEntity();
    Game::HTransform* second = world->AddComponent<Game::HTransform>(level2);
    second->parent = level1;
    second->localPosition = Math::vec3(0.0f, 2.0f, 0.0f);
    PhysicsFeature::BoxCollider* secondBox = world->AddComponent<PhysicsFeature::BoxCollider>(level2);
    secondBox->extents = Math::vec3(0.5f, 0.5f, 0.5f);

    StepPhysicsFrames(1);

    Physics::ActorId const chainActor = BodyActorOf(world, chainBody);
    VERIFY(ShapeCountOf(world, chainBody) == 1);

    // the shape pose is the collider offset composed with the node transforms from the body down to it
    Math::transform const chainPose = Physics::ActorContext::GetShapeTransform(
        Physics::ShapeHandle(Physics::ActorContext::GetShape(Physics::ActorContext::GetActor(chainActor).shapes[0]).shape));
    VERIFY(Math::nearequal(chainPose.position.x, 1.0f, 0.0001f));
    VERIFY(Math::nearequal(chainPose.position.y, 2.0f, 0.0001f));

    Game::HTransform movedNode = world->GetComponent<Game::HTransform>(level2);
    movedNode.localPosition = Math::vec3(0.0f, 3.0f, 0.0f);
    world->SetComponent<Game::HTransform>(level2, movedNode);

    StepPhysicsFrames(1);

    // the world transform of the node moved, the shape of the actor did not
    VERIFY(Math::nearequal(world->GetComponent<Game::Position>(level2).y, 3.0f, 0.0001f));
    Math::transform const staleChainPose = Physics::ActorContext::GetShapeTransform(
        Physics::ShapeHandle(Physics::ActorContext::GetShape(Physics::ActorContext::GetActor(chainActor).shapes[0]).shape));
    VERIFY(Math::nearequal(staleChainPose.position.y, 2.0f, 0.0001f));

    // the ping is what composes the new pose, and it walks the chain from the node up to the body to do it
    PhysicsFeature::PhysicsManager::MarkPoseChanged(world, level2);
    StepPhysicsFrames(1);

    Math::transform const movedChainPose = Physics::ActorContext::GetShapeTransform(
        Physics::ShapeHandle(Physics::ActorContext::GetShape(Physics::ActorContext::GetActor(chainActor).shapes[0]).shape));
    VERIFY(Math::nearequal(movedChainPose.position.x, 1.0f, 0.0001f));
    VERIFY(Math::nearequal(movedChainPose.position.y, 3.0f, 0.0001f));

    // The order the composition walk is in. A point in the shape space goes through the entity its shape is on
    // first and then through its parents, so a rotated node rotates the things below it. With the node rotated 90
    // degrees about x at (1,0,0) and a collider entity below it at (0,1,0), the shape pose is (1,0,1), which is
    // the world transform the hierarchy reports for that entity. Written in the reverse order the answer is
    // (1,1,0), and the two orders only agree when every rotation in the chain is an identity
    {
        Game::Entity const rotatedBody = world->CreateEntity();
        world->SetComponent<Game::Position>(rotatedBody, Game::Position(0.0f, 0.0f, 0.0f));
        world->SetComponent<Game::Orientation>(rotatedBody, Game::Orientation(Math::quat()));
        world->AddComponent<PhysicsFeature::StaticBody>(rotatedBody);

        Math::quat const xRotation(Math::sin(N_PI * 0.25f), 0.0f, 0.0f, Math::cos(N_PI * 0.25f));

        Game::Entity const rotatedNode = world->CreateEntity();
        Game::HTransform* node = world->AddComponent<Game::HTransform>(rotatedNode);
        node->parent = rotatedBody;
        node->localPosition = Math::vec3(1.0f, 0.0f, 0.0f);
        node->localOrientation = xRotation;

        Game::Entity const rotatedCollider = world->CreateEntity();
        Game::HTransform* child = world->AddComponent<Game::HTransform>(rotatedCollider);
        child->parent = rotatedNode;
        child->localPosition = Math::vec3(0.0f, 1.0f, 0.0f);
        child->localOrientation = Math::quat();
        PhysicsFeature::BoxCollider* childBox = world->AddComponent<PhysicsFeature::BoxCollider>(rotatedCollider);
        childBox->extents = Math::vec3(0.5f, 0.5f, 0.5f);

        StepPhysicsFrames(1);

        Physics::ActorId const rotatedActor = BodyActorOf(world, rotatedBody);
        Math::transform const rotatedPose = Physics::ActorContext::GetShapeTransform(
            Physics::ShapeHandle(Physics::ActorContext::GetShape(Physics::ActorContext::GetActor(rotatedActor).shapes[0]).shape));
        VERIFY(Math::nearequal(rotatedPose.position.x, 1.0f, 0.0001f));
        VERIFY(Math::nearequal(rotatedPose.position.y, 0.0f, 0.0001f));
        VERIFY(Math::nearequal(rotatedPose.position.z, 1.0f, 0.0001f));
        VERIFY(Math::nearequal(rotatedPose.rotation.x, xRotation.x, 0.0001f));

        // The shape pose and the hierarchy's world transform for the entity the shape is on are the same transform
        Game::Position const childWorld = world->GetComponent<Game::Position>(rotatedCollider);
        VERIFY(Math::nearequal(rotatedPose.position.x, childWorld.x, 0.0001f));
        VERIFY(Math::nearequal(rotatedPose.position.y, childWorld.y, 0.0001f));
        VERIFY(Math::nearequal(rotatedPose.position.z, childWorld.z, 0.0001f));

        world->DeleteEntity(rotatedBody);
        world->DeleteEntity(rotatedNode);
        world->DeleteEntity(rotatedCollider);
        StepPhysicsFrames(1);
    }

    // A component value written on an entity which already has a handle is not applied by the write itself.
    // The inits and the decay callbacks cover a component appearing and going away, and the manager only
    // touches an entity which is in its rebuild queue, so whoever writes a value has to ping.
    // PhysicsManager::MarkComponentChange is the mapping from a component to the mark its change means, which
    // is what a tool that writes component values pings through. The editor mirrors its writes this way
    {
        Game::Entity const pingBody = world->CreateEntity();
        world->SetComponent<Game::Position>(pingBody, Game::Position(0.0f, 0.0f, 0.0f));
        world->SetComponent<Game::Orientation>(pingBody, Game::Orientation(Math::quat()));
        world->SetComponent<Game::Scale>(pingBody, Game::Scale(Math::vec3(1.0f, 1.0f, 1.0f)));
        world->AddComponent<PhysicsFeature::StaticBody>(pingBody);

        Game::Entity const pingCollider = world->CreateEntity();
        Game::HTransform* pingLocal = world->AddComponent<Game::HTransform>(pingCollider);
        pingLocal->parent = pingBody;
        pingLocal->localPosition = Math::vec3(1.0f, 0.0f, 0.0f);
        PhysicsFeature::BoxCollider* pingBox = world->AddComponent<PhysicsFeature::BoxCollider>(pingCollider);
        pingBox->extents = Math::vec3(0.5f, 0.5f, 0.5f);

        StepPhysicsFrames(1);

        Physics::ActorId const pingActor = BodyActorOf(world, pingBody);

        uint64_t hashBefore;
        hashBefore = Physics::ActorContext::GetShape(Physics::ActorContext::GetActor(pingActor).shapes[0]).geometryHash;

        const Game::ComponentId boxId = Game::GetComponentId<PhysicsFeature::BoxCollider>();
        PhysicsFeature::BoxCollider changedBox = world->GetComponent<PhysicsFeature::BoxCollider>(pingCollider);
        changedBox.extents = Math::vec3(1.0f, 0.5f, 0.5f);
        world->SetComponentValue(pingCollider, boxId, &changedBox, MemDb::AttributeRegistry::TypeSize(boxId));

        StepPhysicsFrames(1);
        // the value is in the component, and the shape is the one it was built from
        VERIFY(Physics::ActorContext::GetShape(ShapeOfBoxCollider(world, pingCollider)).geometryHash == hashBefore);

        // A collider change is the shape mark. The shape is updated onto the existing record instead of being
        // released and built again, which is what shows here: the hash changed and the id was never recycled
        PhysicsFeature::PhysicsManager::MarkComponentChange(world, pingCollider, boxId);
        StepPhysicsFrames(1);
        VERIFY(Physics::ActorContext::GetActor(pingActor).shapes.Size() == 1);
        VERIFY(Ids::Generation(ShapeOfBoxCollider(world, pingCollider).id) == 0);
        VERIFY(Physics::ActorContext::GetShape(ShapeOfBoxCollider(world, pingCollider)).geometryHash != hashBefore);

        // the handle names the shape its entity built, which is the same shape the actor holds
        VERIFY(ShapeOfBoxCollider(world, pingCollider) == Physics::ActorContext::GetActor(pingActor).shapes[0]);

        // A transform change is the pose mark, which recomputes the local pose of the shape on the actor
        Game::Position movedCollider(Math::vec3(2.0f, 0.0f, 0.0f));
        const Game::ComponentId positionId = Game::GetComponentId<Game::Position>();
        world->SetComponentValue(pingCollider, positionId, &movedCollider, MemDb::AttributeRegistry::TypeSize(positionId));

        StepPhysicsFrames(1);
        Math::transform const stalePose = Physics::ActorContext::GetShapeTransform(
            Physics::ShapeHandle(Physics::ActorContext::GetShape(ShapeOfBoxCollider(world, pingCollider)).shape));
        VERIFY(Math::nearequal(stalePose.position.x, 1.0f, 0.0001f));

        PhysicsFeature::PhysicsManager::MarkComponentChange(world, pingCollider, positionId);
        StepPhysicsFrames(1);
        Math::transform const posedShape = Physics::ActorContext::GetShapeTransform(
            Physics::ShapeHandle(Physics::ActorContext::GetShape(ShapeOfBoxCollider(world, pingCollider)).shape));
        VERIFY(Math::nearequal(posedShape.position.x, 2.0f, 0.0001f));

        // A body change is the body mark. Mass is what shows on the actor
        Game::Entity const massBody = world->CreateEntity();
        world->SetComponent<Game::Position>(massBody, Game::Position(20.0f, 0.0f, 0.0f));
        world->SetComponent<Game::Orientation>(massBody, Game::Orientation(Math::quat()));
        world->SetComponent<Game::Scale>(massBody, Game::Scale(Math::vec3(1.0f, 1.0f, 1.0f)));
        PhysicsFeature::Rigidbody* massComponent = world->AddComponent<PhysicsFeature::Rigidbody>(massBody);
        massComponent->mass = 5.0f;
        PhysicsFeature::BoxCollider* massBox = world->AddComponent<PhysicsFeature::BoxCollider>(massBody);
        massBox->extents = Math::vec3(0.5f, 0.5f, 0.5f);

        StepPhysicsFrames(1);
        Physics::ActorId const massActor = BodyActorOf(world, massBody);
        VERIFY(Math::nearequal(Physics::ActorContext::GetActor(massActor).mass, 5.0f, 0.0001f));

        const Game::ComponentId rigidBodyId = Game::GetComponentId<PhysicsFeature::Rigidbody>();
        PhysicsFeature::Rigidbody changedBody = world->GetComponent<PhysicsFeature::Rigidbody>(massBody);
        changedBody.mass = 10.0f;
        world->SetComponentValue(massBody, rigidBodyId, &changedBody, MemDb::AttributeRegistry::TypeSize(rigidBodyId));

        StepPhysicsFrames(1);
        VERIFY(Math::nearequal(Physics::ActorContext::GetActor(massActor).mass, 5.0f, 0.0001f));

        PhysicsFeature::PhysicsManager::MarkComponentChange(world, massBody, rigidBodyId);
        StepPhysicsFrames(1);
        VERIFY(Math::nearequal(Physics::ActorContext::GetActor(massActor).mass, 10.0f, 0.0001f));

        world->DeleteEntity(pingBody);
        world->DeleteEntity(pingCollider);
        world->DeleteEntity(massBody);
        StepPhysicsFrames(1);
    }

    // The poll writes the world transform of a body from the physics actor through the query buffers, which is not
    // a component write, so the tree is not reached by the write on its own. The poll calls the tree explicitly for
    // an entity which is part of a hierarchy. The body here has no local transform, so it is in the hierarchy only
    // as a parent, and the node below it is computed from the transform the poll writes. The collider is on the body
    // itself, which is what makes the handle settle in the first frame and nothing rebuild the body afterwards
    Game::Entity const pollBody = world->CreateEntity();
    world->SetComponent<Game::Position>(pollBody, Game::Position(1.0f, 0.0f, 0.0f));
    world->SetComponent<Game::Orientation>(pollBody, Game::Orientation(Math::quat()));
    world->SetComponent<Game::Scale>(pollBody, Game::Scale(Math::vec3(1.0f, 1.0f, 1.0f)));
    world->AddComponent<PhysicsFeature::Rigidbody>(pollBody);

    PhysicsFeature::BoxCollider* pollBox = world->AddComponent<PhysicsFeature::BoxCollider>(pollBody);
    pollBox->extents = Math::vec3(0.5f, 0.5f, 0.5f);

    Game::Entity const pollNode = world->CreateEntity();
    Game::HTransform* nodeTransform = world->AddComponent<Game::HTransform>(pollNode);
    nodeTransform->parent = pollBody;
    nodeTransform->localPosition = Math::vec3(1.0f, 0.0f, 0.0f);
    nodeTransform->localOrientation = Math::quat();
    nodeTransform->localScale = Math::vec3(1.0f, 1.0f, 1.0f);

    StepPhysicsFrames(1);

    Physics::ActorId const pollActor = BodyActorOf(world, pollBody);
    VERIFY(Physics::ActorContext::IsValid(pollActor));
    VERIFY(ShapeCountOf(world, pollBody) == 1);
    VERIFY(PendingOf(world, pollBody) == 0);
    VERIFY(Math::nearequal(world->GetComponent<Game::Position>(pollNode).x, 2.0f, 0.0001f));

    // The pose is put on the actor directly here, which is what a simulate step does to it. Nothing is queued for
    // the body, so the frame which runs the poll keeps this pose, and the node below is computed from it. The
    // instance has to be marked, because a simulate step marks the entities whose pose it writes and putting a pose
    // on an actor from the test does not, and a pose put on an actor is applied at the flush, which is not always
    // the frame the poll reads it in. The body is stepped until the transform the poll wrote is the transform the
    // actor holds, and what is being tested is that the poll reaches the tree: the node below has no source for
    // its transform except the tree, so it has to follow the pose the poll wrote
    Physics::ActorContext::SetPositionOrientation(pollActor, Math::vec3(10.0f, 0.0f, 0.0f), Math::quat());
    world->MarkAsModified(pollBody);

    bool polled = false;
    for (int frame = 0; frame < 6 && !polled; frame++)
    {
        StepPhysicsFrames(1);
        Math::vec3 actorPosition;
        Math::quat actorOrientation;
        Physics::ActorContext::GetPositionOrientation(pollActor, actorPosition, actorOrientation);
        polled = Math::nearequal(world->GetComponent<Game::Position>(pollBody).x, actorPosition.x, 0.0001f);
    }
    VERIFY(polled);

    Game::Position const polledBody = world->GetComponent<Game::Position>(pollBody);
    Game::Position const polledNode = world->GetComponent<Game::Position>(pollNode);
    VERIFY(Math::nearequal(polledNode.x, polledBody.x + 1.0f, 0.0001f));
    VERIFY(Math::nearequal(polledNode.y, polledBody.y, 0.0001f));

    // An entity which is both a body and a hierarchy member is computed from its own local transform, which is what
    // makes the world the poll wrote and the local the tree derived agree
    Game::Entity const bodyNode = world->CreateEntity();
    Game::HTransform* bodyNodeTransform = world->AddComponent<Game::HTransform>(bodyNode);
    bodyNodeTransform->parent = pollBody;
    bodyNodeTransform->localPosition = Math::vec3(2.0f, 0.0f, 0.0f);
    bodyNodeTransform->localOrientation = Math::quat();
    bodyNodeTransform->localScale = Math::vec3(1.0f, 1.0f, 1.0f);
    StepPhysicsFrames(1);
    VERIFY(Math::nearequal(world->GetComponent<Game::Position>(bodyNode).x,
        world->GetComponent<Game::Position>(pollBody).x + 2.0f, 0.0001f));

    world->DeleteEntity(pollBody);
    world->DeleteEntity(pollNode);
    world->DeleteEntity(bodyNode);
    StepPhysicsFrames(1);

    world->DeleteEntity(chainBody);
    world->DeleteEntity(level1);
    world->DeleteEntity(level2);
    StepPhysicsFrames(1);
    VERIFY(!Physics::ActorContext::IsValid(chainActor));

    world->DeleteEntity(flat);
    world->DeleteEntity(parent);
    world->DeleteEntity(child);
    world->DeleteEntity(lateChild);
    world->DeleteEntity(lateParent);
    StepPhysicsFrames(1);
}

} // namespace Test
