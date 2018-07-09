//------------------------------------------------------------------------------
//  texture.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "config.h"
#include "coregraphics/texture.h"
#include "coregraphics/memorytexturepool.h"

namespace CoreGraphics
{

MemoryTexturePool* texturePool = nullptr;

//------------------------------------------------------------------------------
/**
*/
const TextureId
CreateTexture(const TextureCreateInfo& info)
{
	TextureId id = texturePool->ReserveResource(info.name, info.tag);
	n_assert(id.allocType == TextureIdType);
	texturePool->LoadFromMemory(id, &info);
	return id;
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyTexture(const TextureId id)
{
	texturePool->DiscardResource(id);
}

//------------------------------------------------------------------------------
/**
*/
TextureDimensions
TextureGetDimensions(const TextureId id)
{
	return texturePool->GetDimensions(id);
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::PixelFormat::Code
TextureGetPixelFormat(const TextureId id)
{
	return texturePool->GetPixelFormat(id);
}

//------------------------------------------------------------------------------
/**
*/
TextureType
TextureGetType(const TextureId id)
{
	return texturePool->GetType(id);
}

//------------------------------------------------------------------------------
/**
*/
ImageLayout
TextureGetLayout(const TextureId id)
{
	return texturePool->GetLayout(id);
}

//------------------------------------------------------------------------------
/**
*/
uint
TextureGetNumMips(const TextureId id)
{
	return texturePool->GetNumMips(id);
}

//------------------------------------------------------------------------------
/**
*/
TextureMapInfo 
TextureMap(const TextureId id, IndexT mip, const CoreGraphics::GpuBufferTypes::MapType type)
{
	TextureMapInfo info;
	n_assert(texturePool->Map(id, mip, type, info));
	return info;
}

//------------------------------------------------------------------------------
/**
*/
void
TextureUnmap(const TextureId id, IndexT mip)
{
	texturePool->Unmap(id, mip);
}

//------------------------------------------------------------------------------
/**
*/
TextureMapInfo
TextureMapFace(const TextureId id, IndexT mip, TextureCubeFace face, const CoreGraphics::GpuBufferTypes::MapType type)
{
	TextureMapInfo info;
	n_assert(texturePool->MapCubeFace(id, face, mip, type, info));
	return info;
}

//------------------------------------------------------------------------------
/**
*/
void
TextureUnmapFace(const TextureId id, IndexT mip, TextureCubeFace face)
{
	texturePool->UnmapCubeFace(id, face, mip);
}

} // namespace CoreGraphics