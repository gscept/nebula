//------------------------------------------------------------------------------
//  @file geometry.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "physics/geometry.h"

namespace Physics
{

static GeometryLoader geometryLoader;

//------------------------------------------------------------------------------
/**
*/
void
SetGeometryLoader(const GeometryLoader& loader)
{
    geometryLoader = loader;
}

//------------------------------------------------------------------------------
/**
*/
const GeometryLoader&
GetGeometryLoader()
{
    return geometryLoader;
}

} // namespace Physics
//------------------------------------------------------------------------------
