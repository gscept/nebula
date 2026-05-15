//------------------------------------------------------------------------------
// win32textureconversionjob.cc
// (C) 2009 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "directxtexconversionjob.h"
#include "io/uri.h"
#include "io/assignregistry.h"
#include "io/ioserver.h"
#include "system/systeminfo.h"

#include "toolkit-common/text.h"

namespace ToolkitUtil
{
using namespace IO;
using namespace Util;

//------------------------------------------------------------------------------
/**
*/
DirectXTexConversionJob::DirectXTexConversionJob()
{
    System::SystemInfo systemInfo;
    this->toolPath.Format("toolkit:bin/%s/texconv", System::PlatformTypeAsString(System::Platform));
    this->cubeToolPath.Format("toolkit:bin/%s/texassemble", System::PlatformTypeAsString(System::Platform));
    this->SetDstFileExtension("dds");
}

//------------------------------------------------------------------------------
/**
*/
static const char*
GetTexConvFormat(ToolkitUtil::TextureResourceT const* texture)
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

//------------------------------------------------------------------------------
/**
    Start the conversion process. Returns false, if the job finished immediately.
*/
bool
DirectXTexConversionJob::Convert(const ToolkitUtil::TextureResourceT* texture)
{
    n_assert(this->toolPath.IsValid());
    n_assert(0 != this->logger);
    if (TextureConversionJob::Convert())
    {
        URI srcPathUri(this->srcPath);
        URI dstPathUri(this->dstPath);
        URI tmpDirUri(this->tmpDir);

        String args = " -y -sepalpha -dx10 -nologo -timing ";
        if (texture->compression_quality == ToolkitUtil::TextureCompressionQuality::TextureCompressionQuality_High)
        {
            args.Append(" -bc x ");
        }
        else
        {
            args.Append(" -bc q ");
        }

        if (!texture->generate_mipmaps)
        {
            args.Append(" -m 1 ");
        }

        if (texture->color_space == ToolkitUtil::TextureColorSpace::TextureColorSpace_sRGB)
        {
            args.Append(" -srgb ");
        }
        else
        {
            args.Append(" -srgbo ");
        }

        args.Append(" -f ");
        args.Append(GetTexConvFormat(texture));

        args.Append(" \"");
        Util::String srcPath = IO::IoServer::NativePath(srcPathUri.LocalPath());
        //srcPath.ReplaceChars("/", '\\');
        args.Append(srcPath);
        args.Append("\" -o \"");
        Util::String tmpPath = IO::IoServer::NativePath(tmpDirUri.LocalPath());
        //tmpPath.ReplaceChars("/", '\\');
        args.Append(tmpPath);
        args.Append("\"");

        // launch texconv to perform the conversion        
        this->appLauncher.SetNoConsoleWindow(true);
        this->appLauncher.SetExecutable(this->toolPath);
        this->appLauncher.SetWorkingDirectory(this->srcPath.ExtractDirName());
        this->appLauncher.SetArguments(args);
#if __WIN32__
        //this->appLauncher.SetNoConsoleWindow(this->quiet);
#endif        
        if (!this->appLauncher.LaunchWait())
        {
            this->logger->Warning("Failed to launch converter tool '%s'!\n", this->toolPath.AsCharPtr());
            return false;
        }

        // copy converted texture from temp to export dir
        if (!this->CopyResult())
        {
            return false;
        }

        ToolkitUtil::Text print = Util::String::Sprintf("%s -> %s... ", Text(URI(this->srcPath).LocalPath()).Color(TextColor::Blue).AsCharPtr(), Text(Format("%s", URI(this->dstPath).LocalPath().AsCharPtr())).Color(TextColor::Green).Style(FontMode::Underline).AsCharPtr());
        this->logger->Print(Util::String::Sprintf("%s%s", print.AsCharPtr(), "done\n"_text.Color(TextColor::Green).AsCharPtr()).AsCharPtr());
    }

    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
DirectXTexConversionJob::ConvertCube(const ToolkitUtil::TextureResourceT* texture)
{
    n_assert(this->toolPath.IsValid());
    n_assert(0 != this->logger);

    //FIXME this is some hacky shit
    this->tmpDir.Append("/");
    this->tmpDir.Append(this->srcPath.ExtractLastDirName().AsCharPtr());
    this->neverCopy = true;
    if (TextureConversionJob::Convert())
    {
        URI srcPathUri(this->srcPath);
        URI dstPathUri(this->dstPath);
        URI tmpDirUri(this->tmpDir);

        String args = "";
        Array<String> files = IoServer::Instance()->ListFiles(this->srcPath, "*.*");
        if (files.Size() != 6)
        {
            this->logger->Warning("Exactly 6 images are required for a cubemap!\n");
            return false;
        }

        files.Sort();

        args.Append("cube -nologo -o \"");

        Util::String tmpPath = tmpDirUri.LocalPath();
        tmpPath.ReplaceChars("/", '\\');
        tmpPath.Append("\\");
        tmpPath.Append(this->dstPath.ExtractFileName());
        args.Append(tmpPath);
        args.Append("\"");

        for (int i = 0, n = files.Size(); i < n; i++)
        {
            args.Append(" ");
            args.Append(files[i].AsCharPtr());
        }

        this->appLauncher.SetExecutable(this->cubeToolPath);
        this->appLauncher.SetWorkingDirectory(this->srcPath);
        this->appLauncher.SetArguments(args);
#if __WIN32__
        //this->appLauncher.SetNoConsoleWindow(this->quiet);
#endif        
        if (!this->appLauncher.LaunchWait())
        {
            this->logger->Warning("Failed to launch converter tool '%s'!\n", this->cubeToolPath.AsCharPtr());
            return false;
        }

        this->srcPath = tmpPath;
        return this->Convert(texture);
    }
    return false;
}
}// namespace ToolkitUtil
