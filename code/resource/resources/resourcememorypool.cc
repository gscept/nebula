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
	ResourceId ret;
	ResourceUnknownId resourceId; // this is the id of the shared resource	
	IndexT i = this->ids.FindIndex(res);

	if (i == InvalidIndex || !res.IsValid())
	{
		// allocate new resource
		resourceId = this->AllocObject();

		// allocate new object in pool
		Ids::Id32 instanceId = this->resourceInstanceIndexPool.Alloc(); // this is the ID of the instance

		// create new resource id, if need be, grow the container list
		if (instanceId >= (uint)this->names.Size())
		{
			this->usage.Resize(this->usage.Size() + ResourceIndexGrow);
			this->names.Resize(this->names.Size() + ResourceIndexGrow);
			this->tags.Resize(this->tags.Size() + ResourceIndexGrow);
			this->states.Resize(this->states.Size() + ResourceIndexGrow);
		}

		// add the resource name to the resource id (if valid)
		this->names[instanceId] = res;
		this->usage[instanceId] = 1;
		this->tags[instanceId] = tag;
		this->states[instanceId] = Resource::Pending;

		ret.id24_0 = instanceId;
		ret.id8_0 = this->uniqueId;
		ret.id24_1 = resourceId.id24;
		ret.id8_1 = resourceId.id8;
		
		if (res.IsValid()) this->ids.Add(res, ret);
	}
	else
	{
		// get id of resource
		ret = this->ids.ValueAtIndex(i);

		// bump usage
		this->usage[ret.id24_0]++;
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

	// if usage reaches 0, add it to the list of pending unloads
	if (this->usage[id.id24_0] == 0)
	{
		// unload immediately
		this->Unload(id.id24_1);
		this->DeallocObject(id.id24_1);
		this->resourceInstanceIndexPool.Dealloc(id.id24_0);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceMemoryPool::DiscardByTag(const Util::StringAtom& tag)
{
	IndexT i;
	for (i = 0; i < this->tags.Size(); i++)
	{
		if (this->tags[i] == tag)
		{
			const ResourceId& id = this->ids[this->names[i]];

			// unload
			this->Unload(id.id24_1);
			this->DeallocObject(id.id24_1);
			this->resourceInstanceIndexPool.Dealloc(id.id24_0);
			this->tags[i] = "";
		}
	}
}

} // namespace Resources