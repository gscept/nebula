//------------------------------------------------------------------------------
//  streamskeletonpool.cc
//  (C) 2018-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "streamskeletonpool.h"
#include "nskfileformatstructs.h"
#include "util/fourcc.h"
using namespace IO;
namespace Characters
{

__ImplementClass(Characters::StreamSkeletonPool, 'SSKP', Resources::ResourceStreamPool)

//------------------------------------------------------------------------------
/**
*/
Resources::ResourcePool::LoadStatus 
StreamSkeletonPool::LoadFromStream(const Resources::ResourceId id, const Util::StringAtom & tag, const Ptr<IO::Stream>& stream, bool immediate)
{
	Util::FixedArray<CharacterJoint>& joints = this->Get<Joints>(id.resourceId);
	Util::FixedArray<Math::matrix44>& bindPoses = this->Get<BindPose>(id.resourceId);
	Util::HashTable<Util::StringAtom, IndexT>& jointIndexMap = this->Get<JointNameMap>(id.resourceId);
	Util::FixedArray<Math::float4>& idleSamples = this->Get<IdleSamples>(id.resourceId);

	// map buffer
	byte* ptr = (byte*)stream->Map();

	// read header
	Nsk3Header* header = (Nsk3Header*)ptr;
	ptr += sizeof(Nsk3Header);

	// check magic value
	if (Util::FourCC(header->magic) != NEBULA_NSK3_MAGICNUMBER)
	{
		n_error("StreamSkeletonPool::LoadFromStream(): '%s' has invalid file format (magic number doesn't match)!", stream->GetURI().AsString().AsCharPtr());
		return Failed;
	}

	// load joints
	if (header->numJoints > 0)
	{
		joints.SetSize(header->numJoints);
		bindPoses.SetSize(header->numJoints);
		idleSamples.SetSize(header->numJoints * 4);
		uint jointIndex;
		for (jointIndex = 0; jointIndex < header->numJoints; jointIndex++)
		{
			Nsk3Joint* joint = (Nsk3Joint*)ptr;
			ptr += sizeof(Nsk3Joint);

			// setup base components
			joints[jointIndex].poseTranslation = joint->translation;
			joints[jointIndex].poseRotation = joint->rotation;
			joints[jointIndex].poseScale = joint->scale;
			joints[jointIndex].parentJointIndex = joint->parent;
			if (joint->parent != InvalidIndex)
				joints[jointIndex].parentJoint = &joints[joint->parent];
			else
				joints[jointIndex].parentJoint = nullptr;

#if NEBULA_DEBUG
			joints[jointIndex].name = joint->name;
#endif

			// construct pose matrix
			joints[jointIndex].poseMatrix = Math::matrix44::identity();
			joints[jointIndex].poseMatrix.scale(joints[jointIndex].poseScale);
			joints[jointIndex].poseMatrix = Math::matrix44::multiply(joints[jointIndex].poseMatrix, Math::matrix44::rotationquaternion(joints[jointIndex].poseRotation));
			joints[jointIndex].poseMatrix.translate(joints[jointIndex].poseTranslation);
			if (joints[jointIndex].parentJoint != nullptr)
				joints[jointIndex].poseMatrix = Math::matrix44::multiply(joints[jointIndex].poseMatrix, joints[jointIndex].parentJoint->poseMatrix);

			// setup bind pose and mapping
			bindPoses[jointIndex] = Math::matrix44::inverse(joints[jointIndex].poseMatrix);
			jointIndexMap.Add(joint->name, jointIndex);

			// setup idle samples, which are used when no animation is playing
			idleSamples[jointIndex * 4 + 0] = joints[jointIndex].poseTranslation;
			idleSamples[jointIndex * 4 + 1].load((const float*)&joints[jointIndex].poseRotation);
			idleSamples[jointIndex * 4 + 2] = joints[jointIndex].poseScale;
			idleSamples[jointIndex * 4 + 3] = Math::vector::nullvec();
		}
	}
	stream->Unmap();
	return Resources::ResourcePool::Success;
}

//------------------------------------------------------------------------------
/**
*/
void 
StreamSkeletonPool::Unload(const Resources::ResourceId id)
{
	this->skeletonAllocator.Dealloc(id.resourceId);
}

//------------------------------------------------------------------------------
/**
*/
const Util::FixedArray<Math::matrix44>&
StreamSkeletonPool::GetBindPose(const SkeletonId id) const
{
	return this->skeletonAllocator.Get<BindPose>(id.resourceId);
}

//------------------------------------------------------------------------------
/**
*/
const IndexT 
StreamSkeletonPool::GetJointIndex(const SkeletonId id, const Util::StringAtom& name) const
{
	const IndexT idx = this->skeletonAllocator.Get<JointNameMap>(id.resourceId).FindIndex(name);
	n_assert(idx != InvalidIndex);
	return this->skeletonAllocator.Get<JointNameMap>(id.resourceId).ValueAtIndex(name, idx);
}

//------------------------------------------------------------------------------
/**
*/
const SizeT 
StreamSkeletonPool::GetNumJoints(const SkeletonId id) const
{
	return this->skeletonAllocator.Get<Joints>(id.resourceId).Size();
}

//------------------------------------------------------------------------------
/**
*/
const Util::FixedArray<CharacterJoint>& 
StreamSkeletonPool::GetJoints(const SkeletonId id) const
{
	return this->skeletonAllocator.Get<Joints>(id.resourceId);
}

} // namespace Characters
