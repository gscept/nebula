#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::Win32TextureConversionJob

    Implementation of a conversion job, that converts a texture for
    the win32 platform.

    (C) 2009 Radon Labs GmbH
    (C) 2013-2014 Individual contributors, see AUTHORS file
*/
#include "toolkitutil/texutil/textureconversionjob.h"
#include "toolkitutil/applauncher.h"
#include "io/uri.h"

namespace ToolkitUtil
{
class Win32TextureConversionJob : public TextureConversionJob
{
public:
    /// Constructor
    Win32TextureConversionJob();
    /// perform conversion
    virtual bool Convert();

private:
    ToolkitUtil::AppLauncher appLauncher;
};

} // namespace ToolkitUtil