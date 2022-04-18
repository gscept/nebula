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
    static bool SaveNsk3(const IO::URI& uri, const SkeletonBuilder& skeletonBuilder, Platform::Code platform);

private:
    /// write header to stream
    static void WriteHeader(const Ptr<IO::Stream>& stream, const SkeletonBuilder& skeletonBuilder, const System::ByteOrder& byteOrder);
    /// write skeleton
    static void WriteSkeleton(const Ptr<IO::Stream>& stream, const SkeletonBuilder& skeletonBuilder, const System::ByteOrder& byteOrder);
};

} // namespace ToolkitUtil
