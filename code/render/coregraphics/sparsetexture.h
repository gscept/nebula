#pragma once
//------------------------------------------------------------------------------
/**
    Virtually mapped image

    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "coregraphics/gpubuffertypes.h"
#include "coregraphics/pixelformat.h"
#include "coregraphics/texture.h"
#include "coregraphics/config.h"
#include "math/rectangle.h"
namespace CoreGraphics
{

/// sparse buffer type
ID_24_8_TYPE(SparseTextureId);

struct SparseTexureCreateInfo
{
    SizeT width, height, depth;
    CoreGraphics::TextureType type;
    CoreGraphics::PixelFormat::Code format;
    SizeT mips, layers;
    SizeT samples;
    bool bindless : 1;
};

/// create sparse buffer
SparseTextureId CreateSparseTexture(const SparseTexureCreateInfo& info);
/// destroy sparse buffers
void DestroySparseTexture(const SparseTextureId id);

/// get vulkan bindless handle
uint SparseTextureGetBindlessHandle(const CoreGraphics::SparseTextureId id);

/// make resident a texture and a mip at a certain region and blit data from texture into sparse texture 
void SparseTextureMakeResident(const CoreGraphics::SparseTextureId id, const Math::rectangle<int>& region, IndexT mip, const CoreGraphics::TextureId tex, IndexT texMip);
/// evict a region of the sparse texture
void SparseTextureEvict(const CoreGraphics::SparseTextureId id, IndexT mip, const Math::rectangle<int>& region);
/// commit changes
void SparseTextureCommitChanges(const CoreGraphics::SparseTextureId id);

} // namespace CoreGraphics
