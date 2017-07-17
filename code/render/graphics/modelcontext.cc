//------------------------------------------------------------------------------
// modelcontext.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "modelcontext.h"
#include "graphicsentity.h"
#include "resources/resourcemanager.h"

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
Graphics::ContextId
ModelContext::Register(const EntityId entity, const Resources::ResourceName& modelName)
{
	Graphics::ContextId id;
	if (!this->contextIdPool.Allocate(id))
	{
		this->modelResources.Append(Ids::InvalidId32);
	}

	// create resource
	this->modelResources[Ids::Index(id)] = Resources::ResourceManager::Instance()->CreateResource(modelName, ""_atm);
	return id;
}

//------------------------------------------------------------------------------
/**
*/
void
ModelContext::Unregister(const EntityId entity)
{
	this->contextIdPool.Deallocate(entity);
}

} // namespace Graphics