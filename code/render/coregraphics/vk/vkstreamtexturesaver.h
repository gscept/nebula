#pragma once
//------------------------------------------------------------------------------
/**
	Implements a method to save a texture from Vulkan to stream.
	
	(C) 2016-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "coregraphics/streamtexturesaver.h"
#include "io/stream.h"
#include "coregraphics/texture.h"
#include <IL/il.h>

namespace Vulkan
{

class VkStreamTextureSaver
{
private:
	friend bool CoreGraphics::SaveTexture(const Resources::ResourceId& id, const Ptr<IO::Stream>& stream, IndexT mip, CoreGraphics::ImageFileFormat::Code code);

	/// saves a standard 2D texture
	static bool SaveTexture2D(CoreGraphics::TextureId tex, const Ptr<IO::Stream>& stream, IndexT mip, ILenum imageFileType, CoreGraphics::ImageFileFormat::Code code);
	/// saves a cube map
	static bool SaveCubemap(CoreGraphics::TextureId tex, const Ptr<IO::Stream>& stream, IndexT mip, ILenum imageFileType, CoreGraphics::ImageFileFormat::Code code);
	/// saves a 3D texture
	static bool SaveTexture3D(CoreGraphics::TextureId tex, const Ptr<IO::Stream>& stream, IndexT mip, ILenum imageFileType, CoreGraphics::ImageFileFormat::Code code);

	/// helper function to flip image data horizontally
	template<typename T> void* FlipImageDataHorizontal(SizeT width, SizeT height, void* buf);
	/// helper function to flip image data vertically
	template<typename T> void* FlipImageDataVertical(SizeT width, SizeT height, void* buf);

	/// helper function to flip image data vertically based on block size
	static void* FlipImageDataVerticalBlockWise(SizeT width, SizeT height, SizeT pixelSize, void* buf);
	/// helper function to flip image data horizontally based on block size
	static void* FlipImageDataHorizontalBlockWise(SizeT width, SizeT height, SizeT pixelSize, void* buf);
};

	//------------------------------------------------------------------------------
/**
*/
template<typename T>
inline void*
VkStreamTextureSaver::FlipImageDataHorizontal(SizeT width, SizeT height, void* buf)
{
	T* buffer = reinterpret_cast<T*>(buf);
	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width / 2; x++)
		{
			IndexT coord = x + y * width;
			IndexT nextCoord = (width - 1 - x) + y * width;
			T temp = buffer[coord];
			buffer[coord] = buffer[nextCoord];
			buffer[nextCoord] = temp;
		}
	}
	return reinterpret_cast<void*>(buffer);
}

//------------------------------------------------------------------------------
/**
*/
template<typename T>
inline void*
VkStreamTextureSaver::FlipImageDataVertical(SizeT width, SizeT height, void* buf)
{
	T* buffer = reinterpret_cast<T*>(buf);
	for (int x = 0; x < width; x++)
	{
		for (int y = 0; y < height / 2; y++)
		{
			IndexT coord = x + y * width;
			IndexT nextCoord = x + (height - 1 - y) * width;
			T temp = buffer[coord];
			buffer[coord] = buffer[nextCoord];
			buffer[nextCoord] = temp;
		}
	}
	return reinterpret_cast<void*>(buffer);
}

//------------------------------------------------------------------------------
/**
*/
inline void*
VkStreamTextureSaver::FlipImageDataVerticalBlockWise(SizeT width, SizeT height, SizeT blockSize, void* buf)
{
	char* buffer = static_cast<char*>(buf);
	char* temp = n_new_array(char, blockSize);
	for (int x = 0; x < width; x++)
	{
		for (int y = 0; y < height / 2; y++)
		{
			IndexT coord = (x + y * width) * blockSize;
			IndexT nextCoord = (x + (height - 1 - y) * width) * blockSize;
			memcpy(temp, (void*)(&buffer[coord]), blockSize);
			memcpy((void*)(&buffer[coord]), (void*)(&buffer[nextCoord]), blockSize);
			memcpy((void*)(&buffer[nextCoord]), temp, blockSize);
		}
	}
	n_delete_array(temp);
	return static_cast<void*>(buffer);
}

//------------------------------------------------------------------------------
/**
*/
inline void*
VkStreamTextureSaver::FlipImageDataHorizontalBlockWise(SizeT width, SizeT height, SizeT blockSize, void* buf)
{
	char* buffer = static_cast<char*>(buf);
	char* temp = n_new_array(char, blockSize);
	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width / 2; x++)
		{
			IndexT coord = (x + y * width) * blockSize;
			IndexT nextCoord = ((width - 1 - x) + y * width) * blockSize;
			memcpy(temp, (void*)(&buffer[coord]), blockSize);
			memcpy((void*)(&buffer[coord]), (void*)(&buffer[nextCoord]), blockSize);
			memcpy((void*)(&buffer[nextCoord]), temp, blockSize);
		}
	}
	n_delete_array(temp);
	return static_cast<void*>(buffer);
}

} // namespace Vulkan