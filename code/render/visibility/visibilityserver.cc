//------------------------------------------------------------------------------
// visibilityserver.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "visibilityserver.h"
#include "models/nodes/shaderstatenode.h"
#include "models/nodes/primitivenode.h"
#include "models/modelcontext.h"

namespace Visibility
{

__ImplementClass(Visibility::VisibilityServer, 'VISE', Core::RefCounted);
__ImplementSingleton(Visibility::VisibilityServer);
//------------------------------------------------------------------------------
/**
*/
VisibilityServer::VisibilityServer() :
	locked(false),
	visibilityDirty(true)
{
	__ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
VisibilityServer::~VisibilityServer()
{
	__DestructSingleton;
}

//------------------------------------------------------------------------------
/**
void
VisibilityServer::RegisterVisibilitySystem(const Ptr<VisibilitySystemBase>& system)
{
	n_assert(this->systems.FindIndex(system) == InvalidIndex);
	this->systems.Append(system);
}
*/


//------------------------------------------------------------------------------
/**
*/
void
VisibilityServer::BeginVisibility()
{
	// create visibility jobs for all observers
	IndexT i;
	for (i = 0; i < this->observers.Size(); i++)
	{
		//Ptr<Jobs::Job> jobs = Jobs::Job::Create();
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VisibilityServer::ApplyVisibility(const Ptr<Graphics::View>& view)
{

}

//------------------------------------------------------------------------------
/**
*/
void
VisibilityServer::RegisterObserver(const Graphics::GraphicsEntityId obs, ObserverMask mask)
{
	n_assert(this->observers.FindIndex(obs) == InvalidIndex);
	n_assert(!this->locked);
	this->observers.Append(obs);
	this->observerMasks.Append(mask);
}

//------------------------------------------------------------------------------
/**
*/
void
VisibilityServer::UnregisterObserver(const Graphics::GraphicsEntityId obs, ObserverMask mask)
{
	IndexT i = this->observers.FindIndex(obs);
	n_assert(i != InvalidIndex);
	n_assert(!this->locked);
	this->observers.EraseIndex(i);
	this->observerMasks.EraseIndex(i);
}

//------------------------------------------------------------------------------
/**
*/
void
VisibilityServer::EnterVisibilityLockstep()
{
	n_assert(!this->locked);
	this->locked = true;
}

//------------------------------------------------------------------------------
/**
*/
void
VisibilityServer::RegisterGraphicsEntity(const Graphics::GraphicsEntityId entity)
{
	n_assert(!this->locked);
	Models::ModelContext* mdlContext = Models::ModelContext::Instance();
	const Models::ModelInstanceId mdl = mdlContext->GetModel(entity);
	n_assert(mdl != Models::ModelInstanceId::Invalid());
	this->models.Append(mdl);
	this->entities.Append(entity);
}

//------------------------------------------------------------------------------
/**
*/
void
VisibilityServer::UnregisterGraphicsEntity(const Graphics::GraphicsEntityId entity)
{
	n_assert(!this->locked);
	IndexT i = this->entities.FindIndex(entity);
	n_assert(i != InvalidIndex);
	this->entities.EraseIndex(i);
	this->models.EraseIndex(i);
}

//------------------------------------------------------------------------------
/**
*/
void
VisibilityServer::LeaveVisibilityLockstep()
{
	n_assert(this->locked);
	this->locked = false;
}

} // namespace Visibility