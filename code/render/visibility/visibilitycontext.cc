//------------------------------------------------------------------------------
// visibilitycontext.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "visibilitycontext.h"
#include "models/modelcontext.h"
#include "models/model.h"
#include "models/streammodelpool.h"
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
	const Graphics::ContextEntityId cid = this->entitySliceMap[id.id];

	Models::ModelInstanceId mdl = Models::ModelContext::Instance()->GetModel(id);
	this->visibilityContextAllocator.Get<2>(cid.id) = mdl;
	this->visibilityContextAllocator.Get<3>(cid.id) = type;
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

//------------------------------------------------------------------------------
/**
*/
void
VisibilityContext::OnBeforeFrame(const IndexT frameIndex, const Timing::Time frameTime)
{
	Util::Array<Math::matrix44>& transforms = this->visibilityContextAllocator.GetArray<0>();
	const Util::Array<Models::ModelInstanceId>& models = this->visibilityContextAllocator.GetArray<2>();
	const Util::Array<VisibilityEntityType>& types = this->visibilityContextAllocator.GetArray<3>();
	SizeT i;
	for (i = 0; i < models.Size(); i++)
	{
		if (types[i] == Observee)
		{
			const Models::ModelInstanceId& model = models[i];
			transforms[i] = Models::modelPool->modelInstanceAllocator.Get<2>(model.instance);
		}
		else
		{

		}
	}
}

} // namespace Visibility