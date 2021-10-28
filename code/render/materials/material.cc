//------------------------------------------------------------------------------
//  material.cc
//  (C) 2018-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "material.h"
#include "shaderconfig.h"
#include "materialcache.h"
namespace Materials
{

MaterialCache* surfacePool = nullptr;

//------------------------------------------------------------------------------
/**
*/
bool
ShaderConfigBeginBatch(ShaderConfig* config, CoreGraphics::BatchGroup::Code batch)
{
    n_assert(config != nullptr);
    return config->BeginBatch(batch);
}

//------------------------------------------------------------------------------
/**
*/
void 
MaterialApply(ShaderConfig* config, const MaterialId id)
{
    n_assert(config != nullptr);
    n_assert(id != MaterialId::Invalid());   
    config->ApplySurface(id);
}

//------------------------------------------------------------------------------
/**
*/
void 
MaterialInstanceApply(ShaderConfig* config, const MaterialInstanceId id)
{
    n_assert(config != nullptr);
    n_assert(id != MaterialInstanceId::Invalid());
    config->ApplyInstance(id);
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderConfigEndBatch(ShaderConfig* config)
{
    n_assert(config != nullptr);
    config->EndBatch();
}

} // namespace Material
