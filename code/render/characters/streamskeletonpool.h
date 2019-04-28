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
	const Util::FixedArray<Math::matrix44>& GetBindPose(const SkeletonId id) const;
	/// get joint index by name
	const IndexT GetJointIndex(const SkeletonId id, const Util::StringAtom& name) const;

	/// get number of skeleton joints
	const SizeT GetNumJoints(const SkeletonId id) const;
	/// get joints
	const Util::FixedArray<CharacterJoint>& GetJoints(const SkeletonId id) const;
private:
	enum
	{
		Joints,
		BindPose,
		JointNameMap,
		IdleSamples
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
