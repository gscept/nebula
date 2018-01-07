#pragma once
//------------------------------------------------------------------------------
/**
	The shader read-write texture works like any read/write GPU resource, however it is meant to be used as a texture

	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/id.h"
#include "ids/idpool.h"
#include "resources/resourceid.h"
#include "util/stringatom.h"
#include "coregraphics/pixelformat.h"
#include "coregraphics/texture.h"
namespace CoreGraphics
{
ID_24_8_TYPE(ShaderRWTextureId);

struct ShaderRWTextureCreateInfo
{
	Resources::ResourceName name;
	CoreGraphics::TextureType type;
	CoreGraphics::PixelFormat::Code format;
	SizeT width, height, depth;
	SizeT layers, mips;
	float widthScale, heightScale, depthScale;
	bool window : 1;
	bool dynamicSize : 1;
	bool relativeSize : 1;
};

struct ShaderRWTextureResizeInfo
{
	SizeT width, height, depth;
	float widthScale, heightScale, depthScale;
};

/// create new RW texture
const ShaderRWTextureId CreateShaderRWTexture(const ShaderRWTextureCreateInfo& info);
/// destroy RW texture
void DestroyShaderRWTexture(const ShaderRWTextureId id);

/// resize texture
void ShaderRWTextureResize(const ShaderRWTextureId id, const ShaderRWTextureResizeInfo& info);
/// trigger a window change callback
void RenderTextureWindowResized(const ShaderRWTextureId id);
/// clear texture
void ShaderRWTextureClear(const ShaderRWTextureId id, const Math::float4& color);

} // CoreGraphics
