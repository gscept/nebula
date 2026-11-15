//------------------------------------------------------------------------------
//  @file formats.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "physics/formats/formats.h"

namespace Physics
{

//------------------------------------------------------------------------------
/**
*/
void
RegisterDefaultGeometryLoader()
{
    GeometryLoader loader;

#if WITH_NEBULA_PHYSICS_FORMAT_NVX
    loader.LoadMesh = &Physics::NvxMeshGeometry;
#endif

#if WITH_NEBULA_PHYSICS_FORMAT_DDS
    loader.LoadHeightField = &Physics::DdsHeightField;
#endif

    SetGeometryLoader(loader);
}

} // namespace Physics
//------------------------------------------------------------------------------
