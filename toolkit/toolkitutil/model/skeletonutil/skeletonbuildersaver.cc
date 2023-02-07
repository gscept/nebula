//------------------------------------------------------------------------------
//  skeletonbuildersaver.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "skeletonbuildersaver.h"
#include "io/ioserver.h"
#include "characters/nskfileformatstructs.h"

using namespace IO;
using namespace System;
using namespace Characters;
namespace ToolkitUtil
{

//------------------------------------------------------------------------------
/**
*/
bool 
SkeletonBuilderSaver::Save(const IO::URI& uri, const Util::Array<SkeletonBuilder>& skeletonBuilders, Platform::Code platform)
{
    // make sure the target directory exists
    IoServer::Instance()->CreateDirectory(uri.LocalPath().ExtractDirName());

    Ptr<Stream> stream = IoServer::Instance()->CreateStream(uri);
    stream->SetAccessMode(Stream::WriteAccess);
    if (stream->Open())
    {
        ByteOrder byteOrder(ByteOrder::Host, Platform::GetPlatformByteOrder(platform));
        SkeletonBuilderSaver::WriteHeader(stream, skeletonBuilders, byteOrder);
        SkeletonBuilderSaver::WriteSkeletons(stream, skeletonBuilders, byteOrder);
        SkeletonBuilderSaver::WriteJoints(stream, skeletonBuilders, byteOrder);

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
void 
SkeletonBuilderSaver::WriteHeader(const Ptr<IO::Stream>& stream, const Util::Array<SkeletonBuilder>& skeletonBuilders, const System::ByteOrder& byteOrder)
{
    // setup header
    Nsk3Header nsk3Header;
    nsk3Header.magic = byteOrder.Convert<uint>(NEBULA_NSK3_MAGICNUMBER);
    nsk3Header.numSkeletons = byteOrder.Convert<uint>(skeletonBuilders.Size());

    // write header
    stream->Write(&nsk3Header, sizeof(nsk3Header));
}

//------------------------------------------------------------------------------
/**
*/
void 
SkeletonBuilderSaver::WriteSkeletons(const Ptr<IO::Stream>& stream, const Util::Array<SkeletonBuilder>& skeletonBuilders, const System::ByteOrder& byteOrder)
{
    for (auto& builder : skeletonBuilders)
    {
        Nsk3Skeleton nsk3Skeleton;
        nsk3Skeleton.numJoints = byteOrder.Convert<uint>(builder.joints.Size());

        // Write skeleton
        stream->Write(&nsk3Skeleton, sizeof(nsk3Skeleton));
    }
}

//------------------------------------------------------------------------------
/**
*/
void
SkeletonBuilderSaver::WriteJoints(const Ptr<IO::Stream>& stream, const Util::Array<SkeletonBuilder>& skeletonBuilders, const System::ByteOrder& byteOrder)
{
    for (auto& builder : skeletonBuilders)
    {
        SizeT numJoints = builder.joints.Size();
        IndexT jointIndex;
        for (jointIndex = 0; jointIndex < numJoints; jointIndex++)
        {
            const ToolkitUtil::Joint joint = builder.joints[jointIndex];
            Nsk3Joint nsk3Joint;
            joint.name.CopyToBuffer(&(nsk3Joint.name[0]), sizeof(nsk3Joint.name));
            nsk3Joint.index = joint.index;
            nsk3Joint.parent = joint.parent;
            nsk3Joint.translation = joint.translation;
            nsk3Joint.rotation = joint.rotation;
            nsk3Joint.scale = joint.scale;

            // write clip
            stream->Write(&nsk3Joint, sizeof(nsk3Joint));
        }
    }
}

} // namespace ToolkitUtil
