//------------------------------------------------------------------------------
// vkstreamtexturesaver.cc
// (C) 2016-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "vkstreamtexturesaver.h"
#include "coregraphics/texture.h"
#include "coregraphics/config.h"
#include "vktypes.h"
#include "resources/resourceserver.h"
#include "io/ioserver.h"
#include "coregraphics//streamtexturesaver.h"
#include <IL/il.h>
#include <IL/ilu.h>

using namespace IO;
using namespace CoreGraphics;

namespace CoreGraphics
{

//------------------------------------------------------------------------------
/**
*/
bool
SaveTexture(const Resources::ResourceId& id, const IO::URI& path, IndexT mip, CoreGraphics::ImageFileFormat::Code code)
{
    Ptr<Stream> stream = IoServer::Instance()->CreateStream(path);
    stream->SetAccessMode(Stream::WriteAccess);
    if (stream->Open())
    {
        return SaveTexture(id, stream, mip, code);
    }
    else
    {
        return false;
    }
}

//------------------------------------------------------------------------------
/**
*/
bool SaveTexture(const Resources::ResourceId& id, const Ptr<IO::Stream>& stream, IndexT mip, CoreGraphics::ImageFileFormat::Code code)
{
    n_assert(id.resourceType == TextureIdType);
    TextureId tid = id;
    const TextureType type = TextureGetType(tid);

    // solve result format depending on format
    ILenum imageFormat;
    if (code == ImageFileFormat::DDS) imageFormat = IL_DDS;
    else if (code == ImageFileFormat::JPG) imageFormat = IL_JPG;
    else if (code == ImageFileFormat::PNG) imageFormat = IL_PNG;
    else if (code == ImageFileFormat::TGA) imageFormat = IL_TGA;
    else if (code == ImageFileFormat::BMP) imageFormat = IL_BMP;
    else return false;

    // treat texture
    if (type == Texture2D)          return Vulkan::VkStreamTextureSaver::SaveTexture2D(tid, stream, mip, imageFormat, code);
    else if (type == Texture3D)     return Vulkan::VkStreamTextureSaver::SaveTexture3D(tid, stream, mip, imageFormat, code);
    else if (type == TextureCube)   return Vulkan::VkStreamTextureSaver::SaveCubemap(tid, stream, mip, imageFormat, code);
    else
    {
        n_error("OGL4StreamTextureSaver::OnSave() : Unknown texture type!");
        return false;
    }
}

} // namespace CoreGraphics

namespace Vulkan
{

//------------------------------------------------------------------------------
/**
*/
bool
VkStreamTextureSaver::SaveTexture2D(CoreGraphics::TextureId tex, const Ptr<IO::Stream>& stream, IndexT mip, ILenum imageFileType, CoreGraphics::ImageFileFormat::Code code)
{
    bool retval = false;
    
    SizeT maxLevels = TextureGetNumMips(tex);
    SizeT mipLevelToSave = mip;
    if (mipLevelToSave >= maxLevels)
    {
        mipLevelToSave = maxLevels - 1;
    }

    // create il image
    ILint image = ilGenImage();
    ilBindImage(image);

    // convert our pixel formats to IL components
    ILuint channels;
    ILuint format;
    ILuint type;
    VkFormat fmt = VkTypes::AsVkFormat(TextureGetPixelFormat(tex));
    PixelFormat::Code pfmt = VkTypes::AsNebulaPixelFormat(fmt);
    channels = PixelFormat::ToChannels(pfmt);
    format = PixelFormat::ToILComponents(pfmt);
    type = PixelFormat::ToILType(pfmt);

    TextureMapInfo mapInfo = TextureMap(tex, mipLevelToSave, GpuBufferTypes::MapRead);

    // create image
    ILboolean result;
    
    if (VkTypes::IsCompressedFormat(fmt))
    {
        //result = ilTexImageDxtc(mapInfo.mipWidth, mapInfo.mipHeight, 1, VkTypes::AsILDXTFormat(fmt), (ILubyte*)mapInfo.data);

        // decompress
        ilSetInteger(IL_DXTC_NO_DECOMPRESS, IL_FALSE);
        ilDxtcDataToImage();
    }
    else
    {
        // image is directly mappable to a display-capable format
        result = ilTexImage(mapInfo.mipWidth, mapInfo.mipHeight, 1, channels, format, type, (ILubyte*)mapInfo.data);
    }
    
    n_assert(result == IL_TRUE);

    // now save as PNG (will support proper alpha)
    ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);
    ILint size = ilSaveL(imageFileType, NULL, 0);
    ILbyte* data = n_new_array(ILbyte, size);
    ilSaveL(imageFileType, data, size);
    TextureUnmap(tex, mipLevelToSave);

    // write result to stream
    stream->SetAccessMode(IO::Stream::WriteAccess);
    if (stream->Open())
    {
        // write raw pointer to stream
        stream->Write(data, size);

        stream->Close();
        stream->SetMediaType(ImageFileFormat::ToMediaType(code));

        retval = true;
    }

    n_delete_array(data);
    ilDeleteImage(image);
    return retval;
}

//------------------------------------------------------------------------------
/**
*/
bool
VkStreamTextureSaver::SaveCubemap(CoreGraphics::TextureId tex, const Ptr<IO::Stream>& stream, IndexT mip, ILenum imageFileType, CoreGraphics::ImageFileFormat::Code code)
{
    bool retval = false;
    bool isCompressed = true;

    SizeT maxLevels = TextureGetNumMips(tex);
    SizeT mipLevelToSave = mip;
    if (mipLevelToSave >= maxLevels)
    {
        mipLevelToSave = maxLevels - 1;
    }

    // calculate channels and format
    ILuint channels;
    ILuint format;
    ILuint type;
    VkFormat fmt = VkTypes::AsVkMappableImageFormat(VkTypes::AsVkFormat(TextureGetPixelFormat(tex)));
    PixelFormat::Code pfmt = VkTypes::AsNebulaPixelFormat(fmt);
    channels = PixelFormat::ToChannels(pfmt);
    format = PixelFormat::ToILComponents(pfmt);
    type = PixelFormat::ToILType(pfmt);
    uint32_t pixelSize = PixelFormat::ToSize(pfmt);

    TextureDimensions dims = TextureGetDimensions(tex);
    int32_t mipWidth = (int32_t)Math::max(1.0f, Math::floor(dims.width / Math::pow(2, (float)mip)));
    int32_t mipHeight = (int32_t)Math::max(1.0f, Math::floor(dims.height / Math::pow(2, (float)mip)));
    uint32_t totalWidth = mipWidth * 3;
    uint32_t totalHeight = mipHeight * 4;

    // create il image
    ILint image = ilGenImage();
    ilBindImage(image);
    ILboolean result = ilTexImage(totalWidth, totalHeight, 1, channels, format, type, NULL);
    ilClearImage();
    n_assert(result == IL_TRUE);

    // if we have a DXT image, OpenGL will perform the unpack automatically when mapping the texture
    IndexT cubeFace;
    for (cubeFace = 0; cubeFace < 6; cubeFace++)
    {
        GLint xOffset = 0;
        GLint yOffset = 0;
        if (cubeFace == PosX)   { xOffset = 0;                  yOffset = mipHeight * 2; }
        else if (cubeFace == NegX) { xOffset = mipHeight * 2;   yOffset = mipHeight * 2; }
        else if (cubeFace == PosZ)  { xOffset = mipWidth;       yOffset = 0; }
        else if (cubeFace == NegZ) { xOffset = mipWidth;        yOffset = mipHeight * 2; }
        else if (cubeFace == PosY) { xOffset = mipWidth;        yOffset = mipHeight * 3; }
        else if (cubeFace == NegY) { xOffset = mipWidth;        yOffset = mipWidth * 1; }

        TextureMapInfo mapInfo = TextureMapFace(tex, mipLevelToSave, (TextureCubeFace)cubeFace, GpuBufferTypes::MapRead);

        // flip image if necessary
        if (cubeFace == NegZ || cubeFace == PosX || cubeFace == NegX)
        {
            mapInfo.data = VkStreamTextureSaver::FlipImageDataVerticalBlockWise(mipWidth, mipHeight, pixelSize, mapInfo.data);
        }
        else if (cubeFace == PosZ || cubeFace == PosY)
        {
            mapInfo.data = VkStreamTextureSaver::FlipImageDataHorizontalBlockWise(mipWidth, mipHeight, pixelSize, mapInfo.data);
        }

        // set pixels
        ilSetPixels(xOffset, yOffset, 0, mipWidth, mipHeight, 0, format, type, (ILubyte*)mapInfo.data);

        // unmap
        TextureUnmapFace(tex, mipLevelToSave, (TextureCubeFace)cubeFace);
    }

    // now save as PNG (will give us proper alpha)
    ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);
    ILint size = ilSaveL(imageFileType, NULL, 0);
    ILbyte* data = n_new_array(ILbyte, size);
    ilSaveL(imageFileType, data, size);

    // write result to stream
    stream->SetAccessMode(IO::Stream::WriteAccess);
    if (stream->Open())
    {
        // write raw pointer to stream
        stream->Write(data, size);

        stream->Close();
        stream->SetMediaType(ImageFileFormat::ToMediaType(code));

        retval = true;
    }

    n_delete_array(data);
    ilDeleteImage(image);
    return retval;
}

//------------------------------------------------------------------------------
/**
*/
bool
VkStreamTextureSaver::SaveTexture3D(CoreGraphics::TextureId tex, const Ptr<IO::Stream>& stream, IndexT mip, ILenum imageFileType, CoreGraphics::ImageFileFormat::Code code)
{
    return false;
}

} // namespace Vulkan
