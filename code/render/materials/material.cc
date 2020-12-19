//------------------------------------------------------------------------------
//  material.cc
//  (C) 2018-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "material.h"
#include "materialtype.h"
#include "surfacepool.h"
namespace Materials
{

SurfacePool* surfacePool = nullptr;

//------------------------------------------------------------------------------
/**
*/
bool
MaterialBeginBatch(MaterialType* type, CoreGraphics::BatchGroup::Code batch)
{
    n_assert(type != nullptr);
    return type->BeginBatch(batch);
}

//------------------------------------------------------------------------------
/**
*/
void 
MaterialApplySurface(MaterialType* type, const SurfaceId id)
{
    n_assert(type != nullptr);
    n_assert(id != SurfaceId::Invalid());   
    type->ApplySurface(id);
}

//------------------------------------------------------------------------------
/**
*/
void 
MaterialApplySurfaceInstance(MaterialType* type, const SurfaceInstanceId id)
{
    n_assert(type != nullptr);
    n_assert(id != SurfaceInstanceId::Invalid());
    type->ApplyInstance(id);
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialEndBatch(MaterialType* type)
{
    n_assert(type != nullptr);
    type->EndBatch();
}

} // namespace Material
