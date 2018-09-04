//------------------------------------------------------------------------------
// model.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "model.h"
#include "streammodelpool.h"
#include "coregraphics/config.h"

namespace Models
{

StreamModelPool* modelPool;

//------------------------------------------------------------------------------
/**
*/
const ModelId
CreateModel(const ModelCreateInfo& info)
{
	return modelPool->CreateResource(info.resource, info.tag, info.successCallback, info.failCallback, !info.async);
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyModel(const ModelId id)
{
	modelPool->DiscardResource(id);
}

//------------------------------------------------------------------------------
/**
*/
const ModelInstanceId
CreateModelInstance(const ModelId mdl)
{
	return modelPool->CreateModelInstance(mdl);
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyModelInstance(const ModelInstanceId id)
{
	modelPool->DestroyModelInstance(id);
}

} // namespace Models