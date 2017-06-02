//------------------------------------------------------------------------------
// modelcontext.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "modelcontext.h"
#include "graphicsentity.h"
#include "resource/resourcemanager.h"

using namespace Resources;
namespace Graphics
{

__ImplementClass(Graphics::ModelContext, 'MOCO', Graphics::GraphicsContext);
__ImplementSingleton(Graphics::ModelContext);
//------------------------------------------------------------------------------
/**
*/
ModelContext::ModelContext()
{
	__ConstructSingleton;

	this->pendingData.fillFunc = [](_Pending& data, IndexT idx)
	{
		data.res = nullptr;
	};
	this->modelData.fillFunc = [](_ModelResult& data, IndexT idx)
	{
		data.model = nullptr;
	};
}

//------------------------------------------------------------------------------
/**
*/
ModelContext::~ModelContext()
{
	__DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
ModelContext::_ModelResult*
ModelContext::RegisterEntity(const Ptr<GraphicsEntity>& entity, _ModelSetup setup)
{
	// allocate loader data
	_Pending* pending = this->AllocateSlice<_Pending>(entity->id, this->pendingData);

	// allocate result data
	_ModelResult* res = this->AllocateSlice<_ModelResult>(entity->id, this->modelData);

	// start loading the resource
	Ptr<ResourceContainer<Models::Model>> model = ResourceManager::Instance()->CreateResource<Models::Model>(setup.res,
		[res, entity, this](const Ptr<Models::Model>& mdl) // success
	{
		res->model = mdl->CreateInstance();
	},
		[entity, this](const Ptr<Models::Model>& mdl) // failed
	{
		// empty
	});

	// set the container in the return data
	pending->res = model;
	return res;
}

//------------------------------------------------------------------------------
/**
*/
void
ModelContext::UnregisterEntity(const Ptr<GraphicsEntity>& entity)
{
	// make sure that the resource manager knows this entity is unregistered, so discard the resource
	_Pending* pending = this->GetSlice(entity->id, this->pendingData);
	ResourceManager::Instance()->DiscardResource<Models::Model>(pending->res);

	// and free up any memory that we might have for this entity
	this->FreeSlice(entity->id, this->pendingData);
	this->FreeSlice(entity->id, this->modelData);
}

} // namespace Graphics