//------------------------------------------------------------------------------
// win32textureconversionjob.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"

#include "nvtttextureconversionjob.h"
#include "io/uri.h"
#include "io/assignregistry.h"
#include "io/ioserver.h"
#include "timing/timer.h"

#include <nvtt/nvtt.h>

#include <nvimage/Image.h> 
#include <nvimage/ImageIO.h>
#include <nvimage/FloatImage.h>
#include <nvimage/Filter.h>
#include <nvimage/DirectDrawSurface.h>

#include <nvcore/Ptr.h> // AutoPtr
#include <nvcore/StrLib.h> // Path
#include <nvcore/StdStream.h>
#include <nvcore/FileSystem.h>
#include <nvcore/Timer.h>
#include <lua/lobject.h>


namespace ToolkitUtil
{
using namespace IO;
using namespace Util;


//------------------------------------------------------------------------------
/**
*/
static nvtt::Format TextureAttrToNVTT(ToolkitUtil::TextureAttrs::PixelFormat format)
{
    switch(format)
    {
        case TextureAttrs::DXT1C:
            return nvtt::Format_DXT1;
        case TextureAttrs::DXT1A:
            return nvtt::Format_DXT1a;
        case TextureAttrs::DXT3:
            return nvtt::Format_DXT3;
        case TextureAttrs::DXT5:
            return nvtt::Format_DXT5;
        case TextureAttrs::DXT5NM:
            return nvtt::Format_DXT5n;
		case TextureAttrs::U8888:
			return nvtt::Format_RGBA;
		case TextureAttrs::U888:
			return nvtt::Format_RGB;
        default:
            n_error("unsupported texture compression format");
            return nvtt::Format_RGB;
    }
        
}

//------------------------------------------------------------------------------
/**
*/
NVTTTextureConversionJob::NVTTTextureConversionJob()
{
    this->SetDstFileExtension("dds");
}

//------------------------------------------------------------------------------
/**
    Start the conversion process. Returns false, if the job finished immediately.   
*/
bool
NVTTTextureConversionJob::Convert()
{
    n_assert(0 != this->logger);
    if (TextureConversionJob::Convert())
    {  
        URI srcPathUri(this->srcPath);
        URI dstPathUri(this->dstPath);
        URI tmpDirUri(this->tmpDir);
        
        String src = srcPathUri.LocalPath();                      
        if(src.CheckFileExtension("dds"))
        {
            // Load surface.
            nv::DirectDrawSurface dds(src.AsCharPtr());
            if (!dds.isValid())
            {
                this->logger->Warning("The file '%s' is not a valid DDS file.\n", src.AsCharPtr());
                return false;
            }

            if (!dds.isSupported() || dds.isTexture3D())
            {
                this->logger->Warning("The file '%s' is not a supported DDS file.\n", src.AsCharPtr());            
                return false;
            }
            IoServer::Instance()->CopyFile(this->srcPath, this->dstPath);
            
            return true;            
        }
        
        // we do everything in float so that we can resize
        nv::FloatImage *image;        
        nv::AutoPtr<nv::Image> outputImage;
        nvtt::InputOptions inputOptions;
        
		Timing::Timer timer;
		timer.Start();		
		this->logger->Print("Processing: %s\n", src.AsCharPtr());

		nvtt::OutputOptions outputOptions;
        if (src.CheckFileExtension("exr") || src.CheckFileExtension("hdr"))
        {
            image = nv::ImageIO::loadFloat(src.AsCharPtr());
			outputOptions.setContainer(nvtt::Container_DDS10);

            if (image == NULL)
            {
                this->logger->Warning("The file '%s' is not a supported image type.\n", src.AsCharPtr());                    
                return false;
            }                        
        }
        else
        {
            // Regular image.            
			nv::Image rawimage;
            if (!rawimage.load(src.AsCharPtr()))
            {
                this->logger->Warning("The file '%s' is not a supported image type.\n", src.AsCharPtr());                
                return false;
            }
            
            image = new nv::FloatImage(&rawimage);            
        }
        
        bool isDXT5NormalMap = false;
        const TextureAttrs& attrs = this->textureAttrs;
		
		ToolkitUtil::TextureAttrs::PixelFormat targetformat;
		float gamma = 1.0f;
        if ((attrs.GetRGBPixelFormat() == TextureAttrs::DXT5NM) ||
            (attrs.GetRGBAPixelFormat() == TextureAttrs::DXT5NM) ||
            (String::MatchPattern(this->srcPath, "*norm.*"))||
            (String::MatchPattern(this->srcPath, "*normal.*"))||
            (String::MatchPattern(this->srcPath, "*bump.*")))
        {
            inputOptions.setNormalMap(true);
            inputOptions.setConvertToNormalMap(false);
            inputOptions.setGamma(1.0f, 1.0f);
            inputOptions.setNormalizeMipmaps(false);   
            isDXT5NormalMap = true;
			targetformat = TextureAttrs::DXT5NM;			
        }
		else
		{
            if (image->componentCount() > 3)    targetformat = attrs.GetRGBAPixelFormat();
            else                                targetformat = attrs.GetRGBPixelFormat();
			
			if (attrs.GetColorSpace() == TextureAttrs::Linear)
			{
				gamma = 1.0f;
				inputOptions.setGamma(1.0f, 1.0f);
			}
			else if (attrs.GetColorSpace() == TextureAttrs::sRGB)
			{
				gamma = 2.2f;
				inputOptions.setGamma(2.2f, 2.2f);
				outputOptions.setContainer(nvtt::Container_DDS10);
			}
		}
        
        nvtt::CompressionOptions compressionOptions;
        compressionOptions.setFormat(TextureAttrToNVTT(targetformat));
        
        if (attrs.GetQuality() == TextureAttrs::Normal)  compressionOptions.setQuality(nvtt::Quality_Normal);
        else if (attrs.GetQuality() == ToolkitUtil::TextureAttrs::High)     compressionOptions.setQuality(nvtt::Quality_Highest);
        else if (attrs.GetQuality() == ToolkitUtil::TextureAttrs::Low)      compressionOptions.setQuality(nvtt::Quality_Fastest);
        
        if(targetformat == TextureAttrs::DXT3)
        {
            // Dither alpha when using BC2.
            compressionOptions.setQuantization(false, true, false);
        }
        else if (targetformat == TextureAttrs::DXT1A)
        {
            // Binary alpha when using BC1a.
            compressionOptions.setQuantization(false, true, true, 127);
        }
        
        bool needResize = false;
        int width = image->width();
        int height = image->height();
        if (attrs.GetMaxWidth() < width)
        {
            width = attrs.GetMaxWidth();
            needResize = true;
        }
        if (attrs.GetMaxHeight() < height)
        {
            height = attrs.GetMaxHeight();
            needResize = true;
        }
        
        if (needResize)
        {
			Timing::Time startResize = timer.GetTime();
			this->logger->Print("Resizing to %d %d using %s ....", width, height, ToolkitUtil::TextureAttrs::FilterToString(attrs.GetScaleFilter()).AsCharPtr());
            nv::AutoPtr<nv::Filter> filter;
            if (attrs.GetScaleFilter() == ToolkitUtil::TextureAttrs::Kaiser)
            {
                filter = new nv::KaiserFilter(3);
                ((nv::KaiserFilter *)filter.ptr())->setParameters(4.0f, 1.0f);
            }
            else if (attrs.GetScaleFilter() == ToolkitUtil::TextureAttrs::Box)
            {
                filter = new nv::BoxFilter();
            }
            else if (attrs.GetScaleFilter() == ToolkitUtil::TextureAttrs::Triangle)
            {
                filter = new nv::TriangleFilter();
            }
            else if (attrs.GetScaleFilter() == ToolkitUtil::TextureAttrs::Quadrat)
            {
                filter = new nv::QuadraticFilter();
            }
            else if (attrs.GetScaleFilter() == ToolkitUtil::TextureAttrs::Cubic)
            {
                filter = new nv::CubicFilter();
            }
            else
            {
                filter = new nv::LanczosFilter();
            }			

            image->toLinear(0, 3, gamma);
            nv::AutoPtr<nv::FloatImage> fresult(image->resize(*filter, width, height, nv::FloatImage::WrapMode_Clamp));
            outputImage = fresult->createImageGammaCorrect(gamma);            
            delete image;
			Timing::Time diff = timer.GetTime() - startResize;
			this->logger->Print("done after %f\n",diff);
        }
        else
        {
            outputImage = image->createImage();
            delete image;
        }
                    
        inputOptions.setTextureLayout(nvtt::TextureType_2D, width, height);
        inputOptions.setMipmapData(outputImage->pixels(),width,height);
        inputOptions.setRoundMode(nvtt::RoundMode_ToPreviousPowerOfTwo);
        
        if (!attrs.GetGenMipMaps()) inputOptions.setMipmapGeneration(false);
        
        if (attrs.GetMipMapFilter() == ToolkitUtil::TextureAttrs::Kaiser)           inputOptions.setMipmapFilter(nvtt::MipmapFilter_Kaiser);
        else if (attrs.GetMipMapFilter() == ToolkitUtil::TextureAttrs::Triangle)    inputOptions.setMipmapFilter(nvtt::MipmapFilter_Triangle);
        else if (attrs.GetMipMapFilter() == ToolkitUtil::TextureAttrs::Box)         inputOptions.setMipmapFilter(nvtt::MipmapFilter_Box);
        else                                                                        inputOptions.setMipmapFilter(nvtt::MipmapFilter_Box);
        
        outputOptions.setFileName(dstPathUri.LocalPath().AsCharPtr());
        if (!isDXT5NormalMap)   outputOptions.setSrgbFlag(attrs.GetColorSpace() == TextureAttrs::sRGB);

		Timing::Time beforeCompress = timer.GetTime();
		this->logger->Print("Compressing ...");
        nvtt::Context context;
        if (!context.process(inputOptions, compressionOptions, outputOptions))
        {
			this->logger->Print("Failed!\n");
            return false;
        }
		Timing::Time after = timer.GetTime();
		this->logger->Print("Done after %f, total time: %f\n", after - beforeCompress, after);
        return true;         
    }
    return true;
}

}// namespace ToolkitUtil