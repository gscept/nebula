//------------------------------------------------------------------------------
//  shaderrwtexture.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "shaderrwtexture.h"
#include "coregraphics/displaydevice.h"
namespace CoreGraphics
{

//------------------------------------------------------------------------------
/**
*/
ShaderRWTextureInfo
ShaderRWTextureInfoSetupHelper(const ShaderRWTextureCreateInfo& info)
{
	ShaderRWTextureInfo rt;
	if (info.window)
	{
		rt.isWindow = true;
		rt.window = DisplayDevice::Instance()->GetCurrentWindow();
		const CoreGraphics::DisplayMode mode = CoreGraphics::WindowGetDisplayMode(rt.window);
		rt.width = mode.GetWidth();
		rt.height = mode.GetHeight();
		rt.depth = 1;
		rt.type = CoreGraphics::Texture2D;
		rt.format = mode.GetPixelFormat();
		rt.layers = 1;
		rt.mips = 1;
		rt.relativeSize = true;
		rt.widthScale = rt.heightScale = rt.depthScale = 1.0f;
		rt.name = "__WINDOW__";
	}
	else
	{
		n_assert(info.width > 0 && info.height > 0 && info.depth > 0);
		n_assert(info.type == CoreGraphics::Texture2D || info.type == CoreGraphics::TextureCube || info.type == CoreGraphics::Texture2DArray || info.type == CoreGraphics::TextureCubeArray);

		rt.isWindow = false;
		rt.window = Ids::InvalidId32;
		rt.name = info.name;
		rt.relativeSize = info.relativeSize;
		rt.mips = 1;
		rt.layers = info.type == CoreGraphics::TextureCubeArray ? 6 : 1;
		rt.width = (SizeT)info.width;
		rt.height = (SizeT)info.height;
		rt.depth = (SizeT)info.depth;
		rt.widthScale = 0;
		rt.heightScale = 0;
		rt.depthScale = 0;
		rt.type = info.type;
		rt.format = info.format;

		if (rt.relativeSize)
		{
			CoreGraphics::WindowId wnd = DisplayDevice::Instance()->GetCurrentWindow();
			const CoreGraphics::DisplayMode mode = CoreGraphics::WindowGetDisplayMode(wnd);
			rt.width = SizeT(mode.GetWidth() * info.width);
			rt.height = SizeT(mode.GetHeight() * info.height);
			rt.depth = 1;

			rt.widthScale = info.width;
			rt.heightScale = info.height;
			rt.depthScale = info.depth;
			rt.window = wnd;
		}
	}
	return rt;
}

} // namespace CoreGraphics
