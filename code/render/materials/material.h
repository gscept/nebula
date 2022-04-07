#pragma once
//------------------------------------------------------------------------------
/**
    Materials represent a set of settings and a correlated type, which tells the engine which shader to use and how

    @copyright
    (C) 2018-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/id.h"
#include "ids/idallocator.h"
#include "resources/resourceid.h"
#include "coregraphics/batchgroup.h"
#include <functional>
namespace Materials
{

class ShaderConfig;
ID_32_TYPE(MaterialId);
ID_32_32_NAMED_TYPE(MaterialInstanceId, material, instance); // 32 bits instance, 32 bits surface

class MaterialCache;
extern MaterialCache* materialCache;
} // namespace Materials
