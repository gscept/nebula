//------------------------------------------------------------------------------
// resourceloader.cc
// (C)2017-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "resourcecache.h"

namespace Resources
{

__ImplementAbstractClass(Resources::ResourceCache, 'RELO', Core::RefCounted);
//------------------------------------------------------------------------------
/**
*/
ResourceCache::ResourceCache() :
    resourceInstanceIndexPool(0xFFFFFFFF)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
ResourceCache::~ResourceCache()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceCache::Setup()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceCache::Discard()
{
    // empty
}


//------------------------------------------------------------------------------
/**
*/
void
ResourceCache::LoadFallbackResources()
{
    // empty, implement in subclass
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceCache::DiscardResource(const Resources::ResourceId id)
{
    // reduce usage
    this->usage[id.poolId]--;
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceCache::DiscardByTag(const Util::StringAtom& tag)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceCache::Update(IndexT frameIndex)
{
    // do nothing
}

} // namespace Resources
