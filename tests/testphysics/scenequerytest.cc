//------------------------------------------------------------------------------
//  @file scenequerytest.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "testbase/testcase.h"
#include "scenequerytest.h"

#include "physicsinterface.h"
#include "physics/physxstate.h"
#include "physics/actorcontext.h"
#include "physics/scenequery.h"

#include "PxPhysicsAPI.h"

using namespace physx;

namespace Test
{
__ImplementClass(Test::SceneQueryTest, 'SQTC', Test::TestCase);

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

//------------------------------------------------------------------------------
/**
*/
void
SceneQueryTest::Run()
{
    Physics::state.Setup();

    // the fallback material is created by Physics::Setup(), which needs the resource server,
    // so it is created here instead
    Physics::state.FallbackMaterial.material = Physics::state.physics->createMaterial(0.7f, 0.7f, 0.3f);
    Physics::state.FallbackMaterial.density = 1.0f;

    IndexT scene = Physics::CreateScene();

    // A static box with unit extents at y 5, so its top face is at y 6. The actor names the entity
    // its userData holds, and the shape names the id this context allocated for it
    Physics::BodyDescription body;
    body.type = Physics::Static;
    body.scene = scene;
    body.transform = Math::transform(Math::vec3(0.0f, 5.0f, 0.0f));
    body.userData = 1234;

    Physics::ActorId actor = Physics::ActorContext::CreateActor(body);
    VERIFY(Physics::ActorContext::IsValid(actor));

    Physics::ColliderDescription collider;
    collider.type = Physics::ColliderType_Box;
    collider.boxExtents = Math::vec3(1.0f, 1.0f, 1.0f);

    Physics::ShapeId shape = Physics::ActorContext::AddShape(actor, collider);
    VERIFY(Physics::ActorContext::IsValidShape(shape));

    // A query reads the query filter data, which is the same words the simulation filter reads. A shape
    // with the default group has zero filter data, which is why the query filter in physics/scenequery.cc
    // does not rely on PhysX's own data filter
    PxShape* pxShape = Physics::ActorContext::GetShape(shape).shape;
    VERIFY(pxShape != nullptr);
    if (pxShape != nullptr)
        VERIFY(pxShape->getQueryFilterData().word0 == 0);

    Physics::RaycastHit hit;
    Physics::QueryFilter filter;

    // Straight down from y 10. The top face is 4 away
    VERIFY(Physics::Raycast(scene, Math::point(0.0f, 10.0f, 0.0f), Math::vector(0.0f, -1.0f, 0.0f), 6.0f, hit, filter));
    VERIFY(hit.entity == 1234);
    VERIFY(hit.shapeId == shape.id);
    VERIFY(NearlyEqual(hit.distance, 4.0f));
    VERIFY(NearlyEqual(hit.point.y, 6.0f));
    // PhysX reports the surface normal pointing out of the shape, so a ray hitting the top face from
    // above gets the up normal rather than the ray's own direction
    VERIFY(NearlyEqual(hit.normal.y, 1.0f));

    // A ray that stops before it reaches the shape
    VERIFY(!Physics::Raycast(scene, Math::point(0.0f, 10.0f, 0.0f), Math::vector(0.0f, -1.0f, 0.0f), 3.0f, hit, filter));

    // A ray with no length answers false rather than reporting a hit at the origin
    VERIFY(!Physics::Raycast(scene, Math::point(0.0f, 10.0f, 0.0f), Math::vector(0.0f, -1.0f, 0.0f), 0.0f, hit, filter));

    // The segment is the direction and the distance, so two points do not have to be normalized first
    VERIFY(Physics::SegmentCast(scene, Math::point(0.0f, 10.0f, 0.0f), Math::point(0.0f, 5.0f, 0.0f), hit, filter));
    VERIFY(NearlyEqual(hit.distance, 4.0f));
    VERIFY(hit.shapeId == shape.id);

    // A segment with no length
    VERIFY(!Physics::SegmentCast(scene, Math::point(0.0f, 10.0f, 0.0f), Math::point(0.0f, 10.0f, 0.0f), hit, filter));

    // A second box, far enough from the first that a ray can be aimed at one of them. This is the only
    // shape in the scene with a collision group, which is what a mask is matched against
    Physics::BodyDescription grouped;
    grouped.type = Physics::Static;
    grouped.scene = scene;
    grouped.collisionGroup = 8;
    grouped.transform = Math::transform(Math::vec3(0.0f, 5.0f, 20.0f));
    grouped.userData = 4321;

    Physics::ActorId groupedActor = Physics::ActorContext::CreateActor(grouped);
    VERIFY(Physics::ActorContext::IsValid(groupedActor));

    Physics::ColliderDescription groupedCollider;
    groupedCollider.type = Physics::ColliderType_Box;
    groupedCollider.boxExtents = Math::vec3(1.0f, 1.0f, 1.0f);

    Physics::ShapeId groupedShape = Physics::ActorContext::AddShape(groupedActor, groupedCollider);
    VERIFY(Physics::ActorContext::IsValidShape(groupedShape));

    PxShape* groupedPxShape = Physics::ActorContext::GetShape(groupedShape).shape;
    VERIFY(groupedPxShape != nullptr);
    if (groupedPxShape != nullptr)
        VERIFY(groupedPxShape->getQueryFilterData().word0 == 8);

    Math::point origin(0.0f, 10.0f, 20.0f);
    Math::vector down(0.0f, -1.0f, 0.0f);

    // A query with no group accepts everything, including a shape that has one
    VERIFY(Physics::Raycast(scene, origin, down, 6.0f, hit, filter));
    VERIFY(hit.entity == 4321);

    // A mask that does not include the shape's group rejects it. The default group 0 is accepted by every
    // query, so the query has to come from a group of its own for the mask to be read at all
    filter.group = 1;
    filter.mask = 16;
    VERIFY(!Physics::Raycast(scene, origin, down, 6.0f, hit, filter));

    filter.mask = 8;
    VERIFY(Physics::Raycast(scene, origin, down, 6.0f, hit, filter));
    VERIFY(hit.entity == 4321);

    // The excluded actor is named by its id. A ground check needs this because the ray starts inside the
    // shape the caller owns
    filter.group = 0;
    filter.mask = 0xFFFFFFFF;
    filter.excludeActorId = groupedActor;
    VERIFY(!Physics::Raycast(scene, origin, down, 6.0f, hit, filter));

    filter.excludeActorId = actor;
    VERIFY(Physics::Raycast(scene, origin, down, 6.0f, hit, filter));
    VERIFY(hit.entity == 4321);

    // An id that is not in the pool excludes nothing rather than asserting
    filter.excludeActorId = Physics::ActorId(Ids::InvalidId32);
    VERIFY(Physics::Raycast(scene, origin, down, 6.0f, hit, filter));

    // A query is only allowed while the scene is not simulating, which is the point the physics frame event
    // runs them from. The step here fetches its results before the query
    Physics::state.Step(0.016f, scene);
    VERIFY(Physics::Raycast(scene, origin, down, 6.0f, hit, filter));

    Physics::ActorContext::RemoveShape(groupedShape);
    Physics::ActorContext::DiscardActor(groupedActor);
    Physics::ActorContext::RemoveShape(shape);
    Physics::ActorContext::DiscardActor(actor);
    VERIFY(!Physics::ActorContext::IsValid(actor));

    Physics::DestroyScene(scene);
    Physics::state.Shutdown();
}

} // namespace Test
//------------------------------------------------------------------------------
