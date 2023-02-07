//------------------------------------------------------------------------------
//  skeletonloader.cc
//  (C) 2018-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "skeletonloader.h"
#include "nskfileformatstructs.h"
#include "util/fourcc.h"
#include "math/vector.h"
#include "skeletonresource.h"
using namespace IO;
namespace Characters
{

__ImplementClass(Characters::SkeletonLoader, 'SSKP', Resources::ResourceLoader)

//------------------------------------------------------------------------------
/**
*/
Resources::ResourceUnknownId
SkeletonLoader::LoadFromStream(const Ids::Id32 entry, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream, bool immediate)
{
    // map buffer
    byte* ptr = (byte*)stream->Map();

    // read header
    Nsk3Header* header = (Nsk3Header*)ptr;
    ptr += sizeof(Nsk3Header);

    // check magic value
    if (Util::FourCC(header->magic) != NEBULA_NSK3_MAGICNUMBER)
    {
        n_error("StreamSkeletonCache::LoadFromStream(): '%s' has invalid file format (magic number doesn't match)!", stream->GetURI().AsString().AsCharPtr());
        return Failed;
    }

    Util::FixedArray<SkeletonId> skeletons(header->numSkeletons);
    skeletons.Fill(InvalidSkeletonId);
    for (IndexT skeletonIndex = 0; skeletonIndex < header->numSkeletons; skeletonIndex++)
    {
        Nsk3Skeleton* nsk3Skeleton = (Nsk3Skeleton*)ptr;
        ptr += sizeof(Nsk3Skeleton);

        // load joints
        if (nsk3Skeleton->numJoints > 0)
        {
            Util::FixedArray<CharacterJoint> joints;
            Util::FixedArray<Math::mat4> bindPoses;
            Util::HashTable<Util::StringAtom, IndexT> jointIndexMap;
            Util::FixedArray<Math::vec4> idleSamples;

            joints.SetSize(nsk3Skeleton->numJoints);
            bindPoses.SetSize(nsk3Skeleton->numJoints);
            idleSamples.SetSize(nsk3Skeleton->numJoints * 4);
            Nsk3Joint* nskJoints = (Nsk3Joint*)(nsk3Skeleton + 1);
            uint jointIndex;
            for (jointIndex = 0; jointIndex < nsk3Skeleton->numJoints; jointIndex++)
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
                joints[jointIndex].poseMatrix = Math::mat4();
                joints[jointIndex].poseMatrix.scale(joints[jointIndex].poseScale);
                joints[jointIndex].poseMatrix = Math::rotationquat(joints[jointIndex].poseRotation) * joints[jointIndex].poseMatrix;
                joints[jointIndex].poseMatrix.translate(joints[jointIndex].poseTranslation);
                if (joints[jointIndex].parentJoint != nullptr)
                    joints[jointIndex].poseMatrix = joints[jointIndex].parentJoint->poseMatrix * joints[jointIndex].poseMatrix;

                // setup bind pose and mapping
                bindPoses[jointIndex] = Math::inverse(joints[jointIndex].poseMatrix);
                jointIndexMap.Add(joint->name, jointIndex);

                // setup idle samples, which are used when no animation is playing
                idleSamples[jointIndex * 4 + 0] = Math::vec4(joints[jointIndex].poseTranslation, 1);
                idleSamples[jointIndex * 4 + 1].load((const float*)&joints[jointIndex].poseRotation);
                idleSamples[jointIndex * 4 + 2] = Math::vec4(joints[jointIndex].poseScale, 1);
                idleSamples[jointIndex * 4 + 3] = Math::vector::nullvec();
            }

            SkeletonCreateInfo info;
            info.joints = joints;
            info.bindPoses = bindPoses;
            info.jointIndexMap = jointIndexMap;
            info.idleSamples = idleSamples;
            SkeletonId skeleton = CreateSkeleton(info);
            skeletons[skeletonIndex] = skeleton;
        }
    }
    stream->Unmap();

    auto id = skeletonResourceAllocator.Alloc();
    skeletonResourceAllocator.Set<0>(id, skeletons);
    SkeletonResourceId ret;
    ret.resourceId = id;
    ret.resourceType = CoreGraphics::SkeletonResourceIdType;
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void 
SkeletonLoader::Unload(const Resources::ResourceId id)
{
    DestroySkeletonResource(id);
}

} // namespace Characters
