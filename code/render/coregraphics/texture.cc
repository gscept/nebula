//------------------------------------------------------------------------------
//  texture.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "config.h"
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
	TextureId id = texturePool->ReserveResource(info.name, info.tag);
	id.allocType = TextureIdType;
	texturePool->LoadFromMemory(id.allocId, &info);
	return id;
}

//------------------------------------------------------------------------------
/*
*/
inline void
DestroyTexture(const TextureId id)
{
	texturePool->DiscardResource(id.allocId);
}

//------------------------------------------------------------------------------
/*
*/
inline TextureMapInfo 
TextureMap(const TextureId id, IndexT mip, const CoreGraphics::GpuBufferTypes::MapType type)
{
	TextureMapInfo info;
	n_assert(texturePool->Map(id, mip, type, info));
	return info;
}

//------------------------------------------------------------------------------
/*
*/
inline void
TextureUnmap(const TextureId id, IndexT mip)
{
	texturePool->Unmap(id, mip);
}

} // namespace CoreGraphics