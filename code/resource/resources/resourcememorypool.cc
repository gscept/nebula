//------------------------------------------------------------------------------
// resourcememoryloader.cc
// (C)2017-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
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

        ret.poolId = instanceId;
        ret.poolIndex = this->uniqueId;
        ret.resourceId = resourceId.id24;
        ret.resourceType = resourceId.id8;
        
        if (res.IsValid()) this->ids.Add(res, ret);
    }
    else
    {
        // get id of resource
        ret = this->ids.ValueAtIndex(i);
        if (this->states[ret.poolId] == Resource::Unloaded)
            this->states[ret.poolId] = Resource::Pending;

        // bump usage
        this->usage[ret.poolId]++;
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
    if (this->usage[id.poolId] == 0)
    {
        // unload immediately
        this->Unload(id);
        this->DeallocObject(id.AllocId());
        
        this->resourceInstanceIndexPool.Dealloc(id.poolId);
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
            this->Unload(id.resourceId);
            this->DeallocObject(id.resourceId);
            this->resourceInstanceIndexPool.Dealloc(id.poolId);
            this->tags[i] = "";
        }
    }
}

} // namespace Resources