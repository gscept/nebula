//------------------------------------------------------------------------------
// visibilityserver.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "visibilityserver.h"
#include "models\nodes\statenodeinstance.h"
#include "models\nodes\shapenodeinstance.h"
#include "models\nodes\shapenode.h"

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
VisibilityServer::EnterVisibilityLockstep()
{
	n_assert(!this->locked);
	this->locked = true;
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

	const Util::Array<Ptr<Models::ModelNodeInstance>>& nodes = data->model->GetNodeInstances();
	IndexT i;
	for (i = 0; i < nodes.Size(); i++)
	{
		const Ptr<Models::ModelNodeInstance>& node = nodes[i];
		if (node->IsA(Models::ShapeNodeInstance::RTTI))
		{
			const Ptr<Models::ShapeNodeInstance>& stateNode = node;
			const Ptr<Materials::SurfaceInstance>& surface = stateNode->GetSurfaceInstance();

			// check to see if material is registered, if not, do it
			if (!this->visibilityDatabase.Contains(surface->GetCode()))
			{
				this->visibilityDatabase.Add(surface->GetCode(), Util::Dictionary<Resources::ResourceId, IndexT>());
			}
			else
			{
				// next level, check to see if mesh is registered
				Util::Dictionary<Resources::ResourceId, IndexT>& meshLevel = this->visibilityDatabase[surface->GetCode()];
				const Ptr<Models::ShapeNode>& parentNode = stateNode->GetModelNode();
				if (!meshLevel.Contains(parentNode->GetMeshResourceId()))
				{
					meshLevel.Add(parentNode->GetMeshResourceId())
				}
			}
		}
	}
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
	this->visibilityDatabase.BeginBulkAdd();}

} // namespace Visibility