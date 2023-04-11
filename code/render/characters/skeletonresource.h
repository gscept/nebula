#pragma once
//------------------------------------------------------------------------------
/**
    A skeleton resource is a container for skeletons loaded from an NSK file

    @copyright
    (C) 2022 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "resources/resourceid.h"
#include "ids/idallocator.h"
#include "util/fixedarray.h"
#include "skeleton.h"
namespace Characters
{

RESOURCE_ID_TYPE(SkeletonResourceId);

/// Get skeleton from resource
const SkeletonId SkeletonResourceGetSkeleton(const SkeletonResourceId id, IndexT index);

/// Destroy all skeletons in resource
void DestroySkeletonResource(const SkeletonResourceId id);

typedef Ids::IdAllocator<
    Util::FixedArray<SkeletonId>
> SkeletonResourceAllocator;
extern SkeletonResourceAllocator skeletonResourceAllocator;

} // namespace SkeletonResource
