//------------------------------------------------------------------------------
//  skeletonbuildersaver.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "skeletonbuildersaver.h"
#include "io/ioserver.h"
#include "math/transform.h"
#include "characters/nskfileformatstructs.h"

#include "nflatbuffer/flatbufferinterface.h"
#include "nflatbuffer/nebula_flat.h"
#include "flat/skeleton.h"

using namespace IO;
using namespace System;
using namespace Characters;
namespace ToolkitUtil
{

//------------------------------------------------------------------------------
/**
*/
bool 
SkeletonBuilderSaver::SaveImport(const IO::URI& uri, const Util::Array<SkeletonBuilder>& skeletonBuilders, Platform::Code platform)
{
    // make sure the target directory exists
    IoServer::Instance()->CreateDirectory(uri.LocalPath().ExtractDirName());

    Ptr<Stream> stream = IoServer::Instance()->CreateStream(uri);
    stream->SetAccessMode(Stream::WriteAccess);
    if (stream->Open())
    {
        ToolkitUtil::SkeletonResourceT skeletons;
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
            skeletons.skeletons.push_back(std::move(skeleton));
        }

        Util::Blob data = Flat::FlatbufferInterface::SerializeFlatbuffer<ToolkitUtil::SkeletonResource>(skeletons);
        stream->Write(data.GetPtr(), data.Size());

        //ByteOrder byteOrder(ByteOrder::Host, Platform::GetPlatformByteOrder(platform));
        //SkeletonBuilderSaver::WriteHeader(stream, skeletonBuilders, byteOrder);
        //SkeletonBuilderSaver::WriteSkeletons(stream, skeletonBuilders, byteOrder);
        //SkeletonBuilderSaver::WriteJoints(stream, skeletonBuilders, byteOrder);

        stream->Close();
        stream = nullptr;
        return true;
    }
    else
    {
        // failed to open write stream
        return false;
    }
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
    nsk3Header.numSkeletons = byteOrder.Convert<uint>(resource->skeletons.size());

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
        nsk3Skeleton.numJoints = byteOrder.Convert<uint>(builder->joints.size());

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
        SizeT numJoints = builder->joints.size();
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
