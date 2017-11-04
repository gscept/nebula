//------------------------------------------------------------------------------
// resourceloader.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "resourcepool.h"

namespace Resources
{

__ImplementAbstractClass(Resources::ResourcePool, 'RELO', Core::RefCounted);
//------------------------------------------------------------------------------
/**
*/
ResourcePool::ResourcePool() :
	resourceInstanceIndexPool(0xFFFFFFFF)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
ResourcePool::~ResourcePool()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
ResourcePool::Setup()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
ResourcePool::Discard()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
ResourcePool::DiscardResource(const Resources::ResourceId id)
{
	// the id of the usage and container respectively is in the big part of the low bits
	Ids::Id32 instanceId = Ids::Id::GetHigh(id);
	Ids::Id24 resourceId = Ids::Id::GetBig(Ids::Id::GetLow(id));

	n_assert_fmt(!this->tags[resourceId].IsValid(), "Resource with tag can not be individually deleted");

	// dealloc instance id and reduce usage
	this->resourceInstanceIndexPool.Dealloc(instanceId);
	this->usage[resourceId]--;
}

//------------------------------------------------------------------------------
/**
*/
void
ResourcePool::DiscardByTag(const Util::StringAtom& tag)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
ResourcePool::Update(IndexT frameIndex)
{
	// do nothing
}

} // namespace Resources