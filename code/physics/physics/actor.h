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
namespace Actors
{
    /// helper functions for creating shapes
    ///
    ActorId CreateBox(Math::vector const& extends, IndexT material, bool dynamic, Math::matrix44 const & transform, IndexT scene = 0);
    ///
    ActorId CreateSphere(float radius, IndexT material, bool dynamic, Math::matrix44 const & transform, IndexT scene = 0);
    ///
    ActorId CreatePlane(Math::plane const& plane, IndexT material, IndexT scene = 0);
    ///
    ActorId CreateCapsule(float radius, float halfHeight, IndexT material, bool dynamic, Math::matrix44 const & transform, IndexT scene = 0);

    ///
    Actor& GetActor(ActorId id);
};





}