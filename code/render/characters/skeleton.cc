//------------------------------------------------------------------------------
//  skeleton.cc
//  (C) 2018-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
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

//------------------------------------------------------------------------------
/**
*/
const SizeT 
SkeletonGetNumJoints(const SkeletonId id)
{
	return skeletonPool->GetNumJoints(id);
}

//------------------------------------------------------------------------------
/**
*/
const Util::FixedArray<CharacterJoint>& 
SkeletonGetJoints(const SkeletonId id)
{
	return skeletonPool->GetJoints(id);
}

//------------------------------------------------------------------------------
/**
*/
const Util::FixedArray<Math::mat4>&
SkeletonGetBindPose(const SkeletonId id)
{
	return skeletonPool->GetBindPose(id);
}

//------------------------------------------------------------------------------
/**
*/
const IndexT 
SkeletonGetJointIndex(const SkeletonId id, const Util::StringAtom& name)
{
	return skeletonPool->GetJointIndex(id, name);
}

} // namespace Characters
