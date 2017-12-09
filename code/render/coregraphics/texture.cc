//------------------------------------------------------------------------------
//  texture.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/texture.h"
#include "coregraphics/memorytexturepool.h"

namespace CoreGraphics
{

MemoryTexturePool* texturePool = nullptr;

//------------------------------------------------------------------------------
/*
*/
inline const TextureId
CreateTexture(TextureCreateInfo info)
{
	Resources::ResourceId id = texturePool->ReserveResource(info.name, info.tag);
	TextureId ret = id;
	texturePool->LoadFromMemory(ret.id24, &info);
	return ret;
}

//------------------------------------------------------------------------------
/*
*/
inline void
DestroyTexture(const TextureId id)
{
	texturePool->DiscardResource(id.id24);
}

//------------------------------------------------------------------------------
/*
*/
inline TextureMapInfo 
MapTexture(const TextureId id, IndexT mip, const CoreGraphics::GpuBufferTypes::MapType type)
{
	TextureMapInfo info;
	n_assert(texturePool->Map(id, mip, type, info));
	return info;
}

//------------------------------------------------------------------------------
/*
*/
inline void
UnmapTexture(const TextureId id, IndexT mip)
{
	texturePool->Unmap(id, mip);
}

}