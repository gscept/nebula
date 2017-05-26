#pragma once
//------------------------------------------------------------------------------
/**
    @class OpenGL4::OGL4StreamTextureSaver
    
    OGL4 implementation of StreamTextureSaver.
    
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "coregraphics/base/streamtexturesaverbase.h"
#include "coregraphics/texture.h"

//------------------------------------------------------------------------------
namespace OpenGL4
{
class OGL4StreamTextureSaver : public Base::StreamTextureSaverBase
{
    __DeclareClass(OGL4StreamTextureSaver);

public:
    /// called by resource when a save is requested
    virtual bool OnSave();

private:
	/// saves a standard 2D texture
	bool SaveTexture2D(const Ptr<CoreGraphics::Texture>& tex, uint imageFileType);
	/// saves a cube map
	bool SaveCubemap(const Ptr<CoreGraphics::Texture>& tex, uint imageFileType);
	/// saves a 3D texture
	bool SaveTexture3D(const Ptr<CoreGraphics::Texture>& tex, uint imageFileType);

	/// helper function to flip image data horizontally
	template<typename T> void* FlipImageDataHorizontal(SizeT width, SizeT height, void* buf);
	/// helper function to flip image data vertically
	template<typename T> void* FlipImageDataVertical(SizeT width, SizeT height, void* buf);

	/// helper function to flip image data vertically based on block size
	void* FlipImageDataVerticalBlockWise(SizeT width, SizeT height, SizeT pixelSize, void* buf);
	/// helper function to flip image data horizontally based on block size
	void* FlipImageDataHorizontalBlockWise(SizeT width, SizeT height, SizeT pixelSize, void* buf);
};

//------------------------------------------------------------------------------
/**
*/
template<typename T>
inline void*
OGL4StreamTextureSaver::FlipImageDataHorizontal(SizeT width, SizeT height, void* buf)
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
OGL4StreamTextureSaver::FlipImageDataVertical(SizeT width, SizeT height, void* buf)
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
OGL4StreamTextureSaver::FlipImageDataVerticalBlockWise(SizeT width, SizeT height, SizeT blockSize, void* buf)
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
OGL4StreamTextureSaver::FlipImageDataHorizontalBlockWise(SizeT width, SizeT height, SizeT blockSize, void* buf)
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

} // namespace OpenGL4
//------------------------------------------------------------------------------
    