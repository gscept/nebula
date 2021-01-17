#pragma once
//------------------------------------------------------------------------------
/**
    A texture view is used on a texture to change the format, mip or layer 
    range used to sample from in a shader.

    @copyright
    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "coregraphics/config.h"
#include "coregraphics/texture.h"
namespace CoreGraphics
{

ID_24_8_TYPE(TextureViewId);

struct TextureViewCreateInfo
{
    TextureId tex;
    IndexT startMip;
    SizeT numMips;
    IndexT startLayer;
    SizeT numLayers;
    PixelFormat::Code format;
};

/// create texture view
TextureViewId CreateTextureView(const TextureViewCreateInfo& info);
/// destroy texture view
void DestroyTextureView(const TextureViewId id);

/// get texture
TextureId TextureViewGetTexture(const TextureViewId id);

} // namespace CoreGraphics
