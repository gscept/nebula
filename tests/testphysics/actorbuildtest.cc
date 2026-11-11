//------------------------------------------------------------------------------
//  @file actorbuildtest.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "testbase/testcase.h"
#include "actorbuildtest.h"

#include "physicsinterface.h"
#include "physics/geometry.h"
#include "physics/cooking.h"
#include "physics/physxstate.h"
#include "physics/actorcontext.h"

#include "PxPhysicsAPI.h"

using namespace physx;

namespace Test
{
__ImplementClass(Test::ActorBuildTest, 'ABTC', Test::TestCase);

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
    A loader that fills a small tetrahedron, the way a format reader would
*/
static bool
LoadTestMesh(Util::StringAtom resource, IndexT primGroup, Physics::MeshGeometryInput& out)
{
    if (resource == Util::StringAtom("test:bad_indices"))
    {
        // triangle indices that are not triplets, no cook can be built from this
        out.indexType = Physics::IndexType::Index16;
        out.topology = Physics::MeshTopology_Triangles;

        out.positions.Append(0.0f); out.positions.Append(0.0f); out.positions.Append(0.0f);
        out.positions.Append(1.0f); out.positions.Append(0.0f); out.positions.Append(0.0f);
        out.positions.Append(0.0f); out.positions.Append(1.0f); out.positions.Append(0.0f);
        out.positions.Append(0.0f); out.positions.Append(0.0f); out.positions.Append(1.0f);
        out.vertexCount = 4;

        out.indices.Append(0); out.indices.Append(1);
        out.indexCount = 2;
        return true;
    }

    if (resource != Util::StringAtom("test:tetrahedron"))
        return false;

    out.indexType = Physics::IndexType::Index16;
    out.topology = Physics::MeshTopology_Convex;

    out.positions.Append(0.0f); out.positions.Append(0.0f); out.positions.Append(0.0f);
    out.positions.Append(1.0f); out.positions.Append(0.0f); out.positions.Append(0.0f);
    out.positions.Append(0.0f); out.positions.Append(1.0f); out.positions.Append(0.0f);
    out.positions.Append(0.0f); out.positions.Append(0.0f); out.positions.Append(1.0f);
    out.vertexCount = 4;

    out.indices.Append(0); out.indices.Append(1); out.indices.Append(2);
    out.indices.Append(1); out.indices.Append(3); out.indices.Append(2);
    out.indices.Append(1); out.indices.Append(0); out.indices.Append(3);
    out.indices.Append(2); out.indices.Append(3); out.indices.Append(0);
    out.indexCount = 12;
    return true;
}

//------------------------------------------------------------------------------
/**
    Build actors and shapes from plain descriptions, without going through a resource
*/
void
ActorBuildTest::Run()
{
    Physics::state.Setup();

    // the fallback material is created by Physics::Setup(), which needs the resource server,
    // so it is created here instead
    Physics::state.FallbackMaterial.material = Physics::state.physics->createMaterial(0.7f, 0.7f, 0.3f);
    Physics::state.FallbackMaterial.density = 1.0f;

    Physics::GeometryLoader loader;
    loader.LoadMesh = &LoadTestMesh;
    Physics::SetGeometryLoader(loader);

    IndexT scene = Physics::CreateScene();

    Physics::BodyDescription body;
    body.type = Physics::Dynamic;
    body.scene = scene;
    body.collisionGroup = 17;
    body.feedback = Physics::CollisionFeedback_Single;

    Physics::ActorId actor = Physics::ActorContext::CreateActor(body);
    VERIFY(Physics::ActorContext::IsValid(actor));
    VERIFY(Physics::ActorContext::GetActor(actor).shapes.Size() == 0);
    VERIFY(Physics::ActorContext::GetActor(actor).collisionGroup == 17);
    VERIFY(Physics::ActorContext::GetActor(actor).feedback == Physics::CollisionFeedback_Single);

    PxRigidActor* pxActor = Physics::ActorContext::GetPxActor(actor);
    VERIFY(pxActor != nullptr);
    VERIFY(pxActor->getNbShapes() == 0);

    // a description with an unknown material name falls back to the fallback material
    Physics::ColliderDescription collider;
    collider.type = Physics::ColliderType_Box;
    collider.transform = Math::transform(Math::vec3(2.0f, 0.0f, 0.0f));
    collider.boxExtents = Math::vec3(1.0f, 1.0f, 1.0f);
    collider.materialId = Physics::LookupMaterial("test:unknown_material");

    Physics::ShapeId shape = Physics::ActorContext::AddShape(actor, collider);
    VERIFY(Physics::ActorContext::IsValidShape(shape));
    VERIFY(Physics::ActorContext::GetActor(actor).shapes.Size() == 1);
    VERIFY(pxActor->getNbShapes() == 1);

    PxShape* pxShape = Physics::ActorContext::GetShape(shape).shape;
    VERIFY(pxShape != nullptr);
    if (pxShape != nullptr)
    {
        VERIFY(pxShape->getGeometry().getType() == PxGeometryType::eBOX);
        const PxBoxGeometry& boxGeom = static_cast<const PxBoxGeometry&>(pxShape->getGeometry());
        VERIFY(NearlyEqual(boxGeom.halfExtents.x, 1.0f));
        VERIFY(NearlyEqual(pxShape->getLocalPose().p.x, 2.0f));
        VERIFY(pxShape->getSimulationFilterData().word0 == 17);
        VERIFY(pxShape->getSimulationFilterData().word1 == Physics::CollisionFeedback_Single);

        PxMaterial* materials[1];
        VERIFY(pxShape->getMaterials(materials, 1) == 1);
        VERIFY(materials[0] == Physics::state.FallbackMaterial.material);
    }

    // a dynamic body with no mass gets it from the shape volume and the material density
    PxRigidDynamic* dynamic = static_cast<PxRigidDynamic*>(pxActor);
    VERIFY(NearlyEqual(dynamic->getMass(), 8.0f));

    // the same description does not rebuild the shape
    VERIFY(Physics::ActorContext::UpdateShape(shape, collider));
    VERIFY(Physics::ActorContext::GetShape(shape).shape == pxShape);

    // changing the shape parameters rebuilds it
    collider.boxExtents = Math::vec3(1.0f, 2.0f, 1.0f);
    VERIFY(Physics::ActorContext::UpdateShape(shape, collider));
    PxShape* rebuilt = Physics::ActorContext::GetShape(shape).shape;
    VERIFY(rebuilt != nullptr && rebuilt != pxShape);
    VERIFY(pxActor->getNbShapes() == 1);
    if (rebuilt != nullptr)
    {
        const PxBoxGeometry& rebuiltGeom = static_cast<const PxBoxGeometry&>(rebuilt->getGeometry());
        VERIFY(NearlyEqual(rebuiltGeom.halfExtents.y, 2.0f));
    }

    // an explicit mass overrides the derived one, and the filter data is applied onto the shapes
    body.mass = 12.0f;
    body.collisionGroup = 22;
    Physics::ActorContext::UpdateBody(actor, body);
    VERIFY(NearlyEqual(dynamic->getMass(), 12.0f));
    if (rebuilt != nullptr)
        VERIFY(rebuilt->getSimulationFilterData().word0 == 22);

    // damping, gravity and ccd
    body.linearDamping = 0.25f;
    body.angularDamping = 0.5f;
    body.useGravity = false;
    body.ccd = true;
    Physics::ActorContext::UpdateBody(actor, body);
    VERIFY(NearlyEqual(dynamic->getLinearDamping(), 0.25f));
    VERIFY(NearlyEqual(dynamic->getAngularDamping(), 0.5f));
    VERIFY(dynamic->getActorFlags() & PxActorFlag::eDISABLE_GRAVITY);
    VERIFY(dynamic->getRigidBodyFlags() & PxRigidBodyFlag::eENABLE_CCD);

    // triggers are lowered from the simulation and raised as triggers
    collider.isTrigger = true;
    Physics::ActorContext::UpdateShape(shape, collider);
    pxShape = Physics::ActorContext::GetShape(shape).shape;
    VERIFY(pxShape->getFlags() & PxShapeFlag::eTRIGGER_SHAPE);
    VERIFY(!(pxShape->getFlags() & PxShapeFlag::eSIMULATION_SHAPE));

    // mesh colliders read their geometry through the registered loader
    Physics::ColliderDescription meshCollider;
    meshCollider.type = Physics::ColliderType_Mesh;
    meshCollider.resource = Util::StringAtom("test:tetrahedron");
    meshCollider.topology = Physics::MeshTopology_Convex;

    Physics::ShapeId meshShape = Physics::ActorContext::AddShape(actor, meshCollider);
    VERIFY(Physics::ActorContext::IsValidShape(meshShape));
    VERIFY(pxActor->getNbShapes() == 2);
    PxShape* meshPxShape = Physics::ActorContext::GetShape(meshShape).shape;
    VERIFY(meshPxShape->getGeometry().getType() == PxGeometryType::eCONVEXMESH);

    // a resource the loader does not know can not be built
    meshCollider.resource = Util::StringAtom("test:unknown_resource");
    Physics::ShapeId failed = Physics::ActorContext::AddShape(actor, meshCollider);
    VERIFY(!Physics::ActorContext::IsValidShape(failed));

    // updating a shape with unreadable geometry keeps the geometry that is currently cooked
    VERIFY(!Physics::ActorContext::UpdateShape(meshShape, meshCollider));
    VERIFY(Physics::ActorContext::GetShape(meshShape).shape->getGeometry().getType() == PxGeometryType::eCONVEXMESH);

    // and when the resource becomes readable the shape is rebuilt
    meshCollider.resource = Util::StringAtom("test:tetrahedron");
    VERIFY(Physics::ActorContext::UpdateShape(meshShape, meshCollider));

    // geometry that can not be cooked fails cleanly, and the shape that is currently
    // working survives the attempt
    meshCollider.resource = Util::StringAtom("test:bad_indices");
    meshCollider.topology = Physics::MeshTopology_Triangles;
    VERIFY(!Physics::ActorContext::UpdateShape(meshShape, meshCollider));
    VERIFY(Physics::ActorContext::GetShape(meshShape).shape->getGeometry().getType() == PxGeometryType::eCONVEXMESH);

    Physics::ColliderDescription degenerate;
    degenerate.type = Physics::ColliderType_Mesh;
    degenerate.resource = Util::StringAtom("test:bad_indices");
    degenerate.topology = Physics::MeshTopology_Triangles;
    VERIFY(!Physics::ActorContext::IsValidShape(Physics::ActorContext::AddShape(actor, degenerate)));

    // the trigger below is the first shape on its actor and must contribute nothing to the
    // derived mass. If the density list loses its alignment with the actor's shape order, the
    // box's density ends up on the sphere and the mass comes out nowhere near 8
    Physics::BodyDescription derived;
    derived.type = Physics::Dynamic;
    derived.scene = scene;
    Physics::ActorId derivedActor = Physics::ActorContext::CreateActor(derived);

    Physics::ColliderDescription bigTrigger;
    bigTrigger.type = Physics::ColliderType_Sphere;
    bigTrigger.sphereRadius = 2.0f;
    bigTrigger.isTrigger = true;
    VERIFY(Physics::ActorContext::IsValidShape(Physics::ActorContext::AddShape(derivedActor, bigTrigger)));

    Physics::ColliderDescription unitBox;
    unitBox.type = Physics::ColliderType_Box;
    unitBox.boxExtents = Math::vec3(1.0f, 1.0f, 1.0f);
    VERIFY(Physics::ActorContext::IsValidShape(Physics::ActorContext::AddShape(derivedActor, unitBox)));

    PxRigidActor* derivedPx = Physics::ActorContext::GetPxActor(derivedActor);
    VERIFY(derivedPx != nullptr && NearlyEqual(static_cast<PxRigidDynamic*>(derivedPx)->getMass(), 8.0f));

    Physics::ActorContext::DiscardActor(derivedActor);

    // removing a shape detaches it from the actor
    Physics::ActorContext::RemoveShape(meshShape);
    VERIFY(!Physics::ActorContext::IsValidShape(meshShape));
    VERIFY(pxActor->getNbShapes() == 1);
    VERIFY(Physics::ActorContext::GetActor(actor).shapes.Size() == 1);

    Physics::ActorContext::RemoveShape(shape);
    VERIFY(!Physics::ActorContext::IsValidShape(shape));
    VERIFY(pxActor->getNbShapes() == 0);

    // a static body has no mass, but the filter data is still applied onto its shapes
    Physics::BodyDescription staticBody;
    staticBody.type = Physics::Static;
    staticBody.scene = scene;
    staticBody.collisionGroup = 41;
    staticBody.feedback = Physics::CollisionFeedback_Full;
    staticBody.transform = Math::transform(Math::vec3(0.0f, 5.0f, 0.0f));

    Physics::ActorId staticActor = Physics::ActorContext::CreateActor(staticBody);
    VERIFY(Physics::ActorContext::IsValid(staticActor));

    PxRigidActor* pxStatic = Physics::ActorContext::GetPxActor(staticActor);
    VERIFY(pxStatic != nullptr);
    VERIFY(pxStatic != nullptr && pxStatic->is<PxRigidStatic>());

    Physics::ColliderDescription staticCollider;
    staticCollider.type = Physics::ColliderType_Sphere;
    staticCollider.sphereRadius = 1.5f;

    Physics::ShapeId staticShape = Physics::ActorContext::AddShape(staticActor, staticCollider);
    VERIFY(Physics::ActorContext::IsValidShape(staticShape));
    if (Physics::ActorContext::IsValidShape(staticShape))
    {
        VERIFY(Physics::ActorContext::GetShape(staticShape).shape->getSimulationFilterData().word0 == 41);
        VERIFY(Physics::ActorContext::GetShape(staticShape).shape->getSimulationFilterData().word1 == Physics::CollisionFeedback_Full);
    }

    // the pose of a static body comes from the description
    staticBody.transform = Math::transform(Math::vec3(1.0f, 2.0f, 3.0f));
    Physics::ActorContext::UpdateBody(staticActor, staticBody);
    if (pxStatic != nullptr)
    {
        PxTransform pose = pxStatic->getGlobalPose();
        VERIFY(NearlyEqual(pose.p.x, 1.0f));
        VERIFY(NearlyEqual(pose.p.y, 2.0f));
        VERIFY(NearlyEqual(pose.p.z, 3.0f));
    }

    Physics::ActorContext::RemoveShape(staticShape);
    Physics::ActorContext::DiscardActor(staticActor);
    VERIFY(!Physics::ActorContext::IsValid(staticActor));

    // destroying the actor drops the shapes it owns as well
    Physics::ShapeId last = Physics::ActorContext::AddShape(actor, collider);
    VERIFY(Physics::ActorContext::IsValidShape(last));
    Physics::ActorContext::DiscardActor(actor);
    VERIFY(!Physics::ActorContext::IsValid(actor));
    VERIFY(!Physics::ActorContext::IsValidShape(last));

    // only the scene that is stepped moves. This is what keeps a world in editor mode from simulating
    // while the worlds that are running gameplay do
    IndexT pausedScene = Physics::CreateScene();

    Physics::BodyDescription falling;
    falling.type = Physics::Dynamic;
    falling.transform = Math::transform(Math::vec3(0.0f, 10.0f, 0.0f));

    Physics::ColliderDescription fallingShape;
    fallingShape.type = Physics::ColliderType_Sphere;
    fallingShape.sphereRadius = 0.5f;

    falling.scene = scene;
    Physics::ActorId runningActor = Physics::ActorContext::CreateActor(falling);
    VERIFY(Physics::ActorContext::IsValid(runningActor));
    Physics::ActorContext::AddShape(runningActor, fallingShape);

    falling.scene = pausedScene;
    Physics::ActorId pausedActor = Physics::ActorContext::CreateActor(falling);
    VERIFY(Physics::ActorContext::IsValid(pausedActor));
    Physics::ActorContext::AddShape(pausedActor, fallingShape);

    VERIFY(NearlyEqual(Physics::ActorContext::GetPxActor(pausedActor)->getGlobalPose().p.y, 10.0f));

    // the sync step is limited to 5 physics frames per call, so step the running scene a few times
    for (int i = 0; i < 10; i++)
        Physics::state.Update(0.016f, scene);

    // the actor in the stepped scene moved, the one in the scene that was left alone did not
    VERIFY(Physics::ActorContext::GetPxActor(runningActor)->getGlobalPose().p.y < 9.99f);
    VERIFY(NearlyEqual(Physics::ActorContext::GetPxActor(pausedActor)->getGlobalPose().p.y, 10.0f));

    // a scene that is not simulating can still be ended and flushed, which is what makes it safe to call
    // those for every world each frame regardless of mode
    Physics::state.EndSimulating(pausedScene);
    Physics::state.FlushSimulation(pausedScene);
    VERIFY(NearlyEqual(Physics::ActorContext::GetPxActor(pausedActor)->getGlobalPose().p.y, 10.0f));

    Physics::ActorContext::DiscardActor(runningActor);
    Physics::ActorContext::DiscardActor(pausedActor);
    Physics::DestroyScene(pausedScene);

    Physics::state.Shutdown();
}

} // namespace Test
//------------------------------------------------------------------------------
