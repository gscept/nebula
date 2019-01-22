#pragma once
//------------------------------------------------------------------------------
/**
	Stream loader for skeletons

	(C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "resources/resourcestreampool.h"
#include "util/fixedarray.h"
#include "util/hashtable.h"
#include "skeleton.h"
namespace Characters
{

class StreamSkeletonPool : public Resources::ResourceStreamPool
{
	__DeclareClass(StreamSkeletonPool);
public:

	/// load character definition from stream
	Resources::ResourcePool::LoadStatus LoadFromStream(const Resources::ResourceId id, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream);
	/// unload resource
	void Unload(const Resources::ResourceId id);

	/// get bind pose
	const Util::FixedArray<Math::matrix44>& GetBindPose(const SkeletonId id);
	/// get joint index by name
	const IndexT GetJointIndex(const SkeletonId id, const Util::StringAtom& name);
private:
	enum
	{
		Joints,
		BindPose,
		JointNameMap,
		IdleSamples
	};

	struct CharacterJoint
	{
		Math::vector poseTranslation;
		Math::quaternion poseRotation;
		Math::vector poseScale;
		Math::matrix44 poseMatrix;
		Math::matrix44* invPoseMatrixPtr;
		IndexT parentJointIndex;
		const CharacterJoint* parentJoint;
#if NEBULA_DEBUG
		Util::StringAtom name;
#endif
	};

	Ids::IdAllocator<
		Util::FixedArray<CharacterJoint>,
		Util::FixedArray<Math::matrix44>,
		Util::HashTable<Util::StringAtom, IndexT>,
		Util::FixedArray<Math::float4>
	> skeletonAllocator;
	__ImplementResourceAllocator(skeletonAllocator);
};

} // namespace Characters
