//------------------------------------------------------------------------------
//  textureconverter.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "textureconverter.h"
#include "io/ioserver.h"
#include "io/textwriter.h"
#include "io/xmlwriter.h"
#include "util/guid.h"
#include "flat/texture.h"
#include "system/process.h"
#include "toolkitutil/logger.h"


#if !(__WIN32__)
#include "system/environment.h"
#endif

#include "toolkitutil/text.h"

namespace ToolkitUtil
{
using namespace Util;
using namespace IO;


#if (__WIN32__)    
//------------------------------------------------------------------------------
/**
*/
static const char*
GetConverterFormatString(ToolkitUtil::TextureResourceT const* texture)
{
    switch (texture->target_format)
    {
        case ToolkitUtil::TexturePixelFormat::TexturePixelFormat_BC1: return texture->color_space == ToolkitUtil::TextureColorSpace::TextureColorSpace_sRGB ? "BC1_UNORM_SRGB" : "BC1_UNORM";
        case ToolkitUtil::TexturePixelFormat::TexturePixelFormat_BC2: return texture->color_space == ToolkitUtil::TextureColorSpace::TextureColorSpace_sRGB ? "BC2_UNORM_SRGB" : "BC2_UNORM";
        case ToolkitUtil::TexturePixelFormat::TexturePixelFormat_BC3: return texture->color_space == ToolkitUtil::TextureColorSpace::TextureColorSpace_sRGB ? "BC3_UNORM_SRGB" : "BC3_UNORM";
        case ToolkitUtil::TexturePixelFormat::TexturePixelFormat_BC5: return "BC5_UNORM";
        case ToolkitUtil::TexturePixelFormat::TexturePixelFormat_BC6H: return "BC6H_UF16";
        case ToolkitUtil::TexturePixelFormat::TexturePixelFormat_BC7: return texture->color_space == ToolkitUtil::TextureColorSpace::TextureColorSpace_sRGB ? "BC7_UNORM_SRGB" : "BC7_UNORM";
        case ToolkitUtil::TexturePixelFormat::TexturePixelFormat_R8G8B8A8: return texture->color_space == ToolkitUtil::TextureColorSpace::TextureColorSpace_sRGB ? "R8G8B8A8_UNORM_SRGB" : "R8G8B8A8_UNORM";
        // DXGI has no 24 bit format, widen to RGBA
        case ToolkitUtil::TexturePixelFormat::TexturePixelFormat_R8G8B8: return texture->color_space == ToolkitUtil::TextureColorSpace::TextureColorSpace_sRGB ? "R8G8B8A8_UNORM_SRGB" : "R8G8B8A8_UNORM";
        case ToolkitUtil::TexturePixelFormat::TexturePixelFormat_R16: return "R16_UNORM";
    }
    return "BC7_UNORM";
}
#else
//------------------------------------------------------------------------------
/**
    Returns the compressonatorcli destination format (-fd). The cli has no
    single channel or 24 bit targets, so those are widened.
*/
static const char*
GetConverterFormatString(ToolkitUtil::TextureResourceT const* texture)
{
    switch (texture->target_format)
    {
        case ToolkitUtil::TexturePixelFormat::TexturePixelFormat_BC1: return "BC1";
        case ToolkitUtil::TexturePixelFormat::TexturePixelFormat_BC2: return "BC2";
        case ToolkitUtil::TexturePixelFormat::TexturePixelFormat_BC3: return "BC3";
        case ToolkitUtil::TexturePixelFormat::TexturePixelFormat_BC5: return "BC5";
        case ToolkitUtil::TexturePixelFormat::TexturePixelFormat_BC6H: return "BC6H";
        case ToolkitUtil::TexturePixelFormat::TexturePixelFormat_BC7: return "BC7";
        case ToolkitUtil::TexturePixelFormat::TexturePixelFormat_R8G8B8A8: return "ARGB_8888";
        case ToolkitUtil::TexturePixelFormat::TexturePixelFormat_R8G8B8: return "ARGB_8888";
        case ToolkitUtil::TexturePixelFormat::TexturePixelFormat_R16: return "ARGB_16F";
    }
    return "BC7";
}

//------------------------------------------------------------------------------
/**
    Path to the compressonatorcli executable (built by toolkit/scripts/compressonator),
    can be overridden with the NEBULA_COMPRESSONATORCLI environment variable
*/
static IO::URI
GetConverterToolPath()
{
    if (System::Environment::Exists("NEBULA_COMPRESSONATORCLI"))
    {
        return System::Environment::Read("NEBULA_COMPRESSONATORCLI");
    }
    return Util::Format("toolkit:bin/%s/compressonatorcli", System::PlatformTypeAsString(System::Platform));
}
#endif

//------------------------------------------------------------------------------
/**
*/
bool
ConvertTexture(const TextureConversionInfo& info)
{
#if (__WIN32__)    
    IO::IoServer::Instance()->CreateDirectory(info.destPath);
    URI srcPathUri(info.sourcePath);
    URI dstPathUri(info.destPath);
    URI tmpDirUri(info.tmpDir);

    String args = " -y -sepalpha -dx10 -nologo -timing ";
    if (info.texture->compression_quality == ToolkitUtil::TextureCompressionQuality::TextureCompressionQuality_High)
    {
        args.Append(" -bc x ");
    }
    else
    {
        args.Append(" -bc q ");
    }

    if (info.texture->invert_green)
    {
        args.Append(" -inverty ");
    }
    if (!info.texture->generate_mipmaps)
    {
        args.Append(" -m 1 ");
    }

    if (info.texture->color_space == ToolkitUtil::TextureColorSpace::TextureColorSpace_sRGB)
    {
        args.Append(" -srgbi ");
        args.Append(" -srgbo ");
    }

    args.Append("-ft dds");
    args.Append(" -f ");
    args.Append(GetConverterFormatString(info.texture));

    args.Append(" \"");
    Util::String srcPath = IO::IoServer::NativePath(srcPathUri.LocalPath());
    args.Append(srcPath);
    args.Append("\" -o \"");
    args.Append(info.destPath);
    args.Append("\"");

    System::ProcessStartInfo startInfo;
    startInfo.args = args;
    startInfo.workingDir = info.sourcePath.ExtractDirName();
    startInfo.exePath = Util::Format("toolkit:bin/%s/texconv", System::PlatformTypeAsString(System::Platform));
    startInfo.consoleWindow = false;

    System::ProcessId process = System::StartProcess(startInfo);
    if (process == System::InvalidProcessId)
    {
        info.logger->Warning("Failed to launch converter tool '%s'!\n", startInfo.exePath.LocalPath().AsCharPtr());
        return false;
    }
    System::WaitForProcess(process);
#else
    IO::IoServer::Instance()->CreateDirectory(info.destPath);
    URI srcPathUri(info.sourcePath);

    String args = " -noprogress -silent ";
    args.Append(" -fd ");
    args.Append(GetConverterFormatString(info.texture));

    if (info.texture->generate_mipmaps)
    {
        // clamped to the number of levels the image supports
        args.Append(" -miplevels 20 ");
    }
    else
    {
        args.Append(" -nomipmap ");
    }

    if (info.texture->compression_quality == ToolkitUtil::TextureCompressionQuality::TextureCompressionQuality_High)
    {
        args.Append(" -Quality 0.7 ");
    }
    else
    {
        args.Append(" -Quality 0.1 ");
    }
    args.Append(" -NumThreads 0 ");
    // the linux build has no gpu encoders, HPC is the fastest available
    args.Append(" -EncodeWith HPC ");

    if (info.texture->color_space == ToolkitUtil::TextureColorSpace::TextureColorSpace_sRGB)
    {
        // writes BC1/BC2/BC3/BC7 and RGBA8 as *_SRGB DX10 formats (Nebula patch, see toolkit/scripts/compressonator)
        args.Append(" -OutputSRGB ");
    }

    String dstName = srcPathUri.LocalPath().ExtractFileName();
    dstName.StripFileExtension();
    String dstFile = Util::Format("%s/%s.dds", URI(info.destPath).LocalPath().AsCharPtr(), dstName.AsCharPtr());
    args.Append(" \"");
    args.Append(srcPathUri.LocalPath());
    args.Append("\" \"");
    args.Append(dstFile);
    args.Append("\"");

    System::ProcessStartInfo startInfo;
    startInfo.args = args;
    startInfo.workingDir = info.sourcePath.ExtractDirName();
    startInfo.exePath = GetConverterToolPath();
    startInfo.consoleWindow = false;

    if (!IO::IoServer::Instance()->FileExists(startInfo.exePath))
    {
        info.logger->Error("Converter tool '%s' not found, place compressonatorcli there or set NEBULA_COMPRESSONATORCLI\n", startInfo.exePath.LocalPath().AsCharPtr());
        return false;
    }

    System::ProcessId process = System::StartProcess(startInfo);
    if (process == System::InvalidProcessId)
    {
        info.logger->Warning("Failed to launch converter tool '%s'!\n", startInfo.exePath.LocalPath().AsCharPtr());
        return false;
    }
    uint exitCode = System::WaitForProcess(process);
    if (exitCode != 0)
    {
        info.logger->Error("Converter tool failed on '%s', exit code: %d\n", srcPathUri.LocalPath().AsCharPtr(), exitCode);
        return false;
    }
#endif

    return true;
}

} // namespace ToolkitUtil