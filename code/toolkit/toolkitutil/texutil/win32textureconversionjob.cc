//------------------------------------------------------------------------------
// win32textureconversionjob.cc
// (C) 2009 Radon Labs GmbH
//  (C) 2013-2014 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "win32textureconversionjob.h"
#include "io/uri.h"
#include "io/assignregistry.h"
#include "io/ioserver.h"

namespace ToolkitUtil
{
using namespace IO;
using namespace Util;

//------------------------------------------------------------------------------
/**
*/
Win32TextureConversionJob::Win32TextureConversionJob()
{
    this->SetDstFileExtension("dds");
}

//------------------------------------------------------------------------------
/**
    Start the conversion process. Returns false, if the job finished immediately.	
*/
bool
Win32TextureConversionJob::Convert()
{
    n_assert(this->toolPath.IsValid());
    n_assert(0 != this->logger);
    if (TextureConversionJob::Convert())
    {  
        URI srcPathUri(this->srcPath);
        URI dstPathUri(this->dstPath);
        URI tmpDirUri(this->tmpDir);
        
        // build command line args for nvdxt
        String args = " -rescale lo -overwrite ";
        bool isDXT5NormalMap = false;
        const TextureAttrs& attrs = this->textureAttrs;
        if ((attrs.GetRGBPixelFormat() == TextureAttrs::DXT5NM) ||
            (attrs.GetRGBAPixelFormat() == TextureAttrs::DXT5NM) ||
			(String::MatchPattern(this->srcPath, "*norm.*")) ||
			(String::MatchPattern(this->srcPath, "*normal.*")) ||
			(String::MatchPattern(this->srcPath, "*bump.*")))
        {
            args.Append(" -dxt5nm ");
            isDXT5NormalMap = true;
        }
        else
        {
            args.Append(" -24 ");
            args.Append(TextureAttrs::PixelFormatToString(attrs.GetRGBPixelFormat()));
            args.Append(" -32 ");
            args.Append(TextureAttrs::PixelFormatToString(attrs.GetRGBAPixelFormat()));
            args.Append(" ");
        }
        args.Append(" -clampScale ");
        args.AppendInt(attrs.GetMaxWidth());
        args.Append(" ");
        args.AppendInt(attrs.GetMaxHeight());
        if (attrs.GetQuality() == TextureAttrs::Normal)
        {
            args.Append(" -quality_normal ");
        }
        else if (attrs.GetQuality() == TextureAttrs::High)
        {
            args.Append(" -quality_production ");
        }
        else if (isDXT5NormalMap)
        {
            // DXT5NM conversion has a bug in quick mode
            args.Append(" -quality_normal ");
        }
        else
        {
            // quick quality
            args.Append(" -quick ");
        }
        if (!attrs.GetGenMipMaps())
        {
            args.Append(" -nomipmap ");
        }
        args.Append("-");
        args.Append(TextureAttrs::FilterToString(attrs.GetMipMapFilter()));
        args.Append(" ");
        if (isDXT5NormalMap)
        {
            args.Append(" -norm ");
        }
        args.Append(" -Rescale");
        args.Append(TextureAttrs::FilterToString(attrs.GetScaleFilter()));
        args.Append(" -file \"");
        args.Append(srcPathUri.LocalPath());
        
        args.Append("\" -outdir \"");
        args.Append(tmpDirUri.LocalPath());
        args.Append("\"");

        args.Append(" -outfile \"");
        args.Append(dstPathUri.LocalPath().ExtractFileName());
        args.Append("\"");

        // launch nvdxt to perform the conversion        
        this->appLauncher.SetExecutable(this->toolPath);
        this->appLauncher.SetWorkingDirectory(this->srcPath.ExtractDirName());
        this->appLauncher.SetArguments(args);
#if __WIN32__
        this->appLauncher.SetNoConsoleWindow(this->quiet);
#endif        
        if (!this->appLauncher.LaunchWait())
        {
            this->logger->Warning("Failed to launch converter tool '%s'!\n", this->toolPath.AsCharPtr());
        }

        // copy converted texture from temp to export dir
        if (!this->CopyResult())
        {
            return false;
        }
    }
    return true;
}

}// namespace ToolkitUtil