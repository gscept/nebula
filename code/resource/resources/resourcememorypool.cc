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
	Ids::Id64 ret;
	Ids::Id32 instanceId = this->resourceInstanceIndexPool.Alloc(); // this is the ID of the container
	Ids::Id24 resourceId; // this is the id of the resource	
	IndexT i = this->ids.FindIndex(res);

	if (i == InvalidIndex || !res.IsValid())
	{
		// allocate new resource
		resourceId = this->AllocResource();

		// create new resource id, if need be, grow the container list
		if (resourceId >= (uint32_t)this->names.Size())
		{
			this->usage.Resize(this->usage.Size() + ResourceIndexGrow);
			this->names.Resize(this->names.Size() + ResourceIndexGrow);
			this->tags.Resize(this->tags.Size() + ResourceIndexGrow);
			this->states.Resize(this->states.Size() + ResourceIndexGrow);
		}

		// add the resource name to the resource id (if valid)
		if (res.IsValid()) this->ids.Add(res, resourceId);
		this->names[resourceId] = res;
		this->usage[resourceId] = 1;
		this->tags[resourceId] = tag;
		this->states[resourceId] = Resource::Loaded;
		ret = Ids::Id::MakeId64(instanceId, Ids::Id::MakeId24_8(resourceId, this->uniqueId));
	}
	else
	{
		// get id of resource
		resourceId = this->ids.ValueAtIndex(i);
		ret = Ids::Id::MakeId64(instanceId, Ids::Id::MakeId24_8(resourceId, this->uniqueId));

		// bump usage
		this->usage[resourceId]++;
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
		const Ids::Id24 resId = Ids::Id::GetBig(Ids::Id::GetLow(id));
		this->Unload(resId);
		this->DeallocResource(resourceId);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceMemoryPool::DiscardByTag(const Util::StringAtom& tag)
{
	IndexT i;
	for (i = 0; i < tags.Size(); i++)
	{
		if (tags[i] == tag)
		{
			// unload
			this->Unload(i);
			this->DeallocResource(i);
			this->tags[i] = "";
		}
	}
}

} // namespace Resources