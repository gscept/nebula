#pragma once
//------------------------------------------------------------------------------
/**
	Texture related functions

	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "coregraphics/gpubuffertypes.h"
#include "coregraphics/pixelformat.h"

namespace CoreGraphics
{

ID_32_24_8_TYPE(TextureId);

/// texture types
enum TextureType
{
	InvalidType,

	Texture1D,		//> a 1-dimensional texture
	Texture2D,      //> a 2-dimensional texture
	Texture3D,      //> a 3-dimensional texture
	TextureCube,    //> a cube texture
	TextureBuffer,	//> a texture buffer (1d, no mips)

	Texture1DArray,		//> a 1-dimensional texture array, depth represents array size
	Texture2DArray,		//> a 2-dimensional texture array, depth represents array size
	Texture3DArray,		//> a 3-dimensional texture array, depth represents array size * size of layers in array
	TextureCubeArray,	//> a cube texture array, depth represents array size * 6
	TextureBufferArray,	//> a texture buffer (1d, no mips)
};

/// cube map face
enum TextureCubeFace
{
	PosX = 0,
	NegX,
	PosY,
	NegY,
	PosZ,
	NegZ,
};

/// type of texture usage
enum TextureUsage
{
	Ordinary,		// texture is a shader sampleable 1D, 2D, 3D or Cube texture
	ReadWrite,		// texture is an RWTexture (DX) or Image (GL/Vulkan)
	Buffer			// texture is a Texture Buffer type
};

/// access info filled by Map methods
struct TextureMapInfo
{
	/// constructor
	TextureMapInfo() : data(0), rowPitch(0), depthPitch(0) {};

	void* data;
	SizeT rowPitch;
	SizeT depthPitch;
	SizeT mipWidth;
	SizeT mipHeight;
};

struct TextureDimensions
{
	SizeT width, height, depth;
};

struct TextureCreateInfo
{
	Resources::ResourceName name;
	Util::StringAtom tag;
	const void* buffer;
	CoreGraphics::PixelFormat::Code format;
	SizeT width, height, depth;
};

class MemoryTexturePool;
extern MemoryTexturePool* texturePool;

/// create new vertex buffer with intended usage, access and CPU syncing parameters, together with size of buffer
const TextureId CreateTexture(TextureCreateInfo info);
/// destroy vertex buffer
void DestroyTexture(const TextureId id);

/// get texture dimensions
TextureDimensions TextureGetDimensions(const TextureId id);
/// get texture pixel format
CoreGraphics::PixelFormat::Code TextureGetPixelFormat(const TextureId id);
/// get texture type
TextureType TextureGetType(const TextureId id);
/// get number of mips
uint TextureGetNumMips(const TextureId id);

/// map GPU memory
TextureMapInfo TextureMap(const TextureId id, IndexT mip, const CoreGraphics::GpuBufferTypes::MapType type);
/// unmap GPU memory
void TextureUnmap(const TextureId id, IndexT mip);

} // CoreGraphics
