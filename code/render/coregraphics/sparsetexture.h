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

} // namespace CoreGraphics
