#pragma once
//------------------------------------------------------------------------------
/**
	A character encapsulates a skeleton resource, an animation resource, and the ability
	to instantiate such and drive animations.

	(C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "resources/resourceid.h"
namespace Characters
{

RESOURCE_ID_TYPE(SkeletonId);

class StreamSkeletonPool;
extern StreamSkeletonPool* skeletonPool;

/// create model (resource)
const SkeletonId CreateSkeleton(const ResourceCreateInfo& info);
/// discard model (resource)
void DestroySkeleton(const SkeletonId id);

} // namespace Characters
