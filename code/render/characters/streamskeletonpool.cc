//------------------------------------------------------------------------------
//  streamskeletonpool.cc
//  (C) 2018 Individual contributors, see AUTHORS file
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
StreamSkeletonPool::LoadFromStream(const Resources::ResourceId id, const Util::StringAtom & tag, const Ptr<IO::Stream>& stream)
{
	Util::FixedArray<CharacterJoint>& joints = this->Get<Joints>(id.allocId);
	Util::FixedArray<Math::matrix44>& bindPoses = this->Get<BindPose>(id.allocId);
	Util::HashTable<Util::StringAtom, IndexT>& jointIndexMap = this->Get<JointNameMap>(id.allocId);
	Util::FixedArray<Math::float4>& idleSamples = this->Get<IdleSamples>(id.allocId);
	stream->SetAccessMode(Stream::ReadAccess);
	if (stream->Open())
	{
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
		return Resources::ResourcePool::Success;
	}
	return Resources::ResourcePool::Failed;
}

//------------------------------------------------------------------------------
/**
*/
void 
StreamSkeletonPool::Unload(const Resources::ResourceId id)
{
}

//------------------------------------------------------------------------------
/**
*/
const Util::FixedArray<Math::matrix44>&
StreamSkeletonPool::GetBindPose(const SkeletonId id)
{
	return this->skeletonAllocator.Get<BindPose>(id.allocId);
}

//------------------------------------------------------------------------------
/**
*/
const IndexT 
StreamSkeletonPool::GetJointIndex(const SkeletonId id, const Util::StringAtom& name)
{
	const IndexT idx = this->skeletonAllocator.Get<JointNameMap>(id.allocId).FindIndex(name);
	n_assert(idx != InvalidIndex);
	return this->skeletonAllocator.Get<JointNameMap>(id.allocId).ValueAtIndex(name, idx);
}

} // namespace Characters
