#pragma once
//------------------------------------------------------------------------------
/**
    Navigation::Recast
    
    Helper functions for generating navmesh data
    
    (C) 2022 Individual contributors, see AUTHORS file
*/
#include "io/uri.h"
#include "ids/id.h"
#include "util/blob.h"
#include "math/bbox.h"
#include "nflatbuffer/flatbufferinterface.h"
#include "flat/navigation/navmesh.h"
#include "flat/navigation/navmeshsettings.h"

//------------------------------------------------------------------------------
namespace Navigation
{

namespace Recast
{

//------------------------------------------------------------------------------
/**
*/
bool GenerateNavMesh(NavMeshT const& data, Util::Blob& generated);

}
}
