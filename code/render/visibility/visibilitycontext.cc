//------------------------------------------------------------------------------
// visibilitycontext.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "visibilitycontext.h"
#include "models/modelcontext.h"
#include "models/model.h"
namespace Visibility
{

__ImplementClass(Visibility::VisibilityContext, 'VICX', Graphics::GraphicsContext);
__ImplementSingleton(Visibility::VisibilityContext);
//------------------------------------------------------------------------------
/**
*/
VisibilityContext::VisibilityContext()
{
	__ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
VisibilityContext::~VisibilityContext()
{
	__DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
void
VisibilityContext::Setup(const Graphics::GraphicsEntityId id, VisibilityEntityType type)
{
	const Ids::Id32 cid = this->entitySliceMap[id.id];
	this->visibilityContextAllocator.Get<2>(cid) = type;

	Models::ModelInstanceId mdl = Models::ModelContext::Instance()->GetModel(id);
}

//------------------------------------------------------------------------------
/**
*/
void
VisibilityContext::RegisterEntity(const Graphics::GraphicsEntityId entity)
{
	n_assert(Models::ModelContext::Instance()->IsEntityRegistered(entity));
	return GraphicsContext::RegisterEntity(entity);
}

//------------------------------------------------------------------------------
/**
*/
void
VisibilityContext::DeregisterEntity(const Graphics::GraphicsEntityId entity)
{
	GraphicsContext::DeregisterEntity(entity);
}

} // namespace Visibility