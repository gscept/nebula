#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::AnimBuilderLoader
    
    Load NAX2 or NAX3 file into AnimBuilder object.
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "io/uri.h"
#include "toolkitutil/animutil/animbuilder.h"

//------------------------------------------------------------------------------
namespace ToolkitUtil
{
class AnimBuilderLoader
{
public:
    /// load a NAX2 file
    static bool LoadNax2(const IO::URI& uri, AnimBuilder& animBuilder, const Util::Array<Util::String>& clipNames, bool autoGenerateClipNames);
    /// load a NAX3 file
    //static bool LoadNax3(const IO::URI& uri, System::ByteOrder::Type srcByteOrder, AnimBuilder& animBuilder);

private:
    /// extract anim events from NAX2 metadata
    static void ExtractAnimEventsFromNax2MetaData(AnimBuilderClip& clip, const Util::String& metaData);
};

} // namespace ToolkitUtil
//------------------------------------------------------------------------------
