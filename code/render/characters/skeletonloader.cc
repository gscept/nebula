//------------------------------------------------------------------------------
//  skeletonloader.cc
//  (C) 2018-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "skeletonloader.h"
#include "nskfileformatstructs.h"
#include "util/fourcc.h"
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
            idleSamples.SetSize(nsk3Skeleton->numJoints * 3);
            Nsk3Joint* nskJoints = (Nsk3Joint*)(nsk3Skeleton + 1);
            uint jointIndex;
            for (jointIndex = 0; jointIndex < nsk3Skeleton->numJoints; jointIndex++)
            {
                Nsk3Joint* joint = (Nsk3Joint*)ptr;
                ptr += sizeof(Nsk3Joint);

                // setup base components
                joints[jointIndex].parentJointIndex = joint->parent;
                if (joint->parent != InvalidIndex)
                    joints[jointIndex].parentJoint = &joints[joint->parent];
                else
                    joints[jointIndex].parentJoint = nullptr;

#if NEBULA_DEBUG
                joints[jointIndex].name = joint->name;
#endif

                // setup bind pose and mapping
                bindPoses[jointIndex].loadu(joint->bind);
                jointIndexMap.Add(joint->name, jointIndex);

                idleSamples[jointIndex * 3 + 0].loadu(joint->translation);
                idleSamples[jointIndex * 3 + 0].w = 0.0f;
                idleSamples[jointIndex * 3 + 1].loadu(joint->rotation);
                idleSamples[jointIndex * 3 + 2].loadu(joint->scale);
                idleSamples[jointIndex * 3 + 2].w = 0.0f;
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
