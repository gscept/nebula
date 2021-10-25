//------------------------------------------------------------------------------
// model.cc
// (C)2017-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "model.h"
#include "streammodelcache.h"
#include "coregraphics/config.h"

namespace Models
{

StreamModelCache* modelPool;

//------------------------------------------------------------------------------
/**
*/
const ModelId
CreateModel(const ResourceCreateInfo& info)
{
	return modelPool->CreateResource(info.resource, nullptr, 0, info.tag, info.successCallback, info.failCallback, !info.async).As<ModelId>();
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyModel(const ModelId id)
{
	modelPool->DiscardResource(id);
}

} // namespace Models
