//------------------------------------------------------------------------------
//  @file formats.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "physics/formats/formats.h"
#include "io/path.h"

namespace Physics
{

//------------------------------------------------------------------------------
/**
    Registers the export mappings the readers resolve their URNs with. Graphics registers the same
    mappings, but physics doesn't rely on graphics being set up.
*/
void
RegisterDefaultGeometryLoader()
{
    GeometryLoader loader;

#if WITH_NEBULA_PHYSICS_FORMAT_NVX
    IO::Path::AddExportMapping("msh", "nvx");
    loader.LoadMesh = &Physics::NvxMeshGeometry;
#endif

#if WITH_NEBULA_PHYSICS_FORMAT_DDS
    IO::Path::AddExportMapping("tex", "dds");
    loader.LoadHeightField = &Physics::DdsHeightField;
#endif

    SetGeometryLoader(loader);
}

} // namespace Physics
//------------------------------------------------------------------------------
