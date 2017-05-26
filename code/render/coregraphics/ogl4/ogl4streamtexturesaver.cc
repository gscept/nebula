//------------------------------------------------------------------------------
//  ogl4streamtexturesaver.cc
//  (C) 2012-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/ogl4/ogl4streamtexturesaver.h"
#include "coregraphics/ogl4/ogl4types.h"
#include "coregraphics/renderdevice.h"
#include "coregraphics/base/texturebase.h"
#include <IL/il.h>
#include <IL/ilu.h>

#ifdef RGB
#undef RGB
#endif
#define RGB(r,g,b) (uint)( ((255 & 0xff) << 24) |((b & 0xff) << 16) | ((g & 0xff) << 8) | ((r & 0xff)) )
#define RGBA(r,g,b,a) (uint)( ((a & 0xff) << 24) | ((b & 0xff) << 16) | ((g & 0xff) << 8) | ((r & 0xff)) )

#define RED(rgb) (rgb & 0xff)
#define BLUE(rgb) ((rgb >> 16) & 0xff)
#define GREEN(rgb) ((rgb >> 8) & 0xff)
#define ALPHA(rgb) (rgb >> 24)

namespace OpenGL4
{
__ImplementClass(OpenGL4::OGL4StreamTextureSaver, 'D1TS', Base::StreamTextureSaverBase);

using namespace Base;
using namespace IO;
using namespace CoreGraphics;

//------------------------------------------------------------------------------
/**
*/
bool
OGL4StreamTextureSaver::OnSave()
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
OGL4StreamTextureSaver::SaveTexture2D(const Ptr<CoreGraphics::Texture>& tex, uint imageFileType)
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
	channels = PixelFormat::ToChannels(tex->GetPixelFormat());
	format = PixelFormat::ToILComponents(tex->GetPixelFormat());
	type = PixelFormat::ToILType(tex->GetPixelFormat());

	Texture::MapInfo mapInfo;
	tex->Map(mipLevelToSave, ResourceBase::MapRead, mapInfo);

	// create image
	ILboolean result = ilTexImage(mapInfo.mipWidth, mapInfo.mipHeight, 1, channels, format, type, (ILubyte*)mapInfo.data);
	n_assert(result == IL_TRUE);

	// flip image if it's a GL texture
	if (!tex->IsRenderTargetAttachment())
	{
		iluFlipImage();
	}

	// now save as PNG (will support proper alpha)
	ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);
	ILint size = ilSaveL(imageFileType, NULL, 0);
	ILbyte* data = n_new_array(ILbyte, size);
	ilSaveL(imageFileType, data, size);
	tex->Unmap(mipLevelToSave);

	// write result to stream
	this->stream->SetAccessMode(Stream::WriteAccess);
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
OGL4StreamTextureSaver::SaveCubemap(const Ptr<CoreGraphics::Texture>& tex, uint imageFileType)
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

	// calculate size
	GLenum components = OGL4Types::AsOGL4PixelComponents(tex->GetPixelFormat());
	SizeT pixelSize = PixelFormat::ToSize(tex->GetPixelFormat());

	// get dimensions of mip
	GLint mippedWidth;
	GLint mippedHeight;
	glBindTexture(GL_TEXTURE_CUBE_MAP, tex->GetOGL4Texture());
	glGetTexLevelParameteriv(GL_TEXTURE_CUBE_MAP_POSITIVE_X, mipLevelToSave, GL_TEXTURE_WIDTH, &mippedWidth);
	glGetTexLevelParameteriv(GL_TEXTURE_CUBE_MAP_POSITIVE_X, mipLevelToSave, GL_TEXTURE_HEIGHT, &mippedHeight);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

	// calculate mipped height and width for texture cube, width * 4 and height * 3
	GLint totalWidth = mippedWidth * 3;
	GLint totalHeight = mippedHeight * 4;

	// calculate channels and format
	ILuint channels;
	ILuint format;
	ILuint type;
	channels = PixelFormat::ToChannels(tex->GetPixelFormat());
	format = PixelFormat::ToILComponents(tex->GetPixelFormat());
	type = PixelFormat::ToILType(tex->GetPixelFormat());

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
		if		(cubeFace == Texture::PosX)	{ xOffset = 0;					yOffset = mippedHeight * 2; }
		else if (cubeFace == Texture::NegX) { xOffset = mippedHeight * 2;	yOffset = mippedHeight * 2; }
		else if (cubeFace == Texture::PosZ)	{ xOffset = mippedWidth;		yOffset = 0; }
		else if (cubeFace == Texture::NegZ) { xOffset = mippedWidth;		yOffset = mippedHeight * 2; }
		else if (cubeFace == Texture::PosY) { xOffset = mippedWidth;		yOffset = mippedHeight * 3; }
		else if (cubeFace == Texture::NegY) { xOffset = mippedWidth;		yOffset = mippedWidth * 1; }

		Texture::MapInfo mapInfo;
		tex->MapCubeFace((Texture::CubeFace)cubeFace, mipLevelToSave, Texture::MapRead, mapInfo);

		// flip image if necessary
		if (cubeFace == Texture::NegZ || cubeFace == Texture::PosX || cubeFace == Texture::NegX)
		{
			mapInfo.data = this->FlipImageDataVerticalBlockWise(mippedWidth, mippedHeight, pixelSize, mapInfo.data);
		}
		else if (cubeFace == Texture::PosZ || cubeFace == Texture::PosY)
		{
			mapInfo.data = this->FlipImageDataHorizontalBlockWise(mippedWidth, mippedHeight, pixelSize, mapInfo.data);
		}

		// set pixels
		ilSetPixels(xOffset, yOffset, 0, mippedWidth, mippedHeight, 0, format, type, (ILubyte*)mapInfo.data);
			
		// unmap
		tex->UnmapCubeFace((Texture::CubeFace)cubeFace, mipLevelToSave);
	}
	
	// now save as PNG (will give us proper alpha)
	ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);
	ILint size = ilSaveL(imageFileType, NULL, 0);
	ILbyte* data = n_new_array(ILbyte, size);
	ilSaveL(imageFileType, data, size);

	// write result to stream
	this->stream->SetAccessMode(Stream::WriteAccess);
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
	Implement if required.
*/
bool
OGL4StreamTextureSaver::SaveTexture3D(const Ptr<CoreGraphics::Texture>& tex, uint imageFileType)
{
	return false;
}




} // namespace OpenGL4

