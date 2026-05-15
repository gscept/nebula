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

struct SkeletonResourceT;
class SkeletonBuilderSaver
{
public:
    /// save flatbuffesr skeleton file
    static bool SaveImport(const IO::URI& uri, const Util::Array<SkeletonBuilder>& skeletonBuilders, Platform::Code platform);
    /// Save binary mesh file
    static bool SaveBinary(const IO::URI& uri, const ToolkitUtil::SkeletonResourceT* resource, Platform::Code platform);
private:
    /// Write header to stream
    static void WriteHeader(const Ptr<IO::Stream>& stream, const ToolkitUtil::SkeletonResourceT* resource, const System::ByteOrder& byteOrder);
    /// Write skeletons to stream
    static void WriteSkeletons(const Ptr<IO::Stream>& stream, const ToolkitUtil::SkeletonResourceT* resource, const System::ByteOrder& byteOrder);
    /// Write skeleton
    static void WriteJoints(const Ptr<IO::Stream>& stream, const ToolkitUtil::SkeletonResourceT* resource, const System::ByteOrder& byteOrder);
};

} // namespace ToolkitUtil
