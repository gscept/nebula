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
#include "toolkit-common/logger.h"


#if !(__WIN32__)
#include "Compressonator.h"
#endif
#include "timing/timer.h"

#include "toolkit-common/text.h"

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
        case ToolkitUtil::TexturePixelFormat::TexturePixelFormat_BC1: return "DXT1";
        case ToolkitUtil::TexturePixelFormat::TexturePixelFormat_BC2: return texture->color_space == ToolkitUtil::TextureColorSpace::TextureColorSpace_sRGB ? "BC2_UNORM_SRGB" : "BC2_UNORM";
        case ToolkitUtil::TexturePixelFormat::TexturePixelFormat_BC3: return texture->color_space == ToolkitUtil::TextureColorSpace::TextureColorSpace_sRGB ? "BC3_UNORM_SRGB" : "BC3_UNORM";
        case ToolkitUtil::TexturePixelFormat::TexturePixelFormat_BC5: return "BC5_UNORM";
        case ToolkitUtil::TexturePixelFormat::TexturePixelFormat_BC6H: return "BC6H_UF16";
        case ToolkitUtil::TexturePixelFormat::TexturePixelFormat_BC7: return texture->color_space == ToolkitUtil::TextureColorSpace::TextureColorSpace_sRGB ? "BC7_UNORM_SRGB" : "BC7_UNORM";
        case ToolkitUtil::TexturePixelFormat::TexturePixelFormat_R8G8B8A8: return texture->color_space == ToolkitUtil::TextureColorSpace::TextureColorSpace_sRGB ? "R8G8B8A8_UNORM_SRGB" : "R8G8B8A8_UNORM";
        case ToolkitUtil::TexturePixelFormat::TexturePixelFormat_R8G8B8: return texture->color_space == ToolkitUtil::TextureColorSpace::TextureColorSpace_sRGB ? "R8G8B8_UNORM_SRGB" : "R8G8B8_UNORM";
        case ToolkitUtil::TexturePixelFormat::TexturePixelFormat_R16: return "R16_UNORM";
    }
    return "BC7_UNORM";
}
#else
//------------------------------------------------------------------------------
/**
*/
static CMP_FORMAT
GetConverterFormatString(ToolkitUtil::TextureResourceT const* texture)
{
    switch (texture->target_format)
    {
        case ToolkitUtil::TexturePixelFormat::TexturePixelFormat_BC1: return CMP_FORMAT_BC1;
        case ToolkitUtil::TexturePixelFormat::TexturePixelFormat_BC2: return CMP_FORMAT_BC2;
        case ToolkitUtil::TexturePixelFormat::TexturePixelFormat_BC3: return CMP_FORMAT_BC3;
        case ToolkitUtil::TexturePixelFormat::TexturePixelFormat_BC5: return CMP_FORMAT_BC5;
        case ToolkitUtil::TexturePixelFormat::TexturePixelFormat_BC6H: return CMP_FORMAT_BC6H;
        case ToolkitUtil::TexturePixelFormat::TexturePixelFormat_BC7: CMP_FORMAT_BC7;
        case ToolkitUtil::TexturePixelFormat::TexturePixelFormat_R8G8B8A8: CMP_FORMAT_RGBA_8888;
        case ToolkitUtil::TexturePixelFormat::TexturePixelFormat_R8G8B8: CMP_FORMAT_RGB_888;
        case ToolkitUtil::TexturePixelFormat::TexturePixelFormat_R16: return CMP_FORMAT_R_16;
    }
    return CMP_FORMAT_BC7;
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

    String args = "";
    if (info.cube)
    {
        args.Append("cube ");
    }
    args.Append(" -y -sepalpha -dx10 -nologo -timing ");
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
        args.Append(" -srgb ");
    }
    else
    {
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
    URI dstPathUri(info.destPath);
    URI tmpDirUri(info.tmpDir);

    String src = srcPathUri.LocalPath();

    Timing::Timer timer;
    timer.Start();

    info.logger->Print("Processing: %s\n", src.AsCharPtr());

    CMP_MipSet MipSetIn;
    Memory::Clear(&MipSetIn, sizeof(CMP_MipSet));

    auto cmp_status = CMP_LoadTexture(src.AsCharPtr(), &MipSetIn);

    if (cmp_status != CMP_OK)
    {
        info.logger->Error("Failed to load %s, error code: %d\n", src.AsCharPtr(), cmp_status);
        return false;
    }


    if (MipSetIn.m_format != CMP_FORMAT_RGBA_8888)
    {
        info.logger->Print("Non-rgb(a) texture, saving raw\n");

        cmp_status = CMP_SaveTexture(dstPathUri.LocalPath().AsCharPtr(), &MipSetIn);
        if (cmp_status != CMP_OK)
        {
            info.logger->Error("Failed to load %s, CMP_SaveTexture, error code: %d\n", src.AsCharPtr(), cmp_status);
            return false;
        }

        CMP_FreeMipSet(&MipSetIn);

        Timing::Time after = timer.GetTime();
        info.logger->Print("Done after: %f\n", after);
        return true;
    }


    if (MipSetIn.m_nMipLevels <= 1)
    {
        CMP_INT requestLevel = 10; // Request 10 miplevels for the source image

        //------------------------------------------------------------------------
        // Checks what the minimum image size will be for the requested mip levels
        // if the request is too large, a adjusted minimum size will be returned
        //------------------------------------------------------------------------
        CMP_INT nMinSize = CMP_CalcMinMipSize(MipSetIn.m_nHeight, MipSetIn.m_nWidth, 10);

        //--------------------------------------------------------------
        // now that the minimum size is known, generate the miplevels
        // users can set any requested minumum size to use. The correct
        // miplevels will be set acordingly.
        //--------------------------------------------------------------
        CMP_GenerateMIPLevels(&MipSetIn, nMinSize);
    }

    //==========================
    // Set Compression Options
    //==========================
    KernelOptions   kernel_options;
    memset(&kernel_options, 0, sizeof(KernelOptions));

    float quality = 0.5f;
    switch (info.texture->compression_quality)
    {
        case ToolkitUtil::TextureCompressionQuality_High:
            quality = 0.7f;
            break;
        case ToolkitUtil::TextureCompressionQuality_Low:
            quality = 0.1f;
            break;
    }

    kernel_options.format = GetConverterFormatString(info.texture);
    kernel_options.useSRGBFrames = (info.texture->color_space == ToolkitUtil::TextureColorSpace::TextureColorSpace_sRGB) ? true : false;

    kernel_options.fquality = quality;     // Set the quality of the result
    kernel_options.threads = 0;             // Auto setting
    kernel_options.encodeWith = CMP_CPU;

    //--------------------------------------------------------------
    // Setup a results buffer for the processed file,
    // the content will be set after the source texture is processed
    // in the call to CMP_ProcessTexture()
    //--------------------------------------------------------------
    CMP_MipSet MipSetCmp;
    memset(&MipSetCmp, 0, sizeof(CMP_MipSet));

    CMP_Feedback_Proc CompressionCallback = [](CMP_FLOAT fProgress, CMP_DWORD_PTR pUser1, CMP_DWORD_PTR pUser2)->bool
    {
        return 0;
    };

    //===============================================
    // Compress the texture using Framework Lib
    //===============================================
    cmp_status = CMP_ProcessTexture(&MipSetIn, &MipSetCmp, kernel_options, CompressionCallback);
    if (cmp_status != CMP_OK)
    {
        info.logger->Error("Failed to load %s, CMP_ProcessTexture, error code: %d\n", src.AsCharPtr(), cmp_status);
        return false;
    }

    //----------------------------------------------------------------
    // Save the result into a DDS file
    //----------------------------------------------------------------
    cmp_status = CMP_SaveTexture(dstPathUri.LocalPath().AsCharPtr(), &MipSetCmp);
    if (cmp_status != CMP_OK)
    {
        info.logger->Error("Failed to load %s, CMP_SaveTexture, error code: %d\n", src.AsCharPtr(), cmp_status);
        return false;
    }

    CMP_FreeMipSet(&MipSetIn);
    CMP_FreeMipSet(&MipSetCmp);

    Timing::Time after = timer.GetTime();
    info.logger->Print("Done after: %f\n", after);
#endif

    return true;
}

} // namespace ToolkitUtil