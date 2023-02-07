#pragma once
//------------------------------------------------------------------------------
/**
    A character encapsulates a skeleton resource, an animation resource, and the ability
    to instantiate such and drive animations.

    @copyright
    (C) 2018-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "resources/resourceid.h"
#include "util/fixedarray.h"
#include "math/vector.h"
namespace Characters
{

ID_24_8_TYPE(SkeletonId);

struct CharacterJoint
{
    Math::vector poseTranslation;
    Math::quat poseRotation;
    Math::vector poseScale;
    Math::mat4 poseMatrix;
    IndexT parentJointIndex;
    const CharacterJoint* parentJoint;
#if NEBULA_DEBUG
    Util::StringAtom name;
#endif
};

struct SkeletonCreateInfo
{
    Util::FixedArray<CharacterJoint> joints;
    Util::FixedArray<Math::mat4> bindPoses;
    Util::HashTable<Util::StringAtom, IndexT> jointIndexMap;
    Util::FixedArray<Math::vec4> idleSamples;
};

/// create model (resource)
const SkeletonId CreateSkeleton(const SkeletonCreateInfo& info);
/// discard model (resource)
void DestroySkeleton(const SkeletonId id);

/// get number of joints from skeleton
const SizeT SkeletonGetNumJoints(const SkeletonId id);
/// get joints from skeleton
const Util::FixedArray<CharacterJoint>& SkeletonGetJoints(const SkeletonId id);

/// get bind pose
const Util::FixedArray<Math::mat4>& SkeletonGetBindPose(const SkeletonId id);
/// get joint index
const IndexT SkeletonGetJointIndex(const SkeletonId id, const Util::StringAtom& name);

enum
{
    Skeleton_Joints,
    Skeleton_BindPose,
    Skeleton_JointNameMap,
    Skeleton_IdleSamples
};

typedef Ids::IdAllocator<
    Util::FixedArray<CharacterJoint>,
    Util::FixedArray<Math::mat4>,
    Util::HashTable<Util::StringAtom, IndexT>,
    Util::FixedArray<Math::vec4>
> SkeletonAllocator;
extern SkeletonAllocator skeletonAllocator;

} // namespace Characters
