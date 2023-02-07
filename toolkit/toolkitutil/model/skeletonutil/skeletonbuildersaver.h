#pragma once
//------------------------------------------------------------------------------
/**
    Saves a skeleton to a resource

    (C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "skeletonbuilder.h"
#include "toolkit-common/platform.h"
#include "io/stream.h"
#include "system/byteorder.h"
namespace ToolkitUtil
{

class SkeletonBuilderSaver
{
public:
    /// save NSK3 file
    static bool Save(const IO::URI& uri, const Util::Array<SkeletonBuilder>& skeletonBuilders, Platform::Code platform);

private:
    /// Write header to stream
    static void WriteHeader(const Ptr<IO::Stream>& stream, const Util::Array<SkeletonBuilder>& skeletonBuilders, const System::ByteOrder& byteOrder);
    /// Write skeletons to stream
    static void WriteSkeletons(const Ptr<IO::Stream>& stream, const Util::Array<SkeletonBuilder>& skeletonBuilders, const System::ByteOrder& byteOrder);
    /// Write skeleton
    static void WriteJoints(const Ptr<IO::Stream>& stream, const Util::Array<SkeletonBuilder>& skeletonBuilders, const System::ByteOrder& byteOrder);
};

} // namespace ToolkitUtil
