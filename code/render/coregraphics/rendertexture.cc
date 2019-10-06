//------------------------------------------------------------------------------
//  rendertexture.cc
//  (C)2017-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "config.h"
#include "rendertexture.h"
#include "coregraphics/displaydevice.h"
#include "coregraphics/texture.h"
#include "coregraphics/memorytexturepool.h"
namespace CoreGraphics
{

//------------------------------------------------------------------------------
/**
*/
RenderTextureInfo
RenderTextureInfoSetupHelper(const RenderTextureCreateInfo& info)
{
	RenderTextureInfo rt;
	if (info.window)
	{
		rt.isWindow = true;
		rt.window = DisplayDevice::Instance()->GetCurrentWindow();
		const CoreGraphics::DisplayMode mode = CoreGraphics::WindowGetDisplayMode(rt.window);
		rt.width = mode.GetWidth();
		rt.height = mode.GetHeight();
		rt.depth = 1;
		rt.type = CoreGraphics::Texture2D;
		rt.usage = ColorAttachment;
		rt.format = mode.GetPixelFormat();
		rt.layers = 1;
		rt.mips = 1;
		rt.relativeSize = true;
		rt.msaa = false;
		rt.widthScale = rt.heightScale = rt.depthScale = 1.0f;
		rt.name = info.name;
	}
	else
	{
		n_assert(info.width > 0 && info.height > 0 && info.depth > 0);
		n_assert(info.type == CoreGraphics::Texture2D || info.type == CoreGraphics::TextureCube || info.type == CoreGraphics::Texture2DArray || info.type == CoreGraphics::TextureCubeArray);
		n_assert(info.usage != InvalidAttachment);

		rt.isWindow = false;
		rt.window = Ids::InvalidId32;
		rt.name = info.name;
		rt.relativeSize = info.relativeSize;
		rt.msaa = info.msaa;
		rt.mips = 1;
		rt.layers = info.type == CoreGraphics::TextureCubeArray ? 6 : info.layers;
		rt.width = (SizeT)info.width;
		rt.height = (SizeT)info.height;
		rt.depth = (SizeT)info.depth;
		rt.widthScale = 0;
		rt.heightScale = 0;
		rt.depthScale = 0;
		rt.type = info.type;
		rt.usage = info.usage;
		rt.format = info.format;

		if (rt.relativeSize)
		{
			CoreGraphics::WindowId wnd = DisplayDevice::Instance()->GetCurrentWindow();
			const CoreGraphics::DisplayMode mode = CoreGraphics::WindowGetDisplayMode(wnd);
			rt.width = SizeT(mode.GetWidth() * info.width);
			rt.height = SizeT(mode.GetHeight() * info.height);
			rt.depth = 1;
			rt.window = wnd;

			rt.widthScale = info.width;
			rt.heightScale = info.height;
			rt.depthScale = info.depth;
		}
	}
	return rt;
}

//------------------------------------------------------------------------------
/**
*/
RenderTextureInfo
RenderTextureInfoResizeHelper(const RenderTextureResizeInfo& info)
{
	RenderTextureInfo rt;
	n_assert(info.width > 0 && info.height > 0 && info.depth > 0);
	n_assert(rt.type == CoreGraphics::Texture2D || rt.type == CoreGraphics::TextureCube);
	n_assert(rt.usage != InvalidAttachment);
	CoreGraphics::WindowId wnd = DisplayDevice::Instance()->GetCurrentWindow();
	if (rt.relativeSize)
	{
		const CoreGraphics::DisplayMode mode = CoreGraphics::WindowGetDisplayMode(rt.window);
		rt.width = SizeT(mode.GetWidth() * info.widthScale);
		rt.height = SizeT(mode.GetHeight() * info.heightScale);
		rt.depth = 1;
	}
	return rt;
}


} // CoreGraphics
