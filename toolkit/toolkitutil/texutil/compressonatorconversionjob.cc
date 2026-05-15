//------------------------------------------------------------------------------
//  compressonatorconversionjob.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"

#include "compressonatorconversionjob.h"
#include "io/uri.h"
#include "io/assignregistry.h"
#include "io/ioserver.h"
#include "timing/timer.h"

#include "Compressonator.h"

extern void (*PrintStatusLine)(char*);

namespace ToolkitUtil
{
using namespace IO;
using namespace Util;

void Printl(char* buf)
{
    fprintf(stderr,"%s\n", buf);
}

//------------------------------------------------------------------------------
/**
*/
CompressonatorConversionJob::CompressonatorConversionJob()
{
    this->SetDstFileExtension("dds");
    auto printline = [=](char*buff)->void { this->logger->Error(buff); };
    PrintStatusLine = &Printl;
}

//------------------------------------------------------------------------------
/**
*/
static CMP_FORMAT
GetTexConvFormat(ToolkitUtil::TextureResourceT const* texture)
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

//------------------------------------------------------------------------------
/**
    Start the conversion process. Returns false, if the job finished immediately.   
*/
bool
CompressonatorConversionJob::Convert(ToolkitUtil::TextureResourceT* texture)
{
    n_assert(0 != this->logger);
    if (TextureConversionJob::Convert())
    {
        URI srcPathUri(this->srcPath);
        URI dstPathUri(this->dstPath);
        URI tmpDirUri(this->tmpDir);

        String src = srcPathUri.LocalPath();

        Timing::Timer timer;
        timer.Start();

        this->logger->Print("Processing: %s\n", src.AsCharPtr());

        CMP_MipSet MipSetIn;
        Memory::Clear(&MipSetIn, sizeof(CMP_MipSet));

        auto cmp_status = CMP_LoadTexture(src.AsCharPtr(), &MipSetIn);

        if (cmp_status != CMP_OK)
        {
            this->logger->Error("Failed to load %s, error code: %d\n", src.AsCharPtr(), cmp_status);
            return false;
        }


        if (MipSetIn.m_format != CMP_FORMAT_RGBA_8888)
        {
            this->logger->Print("Non-rgb(a) texture, saving raw\n");

            cmp_status = CMP_SaveTexture(dstPathUri.LocalPath().AsCharPtr(), &MipSetIn);
            if (cmp_status != CMP_OK)
            {
                this->logger->Error("Failed to load %s, CMP_SaveTexture, error code: %d\n", src.AsCharPtr(), cmp_status);
                return false;
            }

            CMP_FreeMipSet(&MipSetIn);

            Timing::Time after = timer.GetTime();
            this->logger->Print("Done after: %f\n", after);
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
        switch (texture->compression_quality)
        {
        case ToolkitUtil::TextureCompressionQuality_High:
            quality = 0.7f;
            break;
        case ToolkitUtil::TextureCompressionQuality_Low:
            quality = 0.1f;
            break;
        }

        kernel_options.format = GetTexConvFormat(texture);
        kernel_options.useSRGBFrames = (texture->color_space == ToolkitUtil::TextureColorSpace::TextureColorSpace_sRGB) ? true : false;

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

        CMP_Feedback_Proc CompressionCallback = [](CMP_FLOAT fProgress, CMP_DWORD_PTR pUser1, CMP_DWORD_PTR pUser2)->bool {
            return 0;
        };

        //===============================================
        // Compress the texture using Framework Lib
        //===============================================
        cmp_status = CMP_ProcessTexture(&MipSetIn, &MipSetCmp, kernel_options, CompressionCallback);
        if (cmp_status != CMP_OK)
        {
            this->logger->Error("Failed to load %s, CMP_ProcessTexture, error code: %d\n", src.AsCharPtr(), cmp_status);
            return false;
        }

        //----------------------------------------------------------------
        // Save the result into a DDS file
        //----------------------------------------------------------------
        cmp_status = CMP_SaveTexture(dstPathUri.LocalPath().AsCharPtr(), &MipSetCmp);
        if (cmp_status != CMP_OK)
        {
            this->logger->Error("Failed to load %s, CMP_SaveTexture, error code: %d\n", src.AsCharPtr(), cmp_status);
            return false;
        }

        CMP_FreeMipSet(&MipSetIn);
        CMP_FreeMipSet(&MipSetCmp);

        Timing::Time after = timer.GetTime();
        this->logger->Print("Done after: %f\n", after);
        return true;
    }
    return false;
}

}// namespace ToolkitUtil
