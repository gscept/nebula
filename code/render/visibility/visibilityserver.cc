//------------------------------------------------------------------------------
// visibilityserver.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "visibilityserver.h"

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
*/
void
VisibilityServer::BeginVisibility()
{

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
VisibilityServer::EnterVisibilityLockstep()
{
	n_assert(!this->locked);
	this->locked = true;
	this->visibilityDatabase.BeginBulkAdd();
}

//------------------------------------------------------------------------------
/**
*/
void
VisibilityServer::RegisterGraphicsEntity(const Ptr<Graphics::GraphicsEntity>& entity, Graphics::ModelContext::_ModelResult* data)
{
	n_assert(!this->locked);
	n_assert(this->entities.FindIndex(entity) == InvalidIndex);
	this->entities.Append(entity);
	this->models.Append(data);
	this->visibilityDirty = true;
}

//------------------------------------------------------------------------------
/**
*/
void
VisibilityServer::UnregisterGraphicsEntity(const Ptr<Graphics::GraphicsEntity>& entity)
{
	n_assert(!this->locked);
	IndexT i = this->entities.FindIndex(entity);
	n_assert(i != InvalidIndex);
	this->entities.EraseIndex(i);
	this->models.EraseIndex(i);
	this->visibilityDirty = true;
}

//------------------------------------------------------------------------------
/**
*/
void
VisibilityServer::LeaveVisibilityLockstep()
{
	n_assert(this->locked);
	this->locked = false;
	this->visibilityDatabase.EndBulkAdd();

	// when we are leaving the visibility lockstep, we must notify our observers that the scene has changed
	if (this->visibilityDirty)
	{
		IndexT i;
		for (i = 0; i < this->observers.Size(); i++)
		{
			this->observers[i]->OnVisibilityDatabaseChanged();
		}
		this->visibilityDirty = false;
	}
}

} // namespace Visibility