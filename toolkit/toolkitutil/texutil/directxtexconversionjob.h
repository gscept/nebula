#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::DirectXTexConversionJob

    Implementation of a conversion job, that converts a texture
    using TexConv from DirectXTex

    (C) 2020 Individual contributors, see AUTHORS file
*/
#include "toolkitutil/texutil/textureconversionjob.h"
#include "toolkit-common/applauncher.h"
#include "io/uri.h"

#include "flat/texture.h"

namespace ToolkitUtil
{
class DirectXTexConversionJob : public TextureConversionJob
{
public:
    /// Constructor
    DirectXTexConversionJob();
    /// perform conversion
    virtual bool Convert(const ToolkitUtil::TextureResourceT* texture);
    /// perform conversion
    virtual bool ConvertCube(const ToolkitUtil::TextureResourceT* texture);

private:
    ToolkitUtil::AppLauncher appLauncher;
    Util::String cubeToolPath;
};

} // namespace ToolkitUtil