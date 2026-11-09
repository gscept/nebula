#pragma once
//------------------------------------------------------------------------------
/**
    Physics::Cooking

    Cooks geometry descriptions into PhysX meshes. 

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "util/array.h"
#include "physics/geometry.h"

#include "PxPhysicsAPI.h"

namespace Physics
{

/// A cooked convex hull together with the bone it belongs to
struct CookedConvex
{
    physx::PxGeometryHolder geometry;
    IndexT bone;
};

/// Cook geometry into a triangle mesh or a convex mesh, depending on the topology
physx::PxGeometryHolder CookMesh(const MeshGeometryInput& input);

/// Cook one convex hull per bone from skinned geometry
Util::Array<CookedConvex> CookApproxSkin(const MeshGeometryInput& input);

/// Cook height field samples into a physx height field. heightRange is the vertical span
/// in meters and targetSizeX / targetSizeZ the world extent of the field
physx::PxGeometryHolder CookHeightField(const HeightFieldInput& input, float heightRange, float targetSizeX, float targetSizeZ);

} // namespace Physics
//------------------------------------------------------------------------------
