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
class AnimBuilderSaver
{
public:
    /// Save NAX3 file
    static bool Save(const IO::URI& uri, const Util::Array<AnimBuilder>& animBuilders, Platform::Code platform);

private:
    /// Write header to stream
    static void WriteHeader(const Ptr<IO::Stream>& stream, const Util::Array<AnimBuilder>& animBuilders, const System::ByteOrder& byteOrder);
    /// Write anim header to stream
    static void WriteAnimations(const Ptr<IO::Stream>& stream, const Util::Array<AnimBuilder>& animBuilders, const System::ByteOrder& byteOrder);
};

} // namespace ToolkitUtil
    