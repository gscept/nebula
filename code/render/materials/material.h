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

class MaterialType;
ID_32_TYPE(MaterialTypeId);
ID_32_TYPE(SurfaceId);
ID_32_32_NAMED_TYPE(SurfaceInstanceId, surface, instance); // 32 bits instance, 32 bits surface

/// begin a material batch
bool MaterialBeginBatch(MaterialType* type, CoreGraphics::BatchGroup::Code batch);
/// begin surface
void MaterialApplySurface(MaterialType* type, const SurfaceId id);
/// apply instance of surface
void MaterialApplySurfaceInstance(MaterialType* type, const SurfaceInstanceId id);
/// end a material batch
void MaterialEndBatch(MaterialType* type);

class SurfacePool;
extern SurfacePool* surfacePool;
} // namespace Materials
