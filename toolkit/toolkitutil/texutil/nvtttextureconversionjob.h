#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil:NVTTTextureConversionJob

    Implementation of a conversion job, that converts a texture for
    the linux platform using the nvidia-texture-tools
    
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "toolkitutil/texutil/textureconversionjob.h"
#include "toolkit-common/applauncher.h"
#include "io/uri.h"

namespace ToolkitUtil
{
class NVTTTextureConversionJob : public TextureConversionJob
{
public:
    /// Constructor
    NVTTTextureConversionJob();
    /// perform conversion
    virtual bool Convert();
    
};

} // namespace ToolkitUtil