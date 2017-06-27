//------------------------------------------------------------------------------
//  texturebase.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/base/texturebase.h"

namespace Base
{
__ImplementClass(Base::TextureBase, 'TXRB', Base::GpuResourceBase);

using namespace CoreGraphics;

//------------------------------------------------------------------------------
/**
*/
TextureBase::TextureBase() :
    type(InvalidType),
    width(0),
    height(0),
    depth(0),
    numMipLevels(0),
    skippedMips(0),
    pixelFormat(PixelFormat::InvalidPixelFormat),
	isRenderTargetAttachment(false)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
TextureBase::~TextureBase()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
bool
TextureBase::Map(IndexT mipLevel, Access accessMode, MapInfo& outMapInfo)
{
    n_error("TextureBase::Map() called!");
    return false;
}

//------------------------------------------------------------------------------
/**
*/
void
TextureBase::Unmap(IndexT mipLevel)
{
    n_error("TextureBase::Unmap() called!");
}

//------------------------------------------------------------------------------
/**
*/
bool
TextureBase::MapCubeFace(CubeFace face, IndexT mipLevel, Access accessMode, MapInfo& outMapInfo)
{
    n_error("TextureBase::MapCubeFace() called!");
    return false;
}

//------------------------------------------------------------------------------
/**
*/
void
TextureBase::UnmapCubeFace(CubeFace cubeFace, IndexT mipLevel)
{
    n_error("TextureBase::UnmapCubeFace() called!");
}

//------------------------------------------------------------------------------
/**
*/
void 
TextureBase::Update( void* data, SizeT size, SizeT width, SizeT height, IndexT left, IndexT top, IndexT mip )
{
	n_error("TextureBase::Update() called!");
}
} // namespace Base
