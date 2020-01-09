#pragma once
//------------------------------------------------------------------------------
/**
	Materials represent a set of settings and a correlated type, which tells the engine which shader to use and how

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
bool MaterialBeginSurface(const SurfaceId id);
/// apply instance of surface
void MaterialApplySurfaceInstance(const SurfaceInstanceId id);
/// end surface
void MaterialEndSurface();
/// end a material batch
void MaterialEndBatch();

extern MaterialType* currentType;

class SurfacePool;
extern SurfacePool* surfacePool;
} // namespace Materials
