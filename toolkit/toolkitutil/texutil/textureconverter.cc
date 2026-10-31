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


//------------------------------------------------------------------------------
/**
*/
static const char*
GetTexconvFormatString(ToolkitUtil::TextureResourceT const* texture)
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

//------------------------------------------------------------------------------
/**
    Converts with DirectXTex's texconv, used for all formats on Windows
*/
static bool
ConvertWithTexconv(const TextureConversionInfo& info)
{
    IO::IoServer::Instance()->CreateDirectory(info.destPath);
    URI srcPathUri(info.sourcePath);

    // the texture's color space decides if the image is sRGB, not the file's metadata
    String args = " -y -sepalpha -dx10 -nologo -timing --ignore-srgb ";
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
    args.Append(GetTexconvFormatString(info.texture));

    // '--' ends the options, texconv would take absolute linux paths for options otherwise
    args.Append(" -o \"");
    args.Append(info.destPath);
    args.Append("\" -- \"");
    Util::String srcPath = IO::IoServer::NativePath(srcPathUri.LocalPath());
    args.Append(srcPath);
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
    uint exitCode = System::WaitForProcess(process);
    if (exitCode != 0)
    {
        info.logger->Error("Converter tool failed on '%s', exit code: %d\n", srcPathUri.LocalPath().AsCharPtr(), exitCode);
        return false;
    }
    return true;
}

#if !(__WIN32__)
//------------------------------------------------------------------------------
/**
    Returns the compressonatorcli destination format (-fd), only used for
    the block compressed formats
*/
static const char*
GetCompressonatorFormatString(ToolkitUtil::TextureResourceT const* texture)
{
    switch (texture->target_format)
    {
        case ToolkitUtil::TexturePixelFormat::TexturePixelFormat_BC1: return "BC1";
        case ToolkitUtil::TexturePixelFormat::TexturePixelFormat_BC2: return "BC2";
        case ToolkitUtil::TexturePixelFormat::TexturePixelFormat_BC3: return "BC3";
        case ToolkitUtil::TexturePixelFormat::TexturePixelFormat_BC5: return "BC5";
        case ToolkitUtil::TexturePixelFormat::TexturePixelFormat_BC6H: return "BC6H";
        case ToolkitUtil::TexturePixelFormat::TexturePixelFormat_BC7: return "BC7";
        default: break;
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

//------------------------------------------------------------------------------
/**
    compressonatorcli loads .hdr images as 8 bit and compresses DDS files with 32 bit
    float pixels to broken BC6H. Both are fine as 16 bit float DDS.
*/
static bool
NeedsHalfFloatDDS(const IO::URI& srcPath)
{
    Util::String ext = srcPath.LocalPath().GetFileExtension();
    if (ext == "hdr")
    {
        return true;
    }
    if (ext != "dds")
    {
        return false;
    }

    // magic and DDS_HEADER, followed by DDS_HEADER_DXT10 if the pixel format's fourcc is DX10
    const SizeT FourCCOffset = 84, DXGIFormatOffset = 128;
    const uint32_t D3DFMT_A32B32G32R32F = 116, DXGI_FORMAT_R32G32B32A32_FLOAT = 2;
    uint8_t header[DXGIFormatOffset + 4] = {};
    Ptr<IO::Stream> stream = IO::IoServer::Instance()->CreateStream(srcPath);
    stream->SetAccessMode(IO::Stream::ReadAccess);
    if (!stream->Open())
    {
        return false;
    }
    stream->Read(header, sizeof(header));
    stream->Close();

    uint32_t fourCC, format;
    memcpy(&fourCC, header + FourCCOffset, 4);
    memcpy(&format, header + DXGIFormatOffset, 4);
    return fourCC == D3DFMT_A32B32G32R32F || (memcmp(header + FourCCOffset, "DX10", 4) == 0 && format == DXGI_FORMAT_R32G32B32A32_FLOAT);
}

//------------------------------------------------------------------------------
/**
    Converts the image to a 16 bit float DDS in dstDir with texconv, returns its path
    or an empty string
*/
static Util::String
ConvertToHalfFloatDDS(const IO::URI& srcPath, const IO::URI& dstDir, Logger* logger)
{
    IO::IoServer::Instance()->CreateDirectory(dstDir);
    Util::String args = " -nologo -y -ft dds -f R16G16B16A16_FLOAT -m 1 -o \"";
    args.Append(dstDir.LocalPath());
    args.Append("\" -- \"");
    args.Append(srcPath.LocalPath());
    args.Append("\"");

    System::ProcessStartInfo startInfo;
    startInfo.args = args;
    startInfo.workingDir = dstDir;
    startInfo.exePath = Util::Format("toolkit:bin/%s/texconv", System::PlatformTypeAsString(System::Platform));
    startInfo.consoleWindow = false;

    System::ProcessId process = System::StartProcess(startInfo);
    if (process == System::InvalidProcessId)
    {
        logger->Warning("Failed to launch texconv '%s'!\n", startInfo.exePath.LocalPath().AsCharPtr());
        return "";
    }
    uint exitCode = System::WaitForProcess(process);
    if (exitCode != 0)
    {
        logger->Error("texconv failed to convert '%s', exit code: %d\n", srcPath.LocalPath().AsCharPtr(), exitCode);
        return "";
    }
    Util::String dstName = srcPath.LocalPath().ExtractFileName();
    dstName.StripFileExtension();
    return Util::Format("%s/%s.dds", dstDir.LocalPath().AsCharPtr(), dstName.AsCharPtr());
}

//------------------------------------------------------------------------------
/**
    Compresses with compressonatorcli, used for the block compressed formats on Linux
*/
static bool
ConvertWithCompressonator(const TextureConversionInfo& info)
{
    IO::IoServer::Instance()->CreateDirectory(info.destPath);
    URI srcPathUri(info.sourcePath);

    IO::URI toolPath = GetConverterToolPath();
    if (!IO::IoServer::Instance()->FileExists(toolPath))
    {
        info.logger->Error("Converter tool '%s' not found, place compressonatorcli there or set NEBULA_COMPRESSONATORCLI\n", toolPath.LocalPath().AsCharPtr());
        return false;
    }

    String args = " -noprogress -silent ";
    args.Append(" -fd ");
    args.Append(GetCompressonatorFormatString(info.texture));

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
        // writes BC1/BC2/BC3/BC7 as *_SRGB DX10 formats (Nebula patch, see toolkit/scripts/compressonator)
        args.Append(" -OutputSRGB ");
    }

    String srcFile = srcPathUri.LocalPath();
    String halfFile;
    if (NeedsHalfFloatDDS(srcPathUri))
    {
        // in a sub folder, the source can be a DDS with the same name
        halfFile = ConvertToHalfFloatDDS(srcPathUri, info.tmpDir + "/halffloat", info.logger);
        if (halfFile.IsEmpty())
        {
            return false;
        }
        srcFile = halfFile;
    }

    String dstName = srcPathUri.LocalPath().ExtractFileName();
    dstName.StripFileExtension();
    String dstFile = Util::Format("%s/%s.dds", URI(info.destPath).LocalPath().AsCharPtr(), dstName.AsCharPtr());
    args.Append(" \"");
    args.Append(srcFile);
    args.Append("\" \"");
    args.Append(dstFile);
    args.Append("\"");

    System::ProcessStartInfo startInfo;
    startInfo.args = args;
    startInfo.workingDir = info.sourcePath.ExtractDirName();
    startInfo.exePath = toolPath;
    startInfo.consoleWindow = false;

    System::ProcessId process = System::StartProcess(startInfo);
    uint exitCode = process != System::InvalidProcessId ? System::WaitForProcess(process) : 0;
    if (!halfFile.IsEmpty())
    {
        IO::IoServer::Instance()->DeleteFile(halfFile);
    }
    if (process == System::InvalidProcessId)
    {
        info.logger->Warning("Failed to launch converter tool '%s'!\n", startInfo.exePath.LocalPath().AsCharPtr());
        return false;
    }
    if (exitCode != 0)
    {
        info.logger->Error("Converter tool failed on '%s', exit code: %d\n", srcPathUri.LocalPath().AsCharPtr(), exitCode);
        return false;
    }
    return true;
}
#endif

//------------------------------------------------------------------------------
/**
*/
bool
ConvertTexture(const TextureConversionInfo& info)
{
#if (__WIN32__)
    return ConvertWithTexconv(info);
#else
    switch (info.texture->target_format)
    {
        // compressonatorcli can't write uncompressed cube maps or single channel formats,
        // texconv converts them like on Windows
        case ToolkitUtil::TexturePixelFormat::TexturePixelFormat_R8G8B8A8:
        case ToolkitUtil::TexturePixelFormat::TexturePixelFormat_R8G8B8:
        case ToolkitUtil::TexturePixelFormat::TexturePixelFormat_R16:
            return ConvertWithTexconv(info);
        default:
            return ConvertWithCompressonator(info);
    }
#endif
}

} // namespace ToolkitUtil
