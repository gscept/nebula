#pragma once
//------------------------------------------------------------------------------
/**
    Physics::Formats

    Asset readers which fill the geometry descriptions.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "physics/geometry.h"

namespace Physics
{

#if WITH_NEBULA_PHYSICS_FORMAT_NVX
/// Fill a mesh geometry description from a nvx3 mesh file
bool NvxMeshGeometry(Util::StringAtom resource, IndexT primGroup, MeshGeometryInput& out);
#endif

#if WITH_NEBULA_PHYSICS_FORMAT_DDS
/// Fill a height field description from a DDS heightmap
bool DdsHeightField(Util::StringAtom resource, HeightFieldInput& out);
#endif

/// Register the compiled-in readers as the geometry loader. A project that uses
/// another format calls Physics::SetGeometryLoader() instead.
void RegisterDefaultGeometryLoader();

} // namespace Physics
//------------------------------------------------------------------------------
