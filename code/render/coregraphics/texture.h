#pragma once
//------------------------------------------------------------------------------
/**
	Texture related functions

	(C)2017-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "coregraphics/gpubuffertypes.h"
#include "coregraphics/pixelformat.h"
#include "coregraphics/config.h"
#include "coregraphics/window.h"

namespace CoreGraphics
{

RESOURCE_ID_TYPE(TextureId);

/// texture types
enum TextureType
{
	InvalidTextureType,

	Texture1D,		//> a 1-dimensional texture
	Texture2D,      //> a 2-dimensional texture
	Texture3D,      //> a 3-dimensional texture
	TextureCube,    //> a cube texture

	Texture1DArray,		//> a 1-dimensional texture array, depth represents array size
	Texture2DArray,		//> a 2-dimensional texture array, depth represents array size
	TextureCubeArray,	//> a cube texture array, depth represents array size * 6
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
	InvalidUsage	= 0x0,		// invalid usage
	ImmutableUsage	= 0x1,		// texture is a shader sampleable 1D, 2D, 3D or Cube texture, and is fed by a texture resource
	RenderUsage		= 0x2,		// texture supports to be rendered to as an attachment
	ReadWriteUsage	= 0x4,		// texture supports to be bound as an RWTexture (DX) or Image (GL/Vulkan)
	MapableUsage	= 0x8,		// texture supports memory mapping
	CopyUsage		= 0x10		// texture supports being a copy source and destination
};
__ImplementEnumBitOperators(CoreGraphics::TextureUsage);

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

static const ubyte TextureAutoMips = 0xFF;

struct TextureCreateInfo
{
	TextureCreateInfo()
		: name(""_atm)
		, usage(CoreGraphics::TextureUsage::ImmutableUsage)
		, tag(""_atm)
		, buffer(nullptr)
		, type(Texture2D)
		, format(CoreGraphics::PixelFormat::RGBA8)
		, width(1)
		, height(1)
		, depth(1)
		, mips(1)
		, layers(1)
		, samples(1)
		, windowRelative(false)
		, windowTexture(false)
		, bindless(true)
		, alias(CoreGraphics::TextureId::Invalid())
		, defaultLayout(CoreGraphicsImageLayout::ShaderRead)
	{};

	Resources::ResourceName name;
	CoreGraphics::TextureUsage usage;
	Util::StringAtom tag;
	const void* buffer;
	CoreGraphics::TextureType type;
	CoreGraphics::PixelFormat::Code format;
	float width, height, depth;
	SizeT mips, layers;
	SizeT samples;
	bool windowRelative : 1;					// size is a window relative percentage if true, other wise size is an absolute size
	bool windowTexture : 1;						// texture is supposed to be a backbuffer target
	bool bindless : 1;
	CoreGraphics::TextureId alias;
	CoreGraphicsImageLayout defaultLayout;
};

struct TextureCreateInfoAdjusted
{
	Resources::ResourceName name;
	CoreGraphics::TextureUsage usage;
	Util::StringAtom tag;
	const void* buffer;
	CoreGraphics::TextureType type;
	CoreGraphics::PixelFormat::Code format;
	SizeT width, height, depth;
	float widthScale, heightScale, depthScale;
	SizeT mips, layers;
	SizeT samples;
	bool windowTexture : 1;						// texture is meant to be a window back buffer
	bool windowRelative : 1;					// size is a window relative percentage if true, other wise size is an absolute size
	bool bindless : 1;
	CoreGraphics::WindowId window;
	CoreGraphics::TextureId alias;
	CoreGraphicsImageLayout defaultLayout;
};

class MemoryTexturePool;
extern MemoryTexturePool* texturePool;

/// create new vertex buffer with intended usage, access and CPU syncing parameters, together with size of buffer
const TextureId CreateTexture(const TextureCreateInfo& info);
/// destroy vertex buffer
void DestroyTexture(const TextureId id);

/// get texture dimensions
TextureDimensions TextureGetDimensions(const TextureId id);
/// get texture pixel format
CoreGraphics::PixelFormat::Code TextureGetPixelFormat(const TextureId id);
/// get texture type
TextureType TextureGetType(const TextureId id);
/// get number of mips
SizeT TextureGetNumMips(const TextureId id);
/// get number of layers
SizeT TextureGetNumLayers(const TextureId id);
/// get sample count
SizeT TextureGetNumSamples(const TextureId id);
/// get texture alias, returns invalid id if not aliased
const CoreGraphics::TextureId TextureGetAlias(const TextureId id);
/// get texture usage
const CoreGraphics::TextureUsage TextureGetUsage(const TextureId id);
/// get default texture layout
const CoreGraphicsImageLayout TextureGetDefaultLayout(const TextureId id);

/// get bindless texture handle
uint TextureGetBindlessHandle(const TextureId id);

/// swap backbuffers for texture if texture is a backbuffer
IndexT TextureSwapBuffers(const TextureId id);
/// handle window resizing
void TextureWindowResized(const TextureId id);

/// map GPU memory
TextureMapInfo TextureMap(const TextureId id, IndexT mip, const CoreGraphics::GpuBufferTypes::MapType type);
/// unmap GPU memory
void TextureUnmap(const TextureId id, IndexT mip);
/// map texture face GPU memory
TextureMapInfo TextureMapFace(const TextureId id, IndexT mip, TextureCubeFace face, const CoreGraphics::GpuBufferTypes::MapType type);
/// unmap texture face
void TextureUnmapFace(const TextureId id, IndexT mip, TextureCubeFace face);
/// generate mipmaps for texture
void TextureGenerateMipmaps(const TextureId id);

/// helper function to setup RenderTextureInfo, already implemented
TextureCreateInfoAdjusted TextureGetAdjustedInfo(const TextureCreateInfo& info);

//------------------------------------------------------------------------------
/**
*/
inline TextureType
TextureTypeFromString(const Util::String& string)
{
	if		(string == "Texture1D") return Texture1D;
	else if (string == "Texture2D") return Texture2D;
	else if (string == "Texture3D") return Texture3D;
	else if (string == "TextureCube") return TextureCube;
	else if (string == "Texture1DArray") return Texture1DArray;
	else if (string == "Texture2DArray") return Texture2DArray;
	else if (string == "TextureCubeArray") return TextureCubeArray;
	else
	{
		n_error("Unknown texture type '%s'", string.AsCharPtr());
		return Texture1D;
	}
}

//------------------------------------------------------------------------------
/**
*/
inline TextureUsage
TextureUsageFromString(const Util::String& string)
{
	Util::Array<Util::String> comps = string.Tokenize("|");
	TextureUsage usage = TextureUsage(0x0);

	for (IndexT i = 0; i < comps.Size(); i++)
	{
		if		(comps[i] == "Immutable") usage |= ImmutableUsage;
		else if (comps[i] == "Render") usage |= RenderUsage;
		else if (comps[i] == "ReadWrite") usage |= ReadWriteUsage;
		else if (comps[i] == "Mapable") usage |= MapableUsage;
		else if (comps[i] == "Copy") usage |= CopyUsage;
	}

	return usage;
}

extern TextureId White1D;
extern TextureId Black2D;
extern TextureId White2D;
extern TextureId WhiteCube;
extern TextureId White3D;
extern TextureId White1DArray;
extern TextureId White2DArray;
extern TextureId WhiteCubeArray;

} // CoreGraphics
