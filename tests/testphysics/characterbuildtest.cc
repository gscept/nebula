//------------------------------------------------------------------------------
//  @file characterbuildtest.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "testbase/testcase.h"
#include "characterbuildtest.h"

#include "physicsinterface.h"
#include "physics/physxstate.h"
#include "physics/actorcontext.h"
#include "physics/charactercontext.h"
#include "physics/scenequery.h"

#include "PxPhysicsAPI.h"

using namespace physx;

namespace Test
{
__ImplementClass(Test::CharacterBuildTest, 'CBTC', Test::TestCase);

//------------------------------------------------------------------------------
/**
*/
static bool
NearlyEqual(float a, float b)
{
    float diff = a - b;
    if (diff < 0.0f) { diff = -diff; }
    return diff < 0.0001f;
}

static int hitCount;
static Physics::CharacterHitEvent lastHit;

//------------------------------------------------------------------------------
/**
*/
static void
OnCharacterHit(Physics::CharacterHitEvent const& event)
{
    hitCount++;
    lastHit = event;
}

//------------------------------------------------------------------------------
/**
    A static box the character can stand on, and one it can walk into
*/
static Physics::ActorId
CreateWall(IndexT scene, Math::vec3 const& center, Math::vec3 const& extents, uint32_t group)
{
    Physics::BodyDescription body;
    body.type = Physics::Static;
    body.scene = scene;
    body.collisionGroup = group;
    body.transform = Math::transform(center);

    Physics::ActorId actor = Physics::ActorContext::CreateActor(body);

    Physics::ColliderDescription collider;
    collider.type = Physics::ColliderType_Box;
    collider.boxExtents = extents;

    Physics::ActorContext::AddShape(actor, collider);
    return actor;
}

//------------------------------------------------------------------------------
/**
    A controller built from plain data, and the records this context holds for the physx objects it
    owns. The controller owns its actor, so the actor context can only hold a record for it, and that
    record is what makes the character nameable by a query or a contact callback.
*/
void
CharacterBuildTest::Run()
{
    Physics::state.Setup();

    // the fallback material is created by Physics::Setup(), which needs the resource server,
    // so it is created here instead
    Physics::state.FallbackMaterial.material = Physics::state.physics->createMaterial(0.7f, 0.7f, 0.3f);
    Physics::state.FallbackMaterial.density = 1.0f;

    IndexT scene = Physics::CreateScene();

    hitCount = 0;

    // a ground the character can stand on, and a wall in a group the character does not accept
    Physics::ActorId ground = CreateWall(scene, Math::vec3(0.0f, -0.5f, 0.0f), Math::vec3(5.0f, 0.5f, 5.0f), 0);
    Physics::ActorId wall = CreateWall(scene, Math::vec3(3.0f, 1.0f, 0.0f), Math::vec3(0.5f, 1.0f, 5.0f), 8);

    Physics::CharacterCreateInfo info;
    info.type = Physics::ColliderType_Capsule;
    info.capsule.radius = 0.5f;
    info.capsule.height = 1.0f;
    info.contactOffset = 0.1f;
    info.collisionGroup = 5;
    info.collisionMask = 2;
    info.userData = 0x1234;
    info.reportCallback = &OnCharacterHit;

    // the center is the foot plus half the shape, which is what the desc takes
    Physics::CharacterId character = Physics::CharacterContext::CreateCharacter(
        Math::vec3(0.0f, 1.5f, 0.0f), info, scene);

    VERIFY(Physics::CharacterContext::IsValid(character));

    Physics::Character const& record = Physics::CharacterContext::GetCharacter(character);
    VERIFY(record.actor.id != Ids::InvalidId32);
    VERIFY(record.shape.id != Ids::InvalidId32);

    // the record is a record and not an ownership, because the controller owns the physx objects
    VERIFY(Physics::ActorContext::IsValid(record.actor));
    VERIFY(!Physics::ActorContext::GetActor(record.actor).ownsActor);
    VERIFY(Physics::ActorContext::GetActor(record.actor).userData == 0x1234);
    VERIFY(Physics::ActorContext::GetShape(record.shape).userData == 0x1234);

    // the actor the record names is the controller's own
    VERIFY(Physics::ActorContext::GetPxActor(record.actor) == record.controller->getActor());
    VERIFY(record.controller->getActor()->getNbShapes() == 1);

    // the group is on the shape's filter data, which is what a scene query reads
    PxShape* pxShapes[1];
    record.controller->getActor()->getShapes(pxShapes, 1);
    PxShape* pxShape = pxShapes[0];
    VERIFY(pxShape->getSimulationFilterData().word0 == 5);
    VERIFY(pxShape->getQueryFilterData().word0 == 5);

    // a query names the character as an entity through the record
    Physics::QueryFilter filter;
    filter.mask = 2;
    Physics::RaycastHit hit;
    VERIFY(Physics::SegmentCast(scene, Math::point(0.0f, 1.5f, 0.0f), Math::point(0.0f, 1.5f, 3.0f), hit, filter));
    VERIFY(hit.shapeId == record.shape.id);
    VERIFY(hit.entity == 0x1234);

    // the character's own shape is not accepted by its own move, so a move that starts inside it
    // reports nothing
    hitCount = 0;
    Physics::CharacterCollision collision = Physics::CharacterContext::MoveCharacter(character, Math::vec3(0.0f, 0.0f, 0.0f), 0.00001f, 0.016f);
    VERIFY(hitCount == 0);

    // the wall is in group 8, which the mask does not accept, so the character walks through it
    collision = Physics::CharacterContext::MoveCharacter(character, Math::vec3(3.0f, 0.0f, 0.0f), 0.00001f, 0.016f);
    VERIFY(!collision.IsSet(Physics::CharacterCollisionBits::Sides));
    VERIFY(NearlyEqual(Physics::CharacterContext::GetCharacterFootPosition(character).x, 3.0f));

    // the ground is in group 0, which is accepted by everything, so the character lands on it
    collision = Physics::CharacterContext::MoveCharacter(character, Math::vec3(0.0f, -1.2f, 0.0f), 0.00001f, 0.016f);
    VERIFY(collision.IsSet(Physics::CharacterCollisionBits::Down));
    float const foot = Physics::CharacterContext::GetCharacterFootPosition(character).y;
    VERIFY(foot > -0.001f && foot < 0.5f);

    // a hit on an accepted shape is reported
    VERIFY(hitCount > 0);
    VERIFY(lastHit.character.id == character.id);
    VERIFY(lastHit.actor.id == ground.id);
    VERIFY(lastHit.shape.id != Ids::InvalidId32);

    // a change to the group is applied onto the existing shape, because a controller cannot be built
    // from a new description. The character has to be put back outside the wall first, because one that
    // is already inside a shape does not report a side hit on it
    Physics::CharacterContext::SetCharacterFilter(character, 9, 8);
    VERIFY(pxShape->getSimulationFilterData().word0 == 9);

    Physics::CharacterContext::SetCharacterPosition(character, Math::vec3(0.0f, 1.0f, 0.0f));
    collision = Physics::CharacterContext::MoveCharacter(character, Math::vec3(2.0f, 0.0f, 0.0f), 0.00001f, 0.016f);
    VERIFY(collision.IsSet(Physics::CharacterCollisionBits::Sides));
    VERIFY(Physics::CharacterContext::GetCharacterFootPosition(character).x < 2.5f);

    // releasing the character releases the records before the controller, which is what keeps the
    // actor context from trying to release an actor it does not own
    Physics::ActorId actorRecord = record.actor;
    Physics::ShapeId shapeRecord = record.shape;

    Physics::CharacterContext::DestroyCharacter(character);
    VERIFY(!Physics::CharacterContext::IsValid(character));
    VERIFY(!Physics::ActorContext::IsValid(actorRecord));
    VERIFY(!Physics::ActorContext::IsValidShape(shapeRecord));

    // the scene is left empty before it is destroyed, which is what the next test expects
    Physics::ActorContext::DiscardActor(ground);
    Physics::ActorContext::DiscardActor(wall);

    // A controller that is still in the scene when the scene goes away, which is the state a shutdown
    // leaves a character in. The release reads the shapes off the actor the controller owns, so the
    // controller manager has to go before the scene's actors. This used to take the run down
    Physics::CharacterId left = Physics::CharacterContext::CreateCharacter(Math::vec3(0.0f, 1.5f, 0.0f), info, scene);
    VERIFY(Physics::CharacterContext::IsValid(left));

    Physics::DestroyScene(scene);

    // The record outlives the controller, because the scene going away does not clear it. Nothing reads
    // the physx objects through it after this point
    VERIFY(Physics::CharacterContext::IsValid(left));

    Physics::state.Shutdown();
}

} // namespace Test
//------------------------------------------------------------------------------
