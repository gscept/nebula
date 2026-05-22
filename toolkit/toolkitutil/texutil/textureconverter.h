#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::TextureConverter
    
    Wraps texture conversion process for all supported target platforms.
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "toolkit-common/platform.h"
#include "io/uri.h"
#include "util/string.h"

//------------------------------------------------------------------------------
namespace ToolkitUtil
{


struct TextureResourceT;
class Logger;
struct TextureConversionInfo
{
    const TextureResourceT* texture;
    ToolkitUtil::Platform::Code platform;
    Util::String tmpDir;
    Util::String sourcePath;
    Util::String destPath;
    bool cube;
    Logger* logger;
};
bool ConvertTexture(const TextureConversionInfo& info);

} // namespace ToolkitUtil
//------------------------------------------------------------------------------

