#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::CompressonatorConversionJob

    Texture conversion job class using amd compressonator

    (C) 2020 Individual contributors, see AUTHORS file
*/
#include "toolkitutil/texutil/textureconversionjob.h"
#include "toolkit-common/applauncher.h"
#include "io/uri.h"

namespace ToolkitUtil
{
class CompressonatorConversionJob : public TextureConversionJob
{
public:
    /// Constructor
    CompressonatorConversionJob();
    /// perform conversion
    virtual bool Convert();
    
};

} // namespace ToolkitUtil