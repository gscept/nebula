//------------------------------------------------------------------------------
//  shaderrwtexture.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "shaderrwtexture.h"
#include "coregraphics/displaydevice.h"
namespace CoreGraphics
{

//------------------------------------------------------------------------------
/**
*/
ShaderRWTextureInfo
ShaderRWTextureInfoSetupHelper(const ShaderRWTextureCreateInfo & info)
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
		rt.dynamicSize = false;
		rt.widthScale = rt.heightScale = rt.depthScale = 1.0f;
		rt.name = "__WINDOW__";
	}
	else
	{
		n_assert(info.width > 0 && info.height > 0 && info.depth > 0);
		n_assert(info.type == CoreGraphics::Texture2D || info.type == CoreGraphics::TextureCube || info.type == CoreGraphics::Texture2DArray || info.type == CoreGraphics::TextureCubeArray);
		n_assert(!(info.relativeSize & info.dynamicSize));

		rt.window = Ids::InvalidId32;
		rt.name = info.name;
		rt.relativeSize = info.relativeSize;
		rt.dynamicSize = info.dynamicSize;
		rt.mips = 1;
		rt.layers = info.type == CoreGraphics::TextureCubeArray ? 6 : 1;
		rt.widthScale = info.widthScale;
		rt.heightScale = info.heightScale;
		rt.depthScale = info.depthScale;
		rt.type = info.type;
		rt.format = info.format;

		if (rt.relativeSize)
		{
			CoreGraphics::WindowId wnd = DisplayDevice::Instance()->GetCurrentWindow();
			const CoreGraphics::DisplayMode mode = CoreGraphics::WindowGetDisplayMode(rt.window);
			rt.width = SizeT(mode.GetWidth() * rt.widthScale);
			rt.height = SizeT(mode.GetHeight() * rt.heightScale);
			rt.depth = 1;
		}
		else if (rt.dynamicSize)
		{
			rt.width = SizeT(info.height * rt.widthScale);
			rt.height = SizeT(info.width * rt.heightScale);
			rt.depth = SizeT(info.depth * rt.depthScale);
		}
		else
		{
			rt.width = info.width;
			rt.height = info.height;
			rt.depth = info.depth;
		}
	}
	return rt;
}

} // namespace CoreGraphics
