//------------------------------------------------------------------------------
//  skeleton.cc
//  (C) 2018-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "skeleton.h"
namespace Characters
{

SkeletonAllocator skeletonAllocator;
//------------------------------------------------------------------------------
/**
*/
const SkeletonId 
CreateSkeleton(const SkeletonCreateInfo& info)
{
    Ids::Id32 id = skeletonAllocator.Alloc();
    skeletonAllocator.Set<Skeleton_Joints>(id, info.joints);
    skeletonAllocator.Set<Skeleton_BindPose>(id, info.bindPoses);
    skeletonAllocator.Set<Skeleton_JointNameMap>(id, info.jointIndexMap);
    skeletonAllocator.Set<Skeleton_IdleSamples>(id, info.idleSamples);

    SkeletonId ret;
    ret.id24 = id;
    ret.id8 = CoreGraphics::SkeletonIdType;
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void 
DestroySkeleton(const SkeletonId id)
{
    skeletonAllocator.Dealloc(id.id24);
}

//------------------------------------------------------------------------------
/**
*/
const SizeT 
SkeletonGetNumJoints(const SkeletonId id)
{
    return skeletonAllocator.Get<Skeleton_Joints>(id.id24).Size();
}

//------------------------------------------------------------------------------
/**
*/
const Util::FixedArray<CharacterJoint>& 
SkeletonGetJoints(const SkeletonId id)
{
    return skeletonAllocator.Get<Skeleton_Joints>(id.id24);
}

//------------------------------------------------------------------------------
/**
*/
const Util::FixedArray<Math::mat4>&
SkeletonGetBindPose(const SkeletonId id)
{
    return skeletonAllocator.Get<Skeleton_BindPose>(id.id24);
}

//------------------------------------------------------------------------------
/**
*/
const IndexT 
SkeletonGetJointIndex(const SkeletonId id, const Util::StringAtom& name)
{
    return skeletonAllocator.Get<Skeleton_JointNameMap>(id.id24)[name];
}

//------------------------------------------------------------------------------
/**
*/
const Util::FixedArray<Math::vec4>&
SkeletonGetIdleSamples(const SkeletonId id)
{
    return skeletonAllocator.Get<Skeleton_IdleSamples>(id.id24);
}

} // namespace Characters
