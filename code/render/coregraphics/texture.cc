//------------------------------------------------------------------------------
//  texture.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "coregraphics/config.h"
#include "coregraphics/texture.h"
#include "coregraphics/memorytexturepool.h"
#include "coregraphics/displaydevice.h"

namespace CoreGraphics
{

TextureId White1D;
TextureId Black2D;
TextureId White2D;
TextureId WhiteCube;
TextureId White3D;
TextureId White1DArray;
TextureId White2DArray;
TextureId WhiteCubeArray;

MemoryTexturePool* texturePool = nullptr;

//------------------------------------------------------------------------------
/**
*/
const TextureId
CreateTexture(const TextureCreateInfo& info)
{
	TextureId id = texturePool->ReserveResource(info.name, info.tag);
	n_assert(id.resourceType == TextureIdType);
	texturePool->LoadFromMemory(id, &info);
    return id;
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyTexture(const TextureId id)
{
	texturePool->DiscardResource(id);
}

//------------------------------------------------------------------------------
/**
*/
TextureDimensions
TextureGetDimensions(const TextureId id)
{
	return texturePool->GetDimensions(id);
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::PixelFormat::Code
TextureGetPixelFormat(const TextureId id)
{
	return texturePool->GetPixelFormat(id);
}

//------------------------------------------------------------------------------
/**
*/
TextureType
TextureGetType(const TextureId id)
{
	return texturePool->GetType(id);
}

//------------------------------------------------------------------------------
/**
*/
SizeT
TextureGetNumMips(const TextureId id)
{
	return texturePool->GetNumMips(id);
}

//------------------------------------------------------------------------------
/**
*/
SizeT 
TextureGetNumLayers(const TextureId id)
{
	return texturePool->GetNumLayers(id);
}

//------------------------------------------------------------------------------
/**
*/
SizeT 
TextureGetNumSamples(const TextureId id)
{
	return texturePool->GetNumSamples(id);
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::TextureId 
TextureGetAlias(const TextureId id)
{
	return texturePool->GetAlias(id);
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::TextureUsage 
TextureGetUsage(const TextureId id)
{
	return texturePool->GetUsageBits(id);
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::ImageLayout 
TextureGetDefaultLayout(const TextureId id)
{
	return texturePool->GetDefaultLayout(id);
}

//------------------------------------------------------------------------------
/**
*/
uint 
TextureGetBindlessHandle(const TextureId id)
{
	return texturePool->GetBindlessHandle(id);
}

//------------------------------------------------------------------------------
/**
*/
uint 
TextureGetStencilBindlessHandle(const TextureId id)
{
	return texturePool->GetStencilBindlessHandle(id);
}

//------------------------------------------------------------------------------
/**
*/
IndexT 
TextureSwapBuffers(const TextureId id)
{
	return texturePool->SwapBuffers(id);
}

//------------------------------------------------------------------------------
/**
*/
void 
TextureWindowResized(const TextureId id)
{
    texturePool->Reload(id);
}

//------------------------------------------------------------------------------
/**
*/
TextureMapInfo 
TextureMap(const TextureId id, IndexT mip, const CoreGraphics::GpuBufferTypes::MapType type)
{
	TextureMapInfo info;
	n_assert(texturePool->Map(id, mip, type, info));
	return info;
}

//------------------------------------------------------------------------------
/**
*/
void
TextureUnmap(const TextureId id, IndexT mip)
{
	texturePool->Unmap(id, mip);
}

//------------------------------------------------------------------------------
/**
*/
TextureMapInfo
TextureMapFace(const TextureId id, IndexT mip, TextureCubeFace face, const CoreGraphics::GpuBufferTypes::MapType type)
{
	TextureMapInfo info;
	n_assert(texturePool->MapCubeFace(id, face, mip, type, info));
	return info;
}

//------------------------------------------------------------------------------
/**
*/
void
TextureUnmapFace(const TextureId id, IndexT mip, TextureCubeFace face)
{
	texturePool->UnmapCubeFace(id, face, mip);
}

//------------------------------------------------------------------------------
/**
*/
void 
TextureGenerateMipmaps(const TextureId id)
{
	texturePool->GenerateMipmaps(id);
}

//------------------------------------------------------------------------------
/**
*/
TextureCreateInfoAdjusted 
TextureGetAdjustedInfo(const TextureCreateInfo& info)
{
	TextureCreateInfoAdjusted rt;
	if (info.windowTexture)
	{
		n_assert_fmt(info.samples == 1, "Texture created as window may not have any multisampling enabled");
		n_assert_fmt(info.alias == CoreGraphics::TextureId::Invalid(), "Texture created as window may not be alias");
		n_assert_fmt(info.buffer == nullptr, "Texture created as window may not have any buffer data");
		
		rt.window = CoreGraphics::DisplayDevice::Instance()->GetCurrentWindow();
		const CoreGraphics::DisplayMode mode = CoreGraphics::WindowGetDisplayMode(rt.window);
		rt.name = info.name;
		rt.usage = CoreGraphics::TextureUsage::RenderUsage | CoreGraphics::TextureUsage::CopyUsage;
		rt.tag = info.tag;
		rt.buffer = nullptr;
		rt.type = CoreGraphics::Texture2D;
		rt.format = mode.GetPixelFormat();
		rt.width = mode.GetWidth();
		rt.height = mode.GetHeight();
		rt.depth = 1;
		rt.widthScale = rt.heightScale = rt.depthScale = 1.0f;
		rt.layers = 1;
		rt.mips = 1;
		rt.samples = 1;
		rt.windowTexture = true;
		rt.windowRelative = true;
		rt.bindless = info.bindless;
		rt.alias = CoreGraphics::TextureId::Invalid();
		rt.defaultLayout = CoreGraphics::ImageLayout::Present;
	}
	else
	{
		n_assert(info.width > 0 && info.height > 0 && info.depth > 0);


		rt.name = info.name;
		rt.usage = info.usage;
		rt.tag = info.tag;
		rt.buffer = info.buffer;
		rt.type = info.type;
		rt.format = info.format;
		rt.width = (SizeT)info.width;
		rt.height = (SizeT)info.height;
		rt.depth = (SizeT)info.depth;
		rt.widthScale = 0;
		rt.heightScale = 0;
		rt.depthScale = 0;
		rt.mips = info.mips;
		rt.layers = (info.type == CoreGraphics::TextureCubeArray || info.type == CoreGraphics::TextureCube) ? 6 : info.layers; 
		rt.samples = info.samples;
		rt.windowTexture = false;
		rt.windowRelative = info.windowRelative;
		rt.bindless = info.bindless;
		rt.window = CoreGraphics::WindowId::Invalid();
		rt.alias = info.alias;
		rt.defaultLayout = info.defaultLayout;

		// correct depth-stencil formats if layout is shader read
		if (CoreGraphics::PixelFormat::IsDepthFormat(rt.format) && rt.defaultLayout == CoreGraphics::ImageLayout::ShaderRead)
			rt.defaultLayout = CoreGraphics::ImageLayout::DepthStencilRead;

		if (rt.windowRelative)
		{
			CoreGraphics::WindowId wnd = CoreGraphics::DisplayDevice::Instance()->GetCurrentWindow();
			const CoreGraphics::DisplayMode mode = CoreGraphics::WindowGetDisplayMode(wnd);
			rt.width = SizeT(Math::n_ceil(mode.GetWidth() * info.width));
			rt.height = SizeT(Math::n_ceil(mode.GetHeight() * info.height));
			rt.depth = 1;
			rt.window = wnd;

			rt.widthScale = info.width;
			rt.heightScale = info.height;
			rt.depthScale = info.depth;
		}


		// if the mip value is set to auto generate mips, generate mip chain
		if (info.mips == TextureAutoMips)
		{
			SizeT width = rt.width;
			SizeT height = rt.height;
			rt.mips = 1;
			while (true)
			{
				width = width >> 1;
				height = height >> 1;

				// break if any dimension reaches 0
				if (width == 0 || height == 0)
					break;
				rt.mips++;
			}
		}
	}
	return rt;
}

} // namespace CoreGraphics