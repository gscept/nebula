//------------------------------------------------------------------------------
//  material.cc
//  (C) 2018-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "material.h"
#include "materialtype.h"
#include "surfacepool.h"
namespace Materials
{

SurfacePool* surfacePool = nullptr;
MaterialType* currentType = nullptr;
SurfaceId currentSurface = SurfaceId::Invalid();

//------------------------------------------------------------------------------
/**
*/
bool
MaterialBeginBatch(MaterialType* type, CoreGraphics::BatchGroup::Code batch)
{
	n_assert(type != nullptr);
	currentType = type;
	return currentType->BeginBatch(batch);
}

//------------------------------------------------------------------------------
/**
*/
bool 
MaterialBeginSurface(const SurfaceId id)
{
	n_assert(currentType != nullptr);
	n_assert(id != SurfaceId::Invalid());
	currentSurface = id;
	return currentType->BeginSurface(id);
}

//------------------------------------------------------------------------------
/**
*/
void 
MaterialApplySurfaceInstance(const SurfaceInstanceId id)
{
	n_assert(currentType != nullptr);
	n_assert(currentSurface != SurfaceId::Invalid());
	currentType->ApplyInstance(id);
}

//------------------------------------------------------------------------------
/**
*/
void 
MaterialEndSurface()
{
	n_assert(currentSurface != SurfaceId::Invalid());
	currentType->EndSurface();
	currentSurface = SurfaceId::Invalid();
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialEndBatch()
{
	n_assert(currentType != nullptr);
	currentType->EndBatch();
	currentType = nullptr;
}

} // namespace Material
