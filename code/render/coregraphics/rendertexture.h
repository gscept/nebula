#pragma once
//------------------------------------------------------------------------------
/**
	RenderTexture is a texture which can be attached to the render device when rendering

	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/id.h"
#include "ids/idpool.h"
#include "ids/idallocator.h"
#include "coregraphics/texture.h"
#include "coregraphics/pixelformat.h"
#include "resources/resourcepool.h"
#include "coregraphics/window.h"
namespace CoreGraphics
{

ID_24_8_TYPE(RenderTextureId); 

enum RenderTextureUsage
{
	ColorAttachment,
	DepthStencilAttachment,

	InvalidAttachment
};

struct RenderTextureCreateInfo
{
	Resources::ResourceName name;
	CoreGraphics::TextureType type;
	CoreGraphics::PixelFormat::Code format;
	RenderTextureUsage usage;
	SizeT width, height, depth;
	SizeT layers, mips;
	float widthScale, heightScale, depthScale;
	bool msaa : 1;
	bool window : 1;
	bool dynamicSize : 1;
	bool relativeSize : 1;
};

struct RenderTextureInfo
{
	Resources::ResourceName name;
	CoreGraphics::TextureType type;
	CoreGraphics::PixelFormat::Code format;
	RenderTextureUsage usage;
	SizeT width, height, depth;
	SizeT layers, mips;
	float widthScale, heightScale, depthScale;
	CoreGraphics::WindowId window;
	bool msaa : 1;
	bool isWindow : 1;
	bool dynamicSize : 1;
	bool relativeSize : 1;
};

struct RenderTextureResizeInfo
{
	SizeT width, height, depth;
	float widthScale, heightScale, depthScale;
};

/// create render texture
RenderTextureId CreateRenderTexture(const RenderTextureCreateInfo& info);
/// destroy render texture
void DestroyRenderTexture(const RenderTextureId id);

/// resize the render texture
void RenderTextureResize(const RenderTextureId id, const RenderTextureResizeInfo& info);
/// trigger a window change callback
void RenderTextureWindowResized(const RenderTextureId id);

/// get texture dimensions
const CoreGraphics::TextureDimensions RenderTextureGetDimensions(const RenderTextureId id);
/// get pixel format
const CoreGraphics::PixelFormat::Code RenderTextureGetPixelFormat(const RenderTextureId id);
/// get msaa
const bool RenderTextureGetMSAA(const RenderTextureId id);

/// helper function to setup RenderTextureInfo, already implemented
RenderTextureInfo RenderTextureInfoSetupHelper(const RenderTextureCreateInfo& info);
/// helper function to setup RenderTextureInfo, already implemented
RenderTextureInfo RenderTextureInfoResizeHelper(const RenderTextureResizeInfo& info);


} // CoreGraphics
