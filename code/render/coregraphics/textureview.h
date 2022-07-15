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
    TextureId tex = InvalidTextureId;
    IndexT startMip = 0;
    SizeT numMips = 1;
    IndexT startLayer = 0;
    SizeT numLayers = 1;
    PixelFormat::Code format = PixelFormat::InvalidPixelFormat;
    CoreGraphics::TextureSwizzle swizzle = { TextureChannelMapping::None, TextureChannelMapping::None, TextureChannelMapping::None, TextureChannelMapping::None };
};

/// create texture view
TextureViewId CreateTextureView(const TextureViewCreateInfo& info);
/// destroy texture view
void DestroyTextureView(const TextureViewId id);

/// reload texture view by creating a new backend view with the old texture (assuming it's changed)
void TextureViewReload(const TextureViewId id);

/// get texture
TextureId TextureViewGetTexture(const TextureViewId id);

} // namespace CoreGraphics
