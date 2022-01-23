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
ID_32_32_NAMED_TYPE(MaterialInstanceId, surface, instance); // 32 bits instance, 32 bits surface

/// begin a material batch
bool ShaderConfigBeginBatch(ShaderConfig* config, CoreGraphics::BatchGroup::Code batch);
/// begin surface
void MaterialApply(ShaderConfig* config, const MaterialId id);
/// apply instance of surface
void MaterialInstanceApply(ShaderConfig* config, const MaterialInstanceId id);
/// end a material batch
void ShaderConfigEndBatch(ShaderConfig* config);

class MaterialCache;
extern MaterialCache* surfacePool;
} // namespace Materials
