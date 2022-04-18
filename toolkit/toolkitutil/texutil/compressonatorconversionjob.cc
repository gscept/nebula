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


namespace ToolkitUtil
{
using namespace IO;
using namespace Util;

//------------------------------------------------------------------------------
/**
*/
CompressonatorConversionJob::CompressonatorConversionJob()
{
    this->SetDstFileExtension("dds");
}

//------------------------------------------------------------------------------
/**
    Start the conversion process. Returns false, if the job finished immediately.   
*/
bool
CompressonatorConversionJob::Convert()
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
            goto processing_failed;
        }


        if (MipSetIn.m_format != CMP_FORMAT_RGBA_8888)
        {
            this->logger->Print("Non-rgb(a) texture, saving raw\n");

            cmp_status = CMP_SaveTexture(dstPathUri.LocalPath().AsCharPtr(), &MipSetIn);
            if (cmp_status != CMP_OK)
            {
                goto processing_failed;
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

        const TextureAttrs& attrs = this->textureAttrs;
        float quality = 0.5f;
        switch (attrs.GetQuality())
        {
        case ToolkitUtil::TextureAttrs::High:
            quality = 0.7f;
            break;
        case ToolkitUtil::TextureAttrs::Low:
            quality = 0.1f;
            break;
        }
        kernel_options.format = CMP_FORMAT_BC7; // Set the format to process
        kernel_options.fquality = quality;     // Set the quality of the result
        kernel_options.threads = 0;             // Auto setting

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
            goto processing_failed;
        }

        //----------------------------------------------------------------
        // Save the result into a DDS file
        //----------------------------------------------------------------
        cmp_status = CMP_SaveTexture(dstPathUri.LocalPath().AsCharPtr(), &MipSetCmp);


        

        if (cmp_status != CMP_OK)
        {
            goto processing_failed;
        }

        CMP_FreeMipSet(&MipSetIn);
        CMP_FreeMipSet(&MipSetCmp);

        Timing::Time after = timer.GetTime();
        this->logger->Print("Done after: %f\n", after);
        return true;

    processing_failed:

        this->logger->Error("Failed to load %s, error code: %d\n", src.AsCharPtr(), cmp_status);
    }
    return false;
}

}// namespace ToolkitUtil