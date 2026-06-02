//------------------------------------------------------------------------------
//  skeletonbuildersaver.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "skeletonbuildersaver.h"
#include "io/ioserver.h"
#include "math/transform.h"
#include "characters/nskfileformatstructs.h"

using namespace IO;
using namespace System;
using namespace Characters;
namespace ToolkitUtil
{

//------------------------------------------------------------------------------
/**
*/
std::unique_ptr<SkeletonResourceT>
SkeletonBuilderSaver::PackImport(const Util::Array<SkeletonBuilder>& skeletonBuilders, Platform::Code platform)
{
    auto skeletons = std::make_unique<ToolkitUtil::SkeletonResourceT>();
    for (auto& builder : skeletonBuilders)
    {
        auto skeleton = std::make_unique<ToolkitUtil::SkeletonInstanceT>();
        for (auto& joint : builder.joints)
        {
            auto jointT = std::make_unique<ToolkitUtil::JointInstanceT>();
            jointT->bind = Math::transform::FromMat4(joint.bind);
            jointT->translation = joint.translation;
            jointT->scale = joint.scale;
            jointT->rotation = joint.rotation;
            jointT->name = joint.name;
            jointT->index = joint.index;
            jointT->parent = joint.parent;
            skeleton->joints.push_back(std::move(jointT));
        }
        skeletons->skeletons.push_back(std::move(skeleton));
    }
    return skeletons;
}

//------------------------------------------------------------------------------
/**
*/
bool 
SkeletonBuilderSaver::SaveBinary(const IO::URI& uri, const ToolkitUtil::SkeletonResourceT* resource, Platform::Code platform)
{
    // make sure the target directory exists
    IoServer::Instance()->CreateDirectory(uri.LocalPath().ExtractDirName());

    Ptr<Stream> stream = IoServer::Instance()->CreateStream(uri);
    stream->SetAccessMode(Stream::WriteAccess);
    if (stream->Open())
    {
        ByteOrder byteOrder(ByteOrder::Host, Platform::GetPlatformByteOrder(platform));
        SkeletonBuilderSaver::WriteHeader(stream, resource, byteOrder);
        SkeletonBuilderSaver::WriteSkeletons(stream, resource, byteOrder);
        SkeletonBuilderSaver::WriteJoints(stream, resource, byteOrder);
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
void 
SkeletonBuilderSaver::WriteHeader(const Ptr<IO::Stream>& stream, const ToolkitUtil::SkeletonResourceT* resource, const System::ByteOrder& byteOrder)
{
    // setup header
    Nsk3Header nsk3Header;
    nsk3Header.magic = byteOrder.Convert<uint>(NEBULA_NSK3_MAGICNUMBER);
    nsk3Header.numSkeletons = byteOrder.Convert<uint>((uint)resource->skeletons.size());

    // write header
    stream->Write(&nsk3Header, sizeof(nsk3Header));
}

//------------------------------------------------------------------------------
/**
*/
void 
SkeletonBuilderSaver::WriteSkeletons(const Ptr<IO::Stream>& stream, const ToolkitUtil::SkeletonResourceT* resource, const System::ByteOrder& byteOrder)
{
    for (auto& builder : resource->skeletons)
    {
        Nsk3Skeleton nsk3Skeleton;
        nsk3Skeleton.numJoints = byteOrder.Convert<uint>((uint)builder->joints.size());

        // Write skeleton
        stream->Write(&nsk3Skeleton, sizeof(nsk3Skeleton));
    }
}

//------------------------------------------------------------------------------
/**
*/
void
SkeletonBuilderSaver::WriteJoints(const Ptr<IO::Stream>& stream, const ToolkitUtil::SkeletonResourceT* resource, const System::ByteOrder& byteOrder)
{
    for (auto& builder : resource->skeletons)
    {
        SizeT numJoints = (uint)builder->joints.size();
        IndexT jointIndex;
        for (jointIndex = 0; jointIndex < numJoints; jointIndex++)
        {
            const auto& joint = builder->joints[jointIndex];
            Nsk3Joint nsk3Joint;
            joint->name.CopyToBuffer(&(nsk3Joint.name[0]), sizeof(nsk3Joint.name));
            nsk3Joint.index = joint->index;
            nsk3Joint.parent = joint->parent;
            Math::affine(joint->bind.scale, joint->bind.rotation, joint->bind.position).storeu(nsk3Joint.bind);
            joint->rotation.storeu(nsk3Joint.rotation);
            joint->translation.storeu(nsk3Joint.translation);
            joint->scale.storeu(nsk3Joint.scale);

            // write clip
            stream->Write(&nsk3Joint, sizeof(nsk3Joint));
        }
    }
}

} // namespace ToolkitUtil
