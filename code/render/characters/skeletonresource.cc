//------------------------------------------------------------------------------
//  @file skeletonresource.cc
//  @copyright (C) 2022 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "skeletonresource.h"
namespace Characters
{

SkeletonResourceAllocator skeletonResourceAllocator;


//------------------------------------------------------------------------------
/**
    Fetch skeleton from loaded resource
*/
const SkeletonId
SkeletonResourceGetSkeleton(const SkeletonResourceId id, IndexT index)
{
    return skeletonResourceAllocator.Get<0>(id.resourceId)[index];
}

//------------------------------------------------------------------------------
/**
    Discard skeleton resource and the skeletons it holds
*/
void
DestroySkeletonResource(const SkeletonResourceId id)
{
    auto skeletons = skeletonResourceAllocator.Get<0>(id.resourceId);
    for (IndexT i = 0; i < skeletons.Size(); i++)
    {
        DestroySkeleton(skeletons[i]);
    }
    skeletonResourceAllocator.Dealloc(id.resourceId);
}

} // namespace Characters
