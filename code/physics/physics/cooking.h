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
bool CookMesh(const MeshGeometryInput& input, physx::PxGeometryHolder& out);

/// Cook one convex hull per bone from skinned geometry
Util::Array<CookedConvex> CookApproxSkin(const MeshGeometryInput& input);

/// Cook height field samples into a physx height field. heightRange is the vertical span
/// in meters, sample 0 sits at the field origin and sample 65535 heightRange above it.
/// targetSizeX / targetSizeZ are the world extent of the field
bool CookHeightField(const HeightFieldInput& input, float heightRange, float targetSizeX, float targetSizeZ, physx::PxGeometryHolder& out);

} // namespace Physics
//------------------------------------------------------------------------------
