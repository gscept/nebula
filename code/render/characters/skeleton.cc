//------------------------------------------------------------------------------
//  skeleton.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "skeleton.h"
#include "streamskeletonpool.h"
namespace Characters
{

StreamSkeletonPool* skeletonPool;

//------------------------------------------------------------------------------
/**
*/
const SkeletonId 
CreateSkeleton(const ResourceCreateInfo& info)
{
	return skeletonPool->CreateResource(info.resource, info.tag, info.successCallback, info.failCallback, !info.async).As<SkeletonId>();
}

//------------------------------------------------------------------------------
/**
*/
void 
DestroySkeleton(const SkeletonId id)
{
	skeletonPool->DiscardResource(id);
}

} // namespace Characters
