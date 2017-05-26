//------------------------------------------------------------------------------
// vkstreamtexturesaver.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "vkstreamtexturesaver.h"
#include "coregraphics/texture.h"
#include "coregraphics/renderdevice.h"
#include <IL/il.h>
#include <IL/ilu.h>
#include "vktypes.h"

using namespace CoreGraphics;
namespace Vulkan
{
__ImplementClass(Vulkan::VkStreamTextureSaver, 'VKTS', Base::StreamTextureSaverBase);
//------------------------------------------------------------------------------
/**
*/
VkStreamTextureSaver::VkStreamTextureSaver()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
VkStreamTextureSaver::~VkStreamTextureSaver()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
bool
VkStreamTextureSaver::OnSave()
{
	n_assert(this->stream.isvalid());
	const Ptr<Texture>& tex = this->resource.downcast<Texture>();

	// solve result format depending on format
	ILenum imageFormat;
	if (this->format == ImageFileFormat::DDS) imageFormat = IL_DDS;
	else if (this->format == ImageFileFormat::JPG) imageFormat = IL_JPG;
	else if (this->format == ImageFileFormat::PNG) imageFormat = IL_PNG;
	else if (this->format == ImageFileFormat::TGA) imageFormat = IL_TGA;
	else if (this->format == ImageFileFormat::BMP) imageFormat = IL_BMP;
	else return false;

	// treat texture
	if (tex->GetType() == Texture::Texture2D)			return this->SaveTexture2D(tex, imageFormat);
	else if (tex->GetType() == Texture::Texture3D)		return this->SaveTexture3D(tex, imageFormat);
	else if (tex->GetType() == Texture::TextureCube)	return this->SaveCubemap(tex, imageFormat);
	else
	{
		n_error("OGL4StreamTextureSaver::OnSave() : Unknown texture type!");
		return false;
	}
}

//------------------------------------------------------------------------------
/**
*/
bool
VkStreamTextureSaver::SaveTexture2D(const Ptr<CoreGraphics::Texture>& tex, ILenum imageFileType)
{
	n_assert(tex->GetType() == Texture::Texture2D);
	bool retval = false;

	SizeT maxLevels = tex->GetNumMipLevels();
	SizeT mipLevelToSave = this->mipLevel;
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
	VkFormat fmt = VkTypes::AsVkFormat(tex->GetPixelFormat());
	PixelFormat::Code pfmt = VkTypes::AsNebulaPixelFormat(fmt);
	channels = PixelFormat::ToChannels(pfmt);
	format = PixelFormat::ToILComponents(pfmt);
	type = PixelFormat::ToILType(pfmt);

	Texture::MapInfo mapInfo;
	tex->Map(mipLevelToSave, Texture::MapRead, mapInfo);

	// create image
	ILboolean result;
	
	if (VkTypes::IsCompressedFormat(fmt))
	{
		result = ilTexImageDxtc(mapInfo.mipWidth, mapInfo.mipHeight, 1, VkTypes::AsILDXTFormat(fmt), (ILubyte*)mapInfo.data);

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
	tex->Unmap(mipLevelToSave);

	// write result to stream
	this->stream->SetAccessMode(IO::Stream::WriteAccess);
	if (this->stream->Open())
	{
		// write raw pointer to stream
		this->stream->Write(data, size);

		this->stream->Close();
		this->stream->SetMediaType(ImageFileFormat::ToMediaType(this->format));

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
VkStreamTextureSaver::SaveCubemap(const Ptr<CoreGraphics::Texture>& tex, ILenum imageFileType)
{
	n_assert(tex->GetType() == Texture::TextureCube);
	bool retval = false;
	bool isCompressed = true;

	SizeT maxLevels = tex->GetNumMipLevels();
	SizeT mipLevelToSave = this->mipLevel;
	if (mipLevelToSave >= maxLevels)
	{
		mipLevelToSave = maxLevels - 1;
	}

	// calculate channels and format
	ILuint channels;
	ILuint format;
	ILuint type;
	VkFormat fmt = VkTypes::AsVkMappableImageFormat(VkTypes::AsVkFormat(tex->GetPixelFormat()));
	PixelFormat::Code pfmt = VkTypes::AsNebulaPixelFormat(fmt);
	channels = PixelFormat::ToChannels(pfmt);
	format = PixelFormat::ToILComponents(pfmt);
	type = PixelFormat::ToILType(pfmt);
	uint32_t pixelSize = PixelFormat::ToSize(pfmt);

	int32_t mipWidth = (int32_t)Math::n_max(1.0f, Math::n_floor(tex->GetWidth() / Math::n_pow(2, (float)mipLevel)));
	int32_t mipHeight = (int32_t)Math::n_max(1.0f, Math::n_floor(tex->GetHeight() / Math::n_pow(2, (float)mipLevel)));
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
		if (cubeFace == Texture::PosX)	{ xOffset = 0;					yOffset = mipHeight * 2; }
		else if (cubeFace == Texture::NegX) { xOffset = mipHeight * 2;	yOffset = mipHeight * 2; }
		else if (cubeFace == Texture::PosZ)	{ xOffset = mipWidth;		yOffset = 0; }
		else if (cubeFace == Texture::NegZ) { xOffset = mipWidth;		yOffset = mipHeight * 2; }
		else if (cubeFace == Texture::PosY) { xOffset = mipWidth;		yOffset = mipHeight * 3; }
		else if (cubeFace == Texture::NegY) { xOffset = mipWidth;		yOffset = mipWidth * 1; }

		Texture::MapInfo mapInfo;
		tex->MapCubeFace((Texture::CubeFace)cubeFace, mipLevelToSave, Texture::MapRead, mapInfo);

		// flip image if necessary
		if (cubeFace == Texture::NegZ || cubeFace == Texture::PosX || cubeFace == Texture::NegX)
		{
			mapInfo.data = this->FlipImageDataVerticalBlockWise(mipWidth, mipHeight, pixelSize, mapInfo.data);
		}
		else if (cubeFace == Texture::PosZ || cubeFace == Texture::PosY)
		{
			mapInfo.data = this->FlipImageDataHorizontalBlockWise(mipWidth, mipHeight, pixelSize, mapInfo.data);
		}

		// set pixels
		ilSetPixels(xOffset, yOffset, 0, mipWidth, mipHeight, 0, format, type, (ILubyte*)mapInfo.data);

		// unmap
		tex->UnmapCubeFace((Texture::CubeFace)cubeFace, mipLevelToSave);
	}

	// now save as PNG (will give us proper alpha)
	ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);
	ILint size = ilSaveL(imageFileType, NULL, 0);
	ILbyte* data = n_new_array(ILbyte, size);
	ilSaveL(imageFileType, data, size);

	// write result to stream
	this->stream->SetAccessMode(IO::Stream::WriteAccess);
	if (this->stream->Open())
	{
		// write raw pointer to stream
		this->stream->Write(data, size);

		this->stream->Close();
		this->stream->SetMediaType(ImageFileFormat::ToMediaType(this->format));

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
VkStreamTextureSaver::SaveTexture3D(const Ptr<CoreGraphics::Texture>& tex, ILenum imageFileType)
{
	return false;
}

} // namespace Vulkan