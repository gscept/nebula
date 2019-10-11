#pragma once
//------------------------------------------------------------------------------
/**
	RenderTexture is a texture which can be attached to the render device when rendering

	(C)2017-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/id.h"
#include "ids/idpool.h"
#include "ids/idallocator.h"
#include "coregraphics/texture.h"
#include "coregraphics/pixelformat.h"
#include "coregraphics/config.h"
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
	float width, height, depth;
	SizeT layers, mips;
	bool msaa : 1;
	bool window : 1;
	bool relativeSize : 1;
};

// much like the above, but for adjusted infos
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

/// swap buffers, only valid if this is a window texture
IndexT RenderTextureSwapBuffers(const CoreGraphics::RenderTextureId id);

/// resize the render texture
void RenderTextureResize(const RenderTextureId id, const RenderTextureResizeInfo& info);
/// trigger a window change callback
void RenderTextureWindowResized(const RenderTextureId id);

/// get texture dimensions
const CoreGraphics::TextureDimensions RenderTextureGetDimensions(const RenderTextureId id);
/// get texture mips
const SizeT RenderTextureGetNumMips(const RenderTextureId id);
/// get texture layers
const SizeT RenderTextureGetLayers(const RenderTextureId id);
/// get pixel format
const CoreGraphics::PixelFormat::Code RenderTextureGetPixelFormat(const RenderTextureId id);
/// get msaa
const bool RenderTextureGetMSAA(const RenderTextureId id);
/// get default layout (does not update during the frame)
const CoreGraphicsImageLayout RenderTextureGetLayout(const RenderTextureId id);

/// get bindless texture handle
uint RenderTextureGetBindlessHandle(const RenderTextureId id);

/// helper function to setup RenderTextureInfo, already implemented
RenderTextureInfo RenderTextureInfoSetupHelper(const RenderTextureCreateInfo& info);
/// helper function to setup RenderTextureInfo, already implemented
RenderTextureInfo RenderTextureInfoResizeHelper(const RenderTextureResizeInfo& info);


} // CoreGraphics
