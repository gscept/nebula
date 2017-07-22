//------------------------------------------------------------------------------
// resourcememoryloader.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "resourcememorypool.h"

namespace Resources
{

__ImplementAbstractClass(Resources::ResourceMemoryPool, 'RMLO', Resources::ResourcePool);
//------------------------------------------------------------------------------
/**
*/
ResourceMemoryPool::ResourceMemoryPool()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
ResourceMemoryPool::~ResourceMemoryPool()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
Resources::ResourceId
ResourceMemoryPool::ReserveResource(const ResourceName& res, const Util::StringAtom& tag)
{
	n_assert(this->resourceClass != nullptr);
	n_assert(this->resourceClass->IsDerivedFrom(Resource::RTTI));

	Ids::Id64 ret;
	Ids::Id32 instanceId = this->resourceInstanceIndexPool.Alloc(); // this is the ID of the container
	Ids::Id24 resourceId; // this is the id of the resource	
	IndexT i = this->ids.FindIndex(res);

	if (i == InvalidIndex)
	{
		// create new resource id, if need be, grow the container list
		resourceId = this->resourceIndexPool.Alloc();
		if (resourceId >= (uint32_t)this->containers.Size())
		{
			this->containers.Resize(this->containers.Size() + this->resourceIndexPool.GetGrow());
			this->usage.Resize(this->usage.Size() + this->resourceIndexPool.GetGrow());
			this->names.Resize(this->names.Size() + this->resourceIndexPool.GetGrow());
		}

		// grab container
		ResourceContainer& container = this->containers[resourceId];
		container.error = nullptr;
		container.placeholder = nullptr;
		container.resource = (Resource*)this->resourceClass->Create();
		container.resource->resourceName = res;
		container.resource->tag = tag;
		container.resource->state = Resource::Loaded; // we set these to loaded immediately

		// add the resource name to the resource id (if valid)
		if (res.IsValid()) this->ids.Add(res, resourceId);
		this->names[resourceId] = res;
		this->usage[resourceId] = 1;
		ret = Ids::Id::MakeId64(instanceId, Ids::Id::MakeId24_8(resourceId, this->uniqueId));
	}
	else
	{
		// get id of resource
		resourceId = this->ids.ValueAtIndex(i);
		ret = Ids::Id::MakeId64(instanceId, Ids::Id::MakeId24_8(resourceId, this->uniqueId));

		// bump usage
		this->usage[resourceId]++;
		ResourceContainer& container = this->containers[resourceId];
	}
	return ret;
}


//------------------------------------------------------------------------------
/**
*/
void
ResourceMemoryPool::DiscardResource(const Resources::ResourceId id)
{
	ResourcePool::DiscardResource(id);
	Ids::Id24 resourceId = Ids::Id::GetBig(Ids::Id::GetLow(id));

	// if usage reaches 0, add it to the list of pending unloads
	if (this->usage[resourceId] == 0)
	{
		// unload immediately
		this->Unload(this->containers[resourceId].resource);
		this->resourceIndexPool.Dealloc(resourceId);
	}
}


//------------------------------------------------------------------------------
/**
*/
void
ResourceMemoryPool::DiscardByTag(const Util::StringAtom& tag)
{
	IndexT i;
	for (i = 0; i < containers.Size(); i++)
	{
		ResourceContainer& container = containers[i];
		if (container.resource->tag == tag)
		{
			// recycle the resource id
			const Ids::Id24 resourceId = this->ids[container.resource->resourceName];

			// unload
			this->Unload(this->containers[resourceId].resource);
			this->resourceIndexPool.Dealloc(resourceId);
		}
	}
}

} // namespace Resources