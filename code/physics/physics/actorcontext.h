#pragma once
//------------------------------------------------------------------------------
/**
    Diverse functions for manipulating physics actors

    @copyright
    (C) 2019-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------

#include "physicsinterface.h"
#include "ids/idgenerationpool.h"
#include "math/vector.h"
#include "math/plane.h"
#include "math/mat4.h"
#include "util/tupleutility.h"
#include "math/transform.h"

namespace Physics
{

struct ShapeHandle
{
    ShapeHandle() : shape(nullptr) {}
    explicit ShapeHandle(physx::PxShape* inShape) : shape(inShape) {}
    bool IsValid() const { return shape != nullptr; }
    bool operator==(const ShapeHandle& other) { return shape == other.shape; }
    physx::PxShape* shape;
};

class PhysxState;
class StreamActorPool;

class ActorContext
{
public:
    /// helper functions for creating shapes
   ///
    static ActorId CreateBox(Math::vector const& extends, IndexT material, ActorType type, Math::mat4 const & transform, IndexT scene = 0);
    ///
    static ActorId CreateSphere(float radius, IndexT material, ActorType type, Math::mat4 const & transform, IndexT scene = 0);
    ///
    static ActorId CreatePlane(Math::plane const& plane, IndexT material, IndexT scene = 0);
    ///
    static ActorId CreateCapsule(float radius, float halfHeight, IndexT material, ActorType type, Math::mat4 const & transform, IndexT scene = 0);
    /// Create a convex hull/mesh from vertex buffer. Assumes "xyz" points (sizeof(float) * 3).
    /// @note You should consider cooking the hulls offline. This is relatively fast for
    /// real-time generation of convex hulls, but won't yield as accurate results as offline.
    static ActorId CreateConvexHull(float* vertices, int numVertices, IndexT material, ActorType type, Math::mat4 const& transform, IndexT scene = 0);

    ///
    static Actor& GetActor(ActorId id);
    ///
    static void SetTransform(ActorId id, Math::mat4 const & transform);
    ///
    static Math::mat4 GetTransform(ActorId id);
    ///
    static void SetPositionOrientation(ActorId id, Math::vec3 const& position, Math::quat const& orientation);
    ///
    static void GetPositionOrientation(ActorId id, Math::vec3& position, Math::quat& orientation);

    ///
    static void SetLinearVelocity(ActorId id, Math::vector speed);
    ///
    static Math::vector GetLinearVelocity(ActorId id);

    ///
    static void SetAngularVelocity(ActorId id, Math::vector speed);
    ///
    static Math::vector GetAngularVelocity(ActorId id);

    /// apply a global impulse vector at the next time step at a global position
    static void ApplyImpulseAtPos(ActorId id, const Math::vector& impulse, const Math::point& pos);

    /// modify collision callback handling
    static void SetCollisionFeedback(ActorId id, CollisionFeedback feedback);

    // shape stuff
    enum { DefaultShapeAlloc = 16 };
    using ShapeArrayType = Util::StackArray<ShapeHandle, DefaultShapeAlloc>;
    static SizeT GetShapeCount(ActorId id);
    static void GetShapes(ActorId id, ShapeArrayType& shapes);
    
    static Math::transform GetShapeTransform(const ShapeHandle& shape);
    static void SetShapeTransform(const ShapeHandle& shape, const Math::transform& transform);


    /// shortcut for getting the pxactor object
    static physx::PxRigidActor* GetPxActor(ActorId id);
    /// shortcut for getting the pxactor object
    static physx::PxRigidDynamic* GetPxDynamic(ActorId id);

    static bool IsValid(ActorId id);

    friend class PhysxState;
    friend class StreamActorPool;
private:
    static Util::Array<Actor> actors;
    static Ids::IdGenerationPool actorPool;
    ///
    static ActorId AllocateActorId(physx::PxRigidActor* pxActor);
    ///
    static ActorId AllocateActorId(physx::PxRigidActor* pxActor, ActorResourceId res);
    ///
    static void DiscardActor(ActorId id);
};

class AggregateContext
{
public:
    static Aggregate& GetAggregate(AggregateId id);
    
    friend class PhysxState;
    friend class StreamActorPool;
private:
    static Util::Array<Aggregate> aggregates;
    static Ids::IdGenerationPool aggPool;
    ///
    static AggregateId AllocateAggregateId(AggregateResourceId res);
    ///
    static void DiscardAggregate(AggregateId id);
};

//------------------------------------------------------------------------------
/**
*/
inline bool
ActorContext::IsValid(ActorId id)
{
    return ActorContext::actorPool.IsValid(id.id);
}

}
