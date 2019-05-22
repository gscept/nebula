#pragma once
//------------------------------------------------------------------------------
/**
    Actor wrappers/helpers for PhysX
    Actor class is mainly aimed at providing simulation callbacks to respective
    nebula objects

    (C) 2019 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------

#include "physicsinterface.h"

//------------------------------------------------------------------------------
namespace Physics
{

//------------------------------------------------------------------------------
class Actor
{
public:

    /// helper functions for creating shapes
    ///
    static Actor* CreateBox(Math::vector const& extends, IndexT material, bool dynamic, Math::matrix44 const & transform, IndexT scene = 0);
    ///
    static Actor* CreateSphere(float radius, IndexT material, bool dynamic, Math::matrix44 const & transform, IndexT scene = 0);
    ///
    static Actor* CreatePlane(Math::plane const& plane, IndexT material, bool dynamic, Math::matrix44 const & transform, IndexT scene = 0);
    ///
    static Actor* CreateCapsule(float radius, float halfHeight, IndexT material, bool dynamic, Math::matrix44 const & transform, IndexT scene = 0);

    physx::PxRigidActor* actor;

};





}