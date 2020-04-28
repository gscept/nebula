#pragma once
//------------------------------------------------------------------------------
/**
	A character encapsulates a skeleton resource, an animation resource, and the ability
	to instantiate such and drive animations.

	(C) 2018-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "resources/resourceid.h"
#include "util/fixedarray.h"
#include "math/vector.h"
namespace Characters
{

RESOURCE_ID_TYPE(SkeletonId);

class StreamSkeletonPool;
extern StreamSkeletonPool* skeletonPool;

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

/// create model (resource)
const SkeletonId CreateSkeleton(const ResourceCreateInfo& info);
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

} // namespace Characters
