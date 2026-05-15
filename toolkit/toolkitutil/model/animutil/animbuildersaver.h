#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::AnimBuilderSaver
    
    Save AnimBuilder object into NAX3 file.
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "io/uri.h"
#include "model/animutil/animbuilder.h"
#include "toolkit-common/platform.h"
#include "io/stream.h"
#include "system/byteorder.h"

//------------------------------------------------------------------------------
namespace ToolkitUtil
{
struct AnimResourceT;
class AnimBuilderSaver
{
public:
    /// Save flatbuffers file
    static bool SaveImport(const IO::URI& uri, const Util::Array<AnimBuilder>& animBuilders, Platform::Code platform);
    /// Save NAX3 file
    static bool SaveBinary(const IO::URI& uri, const ToolkitUtil::AnimResourceT* resource, Platform::Code platform);
private:
    /// Write header to stream
    static void WriteHeader(const Ptr<IO::Stream>& stream, const ToolkitUtil::AnimResourceT* resource, const System::ByteOrder& byteOrder);
    /// Write anim header to stream
    static void WriteAnimations(const Ptr<IO::Stream>& stream, const ToolkitUtil::AnimResourceT* resource, const System::ByteOrder& byteOrder);
};

} // namespace ToolkitUtil
    