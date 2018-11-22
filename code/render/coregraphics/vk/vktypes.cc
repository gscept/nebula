//------------------------------------------------------------------------------
// vktypes.cc
// (C) 2016-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "vktypes.h"
#include <vulkan/vulkan.h>
#include "coregraphics/pixelformat.h"
#include "il_dds.h"

namespace Vulkan
{

using namespace CoreGraphics;
//------------------------------------------------------------------------------
/**
*/
VkFormat
VkTypes::AsVkFormat(CoreGraphics::PixelFormat::Code p)
{
	switch (p)
	{
	case PixelFormat::R8G8B8X8:         return VK_FORMAT_R8G8B8A8_UNORM;
	case PixelFormat::R8G8B8A8:         return VK_FORMAT_R8G8B8A8_UNORM;
	case PixelFormat::A8B8G8R8:         return VK_FORMAT_B8G8R8A8_UNORM;
	case PixelFormat::R8G8B8:           return VK_FORMAT_R8G8B8_UNORM;
	case PixelFormat::R5G6B5:           return VK_FORMAT_R5G6B5_UNORM_PACK16;
	case PixelFormat::SRGBA8:			return VK_FORMAT_R8G8B8A8_SRGB;
	case PixelFormat::R5G5B5A1:         return VK_FORMAT_R5G5B5A1_UNORM_PACK16;
	case PixelFormat::R4G4B4A4:         return VK_FORMAT_R4G4B4A4_UNORM_PACK16;
	case PixelFormat::DXT1:             return VK_FORMAT_BC1_RGB_UNORM_BLOCK;
	case PixelFormat::DXT1A:            return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
	case PixelFormat::DXT3:             return VK_FORMAT_BC2_UNORM_BLOCK;
	case PixelFormat::DXT5:             return VK_FORMAT_BC3_UNORM_BLOCK;
	case PixelFormat::DXT1sRGB:         return VK_FORMAT_BC1_RGB_SRGB_BLOCK;
	case PixelFormat::DXT1AsRGB:        return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
	case PixelFormat::DXT3sRGB:         return VK_FORMAT_BC2_SRGB_BLOCK;
	case PixelFormat::DXT5sRGB:         return VK_FORMAT_BC3_SRGB_BLOCK;
	case PixelFormat::BC7:				return VK_FORMAT_BC7_UNORM_BLOCK;
	case PixelFormat::BC7sRGB:			return VK_FORMAT_BC7_SRGB_BLOCK;
	case PixelFormat::R16F:             return VK_FORMAT_R16_SFLOAT;
	case PixelFormat::R16G16F:          return VK_FORMAT_R16G16_SFLOAT;
	case PixelFormat::R16G16B16A16F:    return VK_FORMAT_R16G16B16A16_SFLOAT;
	case PixelFormat::R16G16B16A16:		return VK_FORMAT_R16G16B16A16_UINT;
	case PixelFormat::R11G11B10F:		return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
	case PixelFormat::R32F:             return VK_FORMAT_R32_SFLOAT;
	case PixelFormat::R32G32F:          return VK_FORMAT_R32G32_SFLOAT;
	case PixelFormat::R32G32B32A32F:    return VK_FORMAT_R32G32B32A32_SFLOAT;
	case PixelFormat::R32G32B32F:		return VK_FORMAT_R32G32B32_SFLOAT;
	case PixelFormat::A8:               return VK_FORMAT_R8_UNORM;
	case PixelFormat::R8:               return VK_FORMAT_R8_UNORM;
	case PixelFormat::G8:               return VK_FORMAT_R8_UNORM;
	case PixelFormat::B8:               return VK_FORMAT_R8_UNORM;
	case PixelFormat::R10G10B10A2:      return VK_FORMAT_A2B10G10R10_UINT_PACK32;
	case PixelFormat::R16G16:           return VK_FORMAT_R16G16_UINT;
	case PixelFormat::D32S8:			return VK_FORMAT_D32_SFLOAT_S8_UINT;
	case PixelFormat::D24X8:			return VK_FORMAT_X8_D24_UNORM_PACK32;
	case PixelFormat::D24S8:            return VK_FORMAT_D24_UNORM_S8_UINT;
	default:
		{
			n_error("VkTypes::AsVkFormat(): invalid pixel format '%d'", p);
			return VK_FORMAT_R8G8B8A8_UINT;
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
VkFormat
VkTypes::AsVkFormat(ILenum p)
{
	switch (p)
	{
	case PF_ARGB:				return VK_FORMAT_R8G8B8A8_UNORM;
	case PF_RGB:				return VK_FORMAT_R8G8B8_UNORM;
	case PF_DXT1:				return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
	case PF_DXT3:				return VK_FORMAT_BC2_UNORM_BLOCK;
	case PF_DXT5:				return VK_FORMAT_BC3_UNORM_BLOCK;
	case PF_BC7:				return VK_FORMAT_BC7_UNORM_BLOCK;
	case PF_DXT1_sRGB:			return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
	case PF_DXT3_sRGB:			return VK_FORMAT_BC2_SRGB_BLOCK;
	case PF_DXT5_sRGB:			return VK_FORMAT_BC3_SRGB_BLOCK;
	case PF_BC7_sRGB:			return VK_FORMAT_BC7_SRGB_BLOCK;
	case PF_A16R16G16B16:		return VK_FORMAT_R16G16B16A16_UNORM;
	case PF_A16B16G16R16:		return VK_FORMAT_R16G16B16A16_UNORM;
	case PF_A16R16B16G16F:		return VK_FORMAT_R16G16B16A16_SFLOAT;
	case PF_A16B16G16R16F:		return VK_FORMAT_R16G16B16A16_SFLOAT;
	case PF_A32B32G32R32F:		return VK_FORMAT_R32G32B32A32_SFLOAT;
	case PF_A32R32G32B32F:		return VK_FORMAT_R32G32B32A32_SFLOAT;
	case PF_R16F:				return VK_FORMAT_R16_SFLOAT;
	case PF_R32F:				return VK_FORMAT_R32_SFLOAT;
	case PF_G16R16F:			return VK_FORMAT_R16G16_SFLOAT;
	case PF_G32R32F:			return VK_FORMAT_R32G32_SFLOAT;
	default:
		{
			n_error("VkTypes::AsVkFormat(): invalid compression '%d'", p);
			return VK_FORMAT_A8B8G8R8_UINT_PACK32;
		}		
	}
}

//------------------------------------------------------------------------------
/**
*/
ILenum
VkTypes::AsILDXTFormat(VkFormat p)
{
	switch (p)
	{

		case VK_FORMAT_BC1_RGBA_UNORM_BLOCK: 	return IL_DXT1;
		case VK_FORMAT_BC2_UNORM_BLOCK:			return IL_DXT3;
		case VK_FORMAT_BC3_UNORM_BLOCK:			return IL_DXT5;
		case VK_FORMAT_BC7_UNORM_BLOCK:			return IL_BPTC;
		case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:		return IL_DXT1A_sRGB;
		case VK_FORMAT_BC2_SRGB_BLOCK:			return IL_DXT3_sRGB;
		case VK_FORMAT_BC3_SRGB_BLOCK:			return IL_DXT5_sRGB;
		case VK_FORMAT_BC7_SRGB_BLOCK:			return IL_BPTC_sRGB;
		default:
		{
			n_error("VkTypes::AsVkFormat(): invalid compression '%d'", p);
			return IL_NO_COMPRESSION;
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
bool
VkTypes::IsCompressedFormat(VkFormat p)
{
	switch (p)
	{
	case VK_FORMAT_BC1_RGBA_UNORM_BLOCK: 	return true;
	case VK_FORMAT_BC2_UNORM_BLOCK:			return true;
	case VK_FORMAT_BC3_UNORM_BLOCK:			return true;
	case VK_FORMAT_BC7_UNORM_BLOCK:			return true;
	case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:		return true;
	case VK_FORMAT_BC2_SRGB_BLOCK:			return true;
	case VK_FORMAT_BC3_SRGB_BLOCK:			return true;
	case VK_FORMAT_BC7_SRGB_BLOCK:			return true;
	default:								return false;
	}
}

//------------------------------------------------------------------------------
/**
*/
VkTypes::VkBlockDimensions
VkTypes::AsVkBlockSize(CoreGraphics::PixelFormat::Code p)
{
	switch (p)
	{
	case PixelFormat::R8G8B8X8:         return { 1, 1 };
	case PixelFormat::R8G8B8A8:         return { 1, 1 };
	case PixelFormat::A8B8G8R8:         return { 1, 1 };
	case PixelFormat::R8G8B8:           return { 1, 1 };
	case PixelFormat::R5G6B5:           return { 1, 1 };
	case PixelFormat::SRGBA8:			return { 1, 1 };
	case PixelFormat::R5G5B5A1:         return { 1, 1 };
	case PixelFormat::R4G4B4A4:         return { 1, 1 };
	case PixelFormat::DXT1:             return { 4, 4 };
	case PixelFormat::DXT1A:            return { 4, 4 };
	case PixelFormat::DXT3:             return { 4, 4 };
	case PixelFormat::DXT5:             return { 4, 4 };
	case PixelFormat::DXT1sRGB:         return { 4, 4 };
	case PixelFormat::DXT1AsRGB:        return { 4, 4 };
	case PixelFormat::DXT3sRGB:         return { 4, 4 };
	case PixelFormat::DXT5sRGB:         return { 4, 4 };
	case PixelFormat::BC7:				return { 4, 4 };
	case PixelFormat::BC7sRGB:			return { 4, 4 };
	case PixelFormat::R16F:             return { 1, 1 };
	case PixelFormat::R16G16F:          return { 1, 1 };
	case PixelFormat::R16G16B16A16F:    return { 1, 1 };
	case PixelFormat::R16G16B16A16:		return { 1, 1 };
	case PixelFormat::R11G11B10F:		return { 1, 1 };
	case PixelFormat::R32F:             return { 1, 1 };
	case PixelFormat::R32G32F:          return { 1, 1 };
	case PixelFormat::R32G32B32A32F:    return { 1, 1 };
	case PixelFormat::R32G32B32F:		return { 1, 1 };
	case PixelFormat::A8:               return { 1, 1 };
	case PixelFormat::R8:               return { 1, 1 };
	case PixelFormat::G8:               return { 1, 1 };
	case PixelFormat::B8:               return { 1, 1 };
	case PixelFormat::R10G10B10A2:      return { 1, 1 };
	case PixelFormat::R16G16:           return { 1, 1 };
	case PixelFormat::D32S8:			return { 1, 1 };
	case PixelFormat::D24X8:			return { 1, 1 };
	case PixelFormat::D24S8:            return { 1, 1 };
	default:
	{
		n_error("VkTypes::AsVkFormat(): invalid pixel format '%d'", p);
		return { 1, 1 };
	}
	}
}

//------------------------------------------------------------------------------
/**
*/
Vulkan::VkTypes::VkBlockDimensions
VkTypes::AsVkBlockSize(VkFormat fmt)
{
	switch (fmt)
	{
	case VK_FORMAT_BC1_RGB_UNORM_BLOCK:		return {4, 4};
	case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:	return {4, 4};
	case VK_FORMAT_BC2_UNORM_BLOCK:			return {4, 4};
	case VK_FORMAT_BC3_UNORM_BLOCK:			return {4, 4};
	case VK_FORMAT_BC1_RGB_SRGB_BLOCK:		return {4, 4};
	case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:		return {4, 4};
	case VK_FORMAT_BC2_SRGB_BLOCK:			return {4, 4};
	case VK_FORMAT_BC3_SRGB_BLOCK:			return {4, 4};
	case VK_FORMAT_BC7_UNORM_BLOCK:			return {4, 4};
	case VK_FORMAT_BC7_SRGB_BLOCK:			return {4, 4};
	case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:	return {4, 4};
	case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:		return {4, 4};
	case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:	return {5, 4};
	case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:		return {5, 4};
	case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:	return {5, 5};
	case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:		return {5, 5};
	case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:	return {6, 5};
	case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:		return {6, 5};
	case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:	return {6, 6};
	case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:		return {6, 6};
	case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:	return {8, 5};
	case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:		return {8, 5};
	case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:	return {8, 6};
	case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:		return {8, 6};
	case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:	return {8, 8};
	case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:		return {8, 8};
	case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:	return {10, 5};
	case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:	return {10, 5};
	case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:	return {10, 6};
	case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:	return {10, 6};
	case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:	return {10, 8};
	case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:	return {10, 8};
	case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:	return {10, 10};
	case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:	return {10, 10};
	case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:	return {12, 10};
	case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:	return {12, 10};
	case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:	return {12, 12};
	case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:	return {12, 12};
	default:
		return { 1, 1 };
	}
}

//------------------------------------------------------------------------------
/**
*/
VkFormat
VkTypes::AsVkMappableImageFormat(VkFormat fmt)
{
	switch (fmt)
	{
	case VK_FORMAT_BC1_RGB_UNORM_BLOCK:		return VK_FORMAT_R8G8B8_UNORM;
	case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:	return VK_FORMAT_R8G8B8A8_UNORM;
	case VK_FORMAT_BC2_UNORM_BLOCK:			return VK_FORMAT_R8G8B8A8_UNORM;
	case VK_FORMAT_BC3_UNORM_BLOCK:			return VK_FORMAT_R8G8B8A8_UNORM;
	case VK_FORMAT_BC1_RGB_SRGB_BLOCK:		return VK_FORMAT_R8G8B8_UNORM;
	case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:		return VK_FORMAT_R8G8B8A8_UNORM;
	case VK_FORMAT_BC2_SRGB_BLOCK:			return VK_FORMAT_R8G8B8A8_UNORM;
	case VK_FORMAT_BC3_SRGB_BLOCK:			return VK_FORMAT_R8G8B8A8_UNORM;
	case VK_FORMAT_BC7_UNORM_BLOCK:			return VK_FORMAT_R8G8B8A8_UNORM;
	case VK_FORMAT_BC7_SRGB_BLOCK:			return VK_FORMAT_R8G8B8A8_UNORM;
	case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:	return VK_FORMAT_R8G8B8A8_UNORM;
	case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:		return VK_FORMAT_R8G8B8A8_UNORM;
	case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:	return VK_FORMAT_R8G8B8A8_UNORM;
	case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:		return VK_FORMAT_R8G8B8A8_UNORM;
	case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:	return VK_FORMAT_R8G8B8A8_UNORM;
	case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:		return VK_FORMAT_R8G8B8A8_UNORM;
	case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:	return VK_FORMAT_R8G8B8A8_UNORM;
	case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:		return VK_FORMAT_R8G8B8A8_UNORM;
	case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:	return VK_FORMAT_R8G8B8A8_UNORM;
	case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:		return VK_FORMAT_R8G8B8A8_UNORM;
	case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:	return VK_FORMAT_R8G8B8A8_UNORM;
	case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:		return VK_FORMAT_R8G8B8A8_UNORM;
	case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:	return VK_FORMAT_R8G8B8A8_UNORM;
	case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:		return VK_FORMAT_R8G8B8A8_UNORM;
	case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:	return VK_FORMAT_R8G8B8A8_UNORM;
	case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:		return VK_FORMAT_R8G8B8A8_UNORM;
	case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:	return VK_FORMAT_R8G8B8A8_UNORM;
	case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:	return VK_FORMAT_R8G8B8A8_UNORM;
	case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:	return VK_FORMAT_R8G8B8A8_UNORM;
	case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:	return VK_FORMAT_R8G8B8A8_UNORM;
	case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:	return VK_FORMAT_R8G8B8A8_UNORM;
	case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:	return VK_FORMAT_R8G8B8A8_UNORM;
	case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:	return VK_FORMAT_R8G8B8A8_UNORM;
	case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:	return VK_FORMAT_R8G8B8A8_UNORM;
	case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:	return VK_FORMAT_R8G8B8A8_UNORM;
	case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:	return VK_FORMAT_R8G8B8A8_UNORM;
	case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:	return VK_FORMAT_R8G8B8A8_UNORM;
	case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:	return VK_FORMAT_R8G8B8A8_UNORM;
	default:
		return fmt;
	}
}

//------------------------------------------------------------------------------
/**
*/
VkFormat
VkTypes::AsVkFramebufferFormat(CoreGraphics::PixelFormat::Code p)
{
	switch (p)
	{
	case PixelFormat::R8G8B8X8:         
	case PixelFormat::R8G8B8A8:         return VK_FORMAT_R8G8B8A8_SNORM;
	case PixelFormat::A8B8G8R8:         return VK_FORMAT_B8G8R8A8_UNORM;
	case PixelFormat::R8G8B8:           return VK_FORMAT_R8G8B8_SNORM;
	case PixelFormat::R5G6B5:           return VK_FORMAT_R5G6B5_UNORM_PACK16;
	case PixelFormat::SRGBA8:			return VK_FORMAT_B8G8R8A8_SRGB;
	case PixelFormat::R5G5B5A1:         return VK_FORMAT_R5G5B5A1_UNORM_PACK16;
	case PixelFormat::R4G4B4A4:         return VK_FORMAT_R4G4B4A4_UNORM_PACK16;
	case PixelFormat::DXT1:             return VK_FORMAT_BC1_RGB_UNORM_BLOCK;
	case PixelFormat::DXT1A:            return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
	case PixelFormat::DXT3:             return VK_FORMAT_BC2_UNORM_BLOCK;
	case PixelFormat::DXT5:             return VK_FORMAT_BC3_UNORM_BLOCK;
	case PixelFormat::DXT1sRGB:         return VK_FORMAT_BC1_RGB_SRGB_BLOCK;
	case PixelFormat::DXT1AsRGB:        return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
	case PixelFormat::DXT3sRGB:         return VK_FORMAT_BC2_SRGB_BLOCK;
	case PixelFormat::DXT5sRGB:         return VK_FORMAT_BC3_SRGB_BLOCK;
	case PixelFormat::BC7:				return VK_FORMAT_BC7_UNORM_BLOCK;
	case PixelFormat::BC7sRGB:			return VK_FORMAT_BC7_SRGB_BLOCK;
	case PixelFormat::R16F:             return VK_FORMAT_R16_SFLOAT;
	case PixelFormat::R16G16F:          return VK_FORMAT_R16G16_SFLOAT;
	case PixelFormat::R16G16B16A16F:    return VK_FORMAT_R16G16B16A16_SFLOAT;
	case PixelFormat::R16G16B16A16:		return VK_FORMAT_R16G16B16A16_SNORM;
	case PixelFormat::R11G11B10F:		return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
	case PixelFormat::R32F:             return VK_FORMAT_R32_SFLOAT;
	case PixelFormat::R32G32F:          return VK_FORMAT_R32G32_SFLOAT;
	case PixelFormat::R32G32B32A32F:    return VK_FORMAT_R32G32B32A32_SFLOAT;
	case PixelFormat::R32G32B32F:		return VK_FORMAT_R32G32B32_SFLOAT;
	case PixelFormat::A8:               return VK_FORMAT_R8_SNORM;
	case PixelFormat::R8:               return VK_FORMAT_R8_SNORM;
	case PixelFormat::G8:               return VK_FORMAT_R8_SNORM;
	case PixelFormat::B8:               return VK_FORMAT_R8_SNORM;
	case PixelFormat::R10G10B10A2:      return VK_FORMAT_A2B10G10R10_UINT_PACK32;
	case PixelFormat::R16G16:           return VK_FORMAT_R16G16_SNORM;
	case PixelFormat::D32S8:			return VK_FORMAT_D32_SFLOAT_S8_UINT;
	case PixelFormat::D24X8:			return VK_FORMAT_X8_D24_UNORM_PACK32;
	case PixelFormat::D24S8:            return VK_FORMAT_D24_UNORM_S8_UINT;
	default:
	{
		n_error("VkTypes::AsVkFormat(): invalid pixel format '%d'", p);
		return VK_FORMAT_R8G8B8A8_UINT;
	}
	}
}
//------------------------------------------------------------------------------
/**
*/
VkFormat
VkTypes::AsVkDataFormat(CoreGraphics::PixelFormat::Code p)
{
	switch (p)
	{
		case PixelFormat::R8G8B8X8:
		case PixelFormat::R8G8B8A8:         return VK_FORMAT_R8G8B8A8_UINT;
		case PixelFormat::A8B8G8R8:         return VK_FORMAT_R8G8B8A8_UINT;
		case PixelFormat::R8G8B8:           return VK_FORMAT_R8G8B8_UINT;
		case PixelFormat::R16G16F:          return VK_FORMAT_R16G16_SFLOAT;
		case PixelFormat::R16G16B16A16F:    return VK_FORMAT_R16G16B16A16_SFLOAT;
		case PixelFormat::R16G16B16A16:		return VK_FORMAT_R16G16B16A16_UINT;
		case PixelFormat::R32F:             return VK_FORMAT_R32_SFLOAT;
		case PixelFormat::R16F:				return VK_FORMAT_R16_SFLOAT;
		case PixelFormat::R32G32F:          return VK_FORMAT_R32G32_SFLOAT;
		case PixelFormat::R32G32B32A32F:    return VK_FORMAT_R32G32B32A32_SFLOAT;
		case PixelFormat::R32G32B32F:		return VK_FORMAT_R32G32B32_SFLOAT;
		case PixelFormat::A8:               return VK_FORMAT_R8_UINT;
		case PixelFormat::R8:               return VK_FORMAT_R8_UINT;
		case PixelFormat::G8:               return VK_FORMAT_R8_UINT;
		case PixelFormat::B8:               return VK_FORMAT_R8_UINT;
		case PixelFormat::R16G16:           return VK_FORMAT_R16G16_UINT;
		default:
		{
			n_error("VkTypes::AsVkFormat(): invalid pixel format '%d'", p);
			return VK_FORMAT_R8G8B8A8_UINT;
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::PixelFormat::Code
VkTypes::AsNebulaPixelFormat(VkFormat f)
{
	
	switch (f)
	{
	case VK_FORMAT_R8G8B8A8_UINT:					return PixelFormat::R8G8B8A8;
	case VK_FORMAT_R8G8B8_UINT:						return PixelFormat::R8G8B8;
	case VK_FORMAT_R8G8B8A8_UNORM:					return PixelFormat::R8G8B8A8;
	case VK_FORMAT_B8G8R8A8_UNORM:					return PixelFormat::A8B8G8R8;
	case VK_FORMAT_R8G8B8_UNORM:					return PixelFormat::R8G8B8;
	case VK_FORMAT_R5G6B5_UNORM_PACK16:				return PixelFormat::R5G6B5;
	case VK_FORMAT_R8G8B8A8_SRGB:					return PixelFormat::SRGBA8;
	case VK_FORMAT_B8G8R8A8_SRGB:					return PixelFormat::SRGBA8;
	case VK_FORMAT_R5G5B5A1_UNORM_PACK16:			return PixelFormat::R5G5B5A1;
	case VK_FORMAT_R4G4B4A4_UNORM_PACK16:			return PixelFormat::R4G4B4A4;
	case VK_FORMAT_BC1_RGB_UNORM_BLOCK:				return PixelFormat::DXT1;
	case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:			return PixelFormat::DXT1A;
	case VK_FORMAT_BC2_UNORM_BLOCK:					return PixelFormat::DXT3;
	case VK_FORMAT_BC3_UNORM_BLOCK:					return PixelFormat::DXT5;
	case VK_FORMAT_BC1_RGB_SRGB_BLOCK:				return PixelFormat::DXT1sRGB;
	case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:				return PixelFormat::DXT1AsRGB;
	case VK_FORMAT_BC2_SRGB_BLOCK:					return PixelFormat::DXT3sRGB;
	case VK_FORMAT_BC3_SRGB_BLOCK:					return PixelFormat::DXT5sRGB;
	case VK_FORMAT_BC7_UNORM_BLOCK:					return PixelFormat::BC7;
	case VK_FORMAT_BC7_SRGB_BLOCK:					return PixelFormat::BC7sRGB;
	case VK_FORMAT_R16_SFLOAT:						return PixelFormat::R16F;
	case VK_FORMAT_R16G16_SFLOAT:					return PixelFormat::R16G16F;
	case VK_FORMAT_R16G16B16A16_SFLOAT:				return PixelFormat::R16G16B16A16F;
	case VK_FORMAT_R16G16B16A16_UINT:				return PixelFormat::R16G16B16A16;
	case VK_FORMAT_R16G16B16A16_UNORM:				return PixelFormat::R16G16B16A16;
	case VK_FORMAT_B10G11R11_UFLOAT_PACK32:			return PixelFormat::R11G11B10F;
	case VK_FORMAT_R32_SFLOAT:						return PixelFormat::R32F;
	case VK_FORMAT_R32G32_SFLOAT:					return PixelFormat::R32G32F;
	case VK_FORMAT_R32G32B32A32_SFLOAT:				return PixelFormat::R32G32B32A32F;
	case VK_FORMAT_R32G32B32_SFLOAT:				return PixelFormat::R32G32B32F;
	case VK_FORMAT_R8_UNORM:						return PixelFormat::R8;
	case VK_FORMAT_A2B10G10R10_UINT_PACK32:			return PixelFormat::R10G10B10A2;
	case VK_FORMAT_R16G16_UINT:						return PixelFormat::R16G16;
	case VK_FORMAT_X8_D24_UNORM_PACK32:				return PixelFormat::D24X8;
	case VK_FORMAT_D24_UNORM_S8_UINT:				return PixelFormat::D24S8;
	case VK_FORMAT_D32_SFLOAT_S8_UINT:				return PixelFormat::D32S8;
	default:
	{
		n_error("VkTypes::AsNebulaPixelFormat(): invalid pixel format '%d'", f);
		return PixelFormat::R8G8B8A8;
	}
	}
}

//------------------------------------------------------------------------------
/**
*/
VkComponentMapping
VkTypes::AsVkMapping(CoreGraphics::PixelFormat::Code p)
{
	VkComponentMapping mapping;
	mapping.r = VK_COMPONENT_SWIZZLE_R;
	mapping.g = VK_COMPONENT_SWIZZLE_G;
	mapping.b = VK_COMPONENT_SWIZZLE_B;
	mapping.a = VK_COMPONENT_SWIZZLE_A;

	switch (p)
	{
	case CoreGraphics::PixelFormat::R16G16B16A16:
	case CoreGraphics::PixelFormat::R16G16B16A16F:
	case CoreGraphics::PixelFormat::R32G32B32A32F:
	case CoreGraphics::PixelFormat::R11G11B10F:
	case CoreGraphics::PixelFormat::A8B8G8R8:
		mapping.r = VK_COMPONENT_SWIZZLE_R;
		mapping.b = VK_COMPONENT_SWIZZLE_B;
		break;
	
	}

	return mapping;
}

//------------------------------------------------------------------------------
/**
*/
VkComponentMapping
VkTypes::AsVkMapping(ILenum p)
{
	VkComponentMapping mapping;
	mapping.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	mapping.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	mapping.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	mapping.a = VK_COMPONENT_SWIZZLE_IDENTITY;

	switch (p)
	{
	case PF_ARGB:
	case PF_RGB:
	case PF_DXT1:				
	case PF_DXT3:				
	case PF_DXT5:				
	case PF_BC7:				
	case PF_DXT1_sRGB:			
	case PF_DXT3_sRGB:			
	case PF_DXT5_sRGB:			
	case PF_BC7_sRGB:			
	case PF_A16R16G16B16:		
	case PF_A16R16B16G16F:		
	case PF_A32R32G32B32F:		
	case PF_R16F:				
	case PF_R32F:				
		break;
	case PF_A16B16G16R16:		
	case PF_A16B16G16R16F:		
	case PF_A32B32G32R32F:
		mapping.r = VK_COMPONENT_SWIZZLE_R;
		mapping.b = VK_COMPONENT_SWIZZLE_B;
		break;
	case PF_G16R16F:
	case PF_G32R32F:
		mapping.g = VK_COMPONENT_SWIZZLE_R;
		mapping.r = VK_COMPONENT_SWIZZLE_G;
	default:
		{
			n_error("VkTypes::AsVkMapping(): invalid pixel swizzle '%d'", p);
		}
	}

	return mapping;
}

//------------------------------------------------------------------------------
/**
*/
VkPipelineStageFlags
VkTypes::AsVkPipelineFlags(const CoreGraphics::BarrierStage dep)
{
	VkPipelineStageFlags flags = 0;
	uint32_t bit;
	for (bit = 1; dep >= bit; bit *= 2)
	{
		if ((dep & bit) == bit) switch ((CoreGraphics::BarrierStage)bit)
		{
		case CoreGraphics::BarrierStage::VertexShader:
			flags |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
			break;
		case CoreGraphics::BarrierStage::HullShader:
			flags |= VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT;
			break;
		case CoreGraphics::BarrierStage::DomainShader:
			flags |= VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;
			break;
		case CoreGraphics::BarrierStage::GeometryShader:
			flags |= VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;
			break;
		case CoreGraphics::BarrierStage::PixelShader:
			flags |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			break;
		case CoreGraphics::BarrierStage::ComputeShader:
			flags |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
			break;
		case CoreGraphics::BarrierStage::VertexInput:
			flags |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
			break;
		case CoreGraphics::BarrierStage::EarlyDepth:
			flags |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			break;
		case CoreGraphics::BarrierStage::LateDepth:
			flags |= VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			break;
		case CoreGraphics::BarrierStage::Transfer:
			flags |= VK_PIPELINE_STAGE_TRANSFER_BIT;
			break;
		case CoreGraphics::BarrierStage::Host:
			flags |= VK_PIPELINE_STAGE_HOST_BIT;
			break;
		case CoreGraphics::BarrierStage::PassOutput:
			flags |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			break;
		case CoreGraphics::BarrierStage::Top:
			flags |= VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			break;
		case CoreGraphics::BarrierStage::Bottom:
			flags |= VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			break;
		}
	}
	return flags;
}

//------------------------------------------------------------------------------
/**
*/
VkAccessFlags
VkTypes::AsVkResourceAccessFlags(const CoreGraphics::BarrierAccess access)
{
	VkAccessFlags flags = 0;
	uint32_t bit;
	for (bit = 1; access >= bit; bit *= 2)
	{
		if ((access & bit) == bit) switch ((CoreGraphics::BarrierAccess)bit)
		{
		case CoreGraphics::BarrierAccess::IndirectRead:
			flags |= VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
			break;
		case CoreGraphics::BarrierAccess::IndexRead:
			flags |= VK_ACCESS_INDEX_READ_BIT;
			break;
		case CoreGraphics::BarrierAccess::VertexRead:
			flags |= VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
			break;
		case CoreGraphics::BarrierAccess::UniformRead:
			flags |= VK_ACCESS_UNIFORM_READ_BIT;
			break;
		case CoreGraphics::BarrierAccess::InputAttachmentRead:
			flags |= VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
			break;
		case CoreGraphics::BarrierAccess::ShaderRead:
			flags |= VK_ACCESS_SHADER_READ_BIT;
			break;
		case CoreGraphics::BarrierAccess::ShaderWrite:
			flags |= VK_ACCESS_SHADER_WRITE_BIT;
			break;
		case CoreGraphics::BarrierAccess::ColorAttachmentRead:
			flags |= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
			break;
		case CoreGraphics::BarrierAccess::ColorAttachmentWrite:
			flags |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			break;
		case CoreGraphics::BarrierAccess::DepthRead:
			flags |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
			break;
		case CoreGraphics::BarrierAccess::DepthWrite:
			flags |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			break;
		case CoreGraphics::BarrierAccess::TransferRead:
			flags |= VK_ACCESS_TRANSFER_READ_BIT;
			break;
		case CoreGraphics::BarrierAccess::TransferWrite:
			flags |= VK_ACCESS_TRANSFER_WRITE_BIT;
			break;
		case CoreGraphics::BarrierAccess::HostRead:
			flags |= VK_ACCESS_HOST_READ_BIT;
			break;
		case CoreGraphics::BarrierAccess::HostWrite:
			flags |= VK_ACCESS_HOST_WRITE_BIT;
			break;
		case CoreGraphics::BarrierAccess::MemoryRead:
			flags |= VK_ACCESS_MEMORY_READ_BIT;
			break;
		case CoreGraphics::BarrierAccess::MemoryWrite:
			flags |= VK_ACCESS_MEMORY_WRITE_BIT;
			break;
		}
	}
	return flags;
}


//------------------------------------------------------------------------------
/**
*/
VkImageAspectFlags
VkTypes::AsVkImageAspectFlags(const CoreGraphicsImageAspect aspect)
{
	VkImageAspectFlags flags = 0;
	uint32_t bit;
	for (bit = 1; aspect >= bit; bit *= 2)
	{
		if ((aspect & bit) == bit) switch ((CoreGraphicsImageAspect)bit)
		{
		case CoreGraphicsImageAspect::ColorBits:
			flags |= VK_IMAGE_ASPECT_COLOR_BIT;
			break;
		case CoreGraphicsImageAspect::DepthBits:
			flags |= VK_IMAGE_ASPECT_DEPTH_BIT;
			break;
		case CoreGraphicsImageAspect::StencilBits:
			flags |= VK_IMAGE_ASPECT_STENCIL_BIT;
			break;
		case CoreGraphicsImageAspect::MetaBits:
			flags |= VK_IMAGE_ASPECT_METADATA_BIT;
			break;
		case CoreGraphicsImageAspect::Plane0Bits:
			flags |= VK_IMAGE_ASPECT_PLANE_0_BIT;
			break;
		case CoreGraphicsImageAspect::Plane1Bits:
			flags |= VK_IMAGE_ASPECT_PLANE_1_BIT;
			break;
		case CoreGraphicsImageAspect::Plane2Bits:
			flags |= VK_IMAGE_ASPECT_PLANE_2_BIT;
			break;
		}
	}
	return flags;
}

//------------------------------------------------------------------------------
/**
*/
VkShaderStageFlags
VkTypes::AsVkShaderVisibility(const CoreGraphicsShaderVisibility vis)
{
	VkShaderStageFlags ret = 0;
	if ((vis & VertexShaderVisibility) == VertexShaderVisibility)		ret |= VK_SHADER_STAGE_VERTEX_BIT;
	if ((vis & HullShaderVisibility) == HullShaderVisibility)			ret |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
	if ((vis & DomainShaderVisibility) == DomainShaderVisibility)		ret |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
	if ((vis & GeometryShaderVisibility) == GeometryShaderVisibility)	ret |= VK_SHADER_STAGE_GEOMETRY_BIT;
	if ((vis & PixelShaderVisibility) == PixelShaderVisibility)			ret |= VK_SHADER_STAGE_FRAGMENT_BIT;
	if ((vis & ComputeShaderVisibility) == ComputeShaderVisibility)		ret |= VK_SHADER_STAGE_COMPUTE_BIT;
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
VkImageLayout
VkTypes::AsVkImageLayout(const CoreGraphicsImageLayout layout)
{
	switch (layout)
	{
		case CoreGraphicsImageLayout::Undefined:					return VK_IMAGE_LAYOUT_UNDEFINED;
		case CoreGraphicsImageLayout::General:						return VK_IMAGE_LAYOUT_GENERAL;
		case CoreGraphicsImageLayout::ColorRenderTexture:			return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		case CoreGraphicsImageLayout::DepthStencilRenderTexture:	return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		case CoreGraphicsImageLayout::DepthStencilRead:				return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		case CoreGraphicsImageLayout::ShaderRead:					return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		case CoreGraphicsImageLayout::TransferSource:				return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		case CoreGraphicsImageLayout::TransferDestination:			return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		case CoreGraphicsImageLayout::Preinitialized:				return VK_IMAGE_LAYOUT_PREINITIALIZED;
		case CoreGraphicsImageLayout::Present:						return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	}
	return VK_IMAGE_LAYOUT_UNDEFINED;
}

//------------------------------------------------------------------------------
/**
*/
VkPrimitiveTopology
VkTypes::AsVkPrimitiveType(CoreGraphics::PrimitiveTopology::Code t)
{
	switch (t)
	{
	case CoreGraphics::PrimitiveTopology::PointList: return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
	case CoreGraphics::PrimitiveTopology::LineList: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
	case CoreGraphics::PrimitiveTopology::LineStrip: return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
	case CoreGraphics::PrimitiveTopology::TriangleList: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	case CoreGraphics::PrimitiveTopology::TriangleStrip: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	case CoreGraphics::PrimitiveTopology::PatchList: return VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
	}
	return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
}

//------------------------------------------------------------------------------
/**
*/
uint32_t
VkTypes::AsByteSize(uint32_t semantic)
{
	switch (semantic)
	{
	case VertexComponent::Position:
		return sizeof(uint32_t) * 4;
	case VertexComponent::Normal:
		return sizeof(uint32_t) * 3;
	case VertexComponent::Tangent:
		return sizeof(uint32_t) * 3;
	case VertexComponent::Binormal:
		return sizeof(uint32_t) * 3;
	case VertexComponent::TexCoord1:
		return sizeof(uint32_t) * 2;
	case VertexComponent::Color:
		return sizeof(uint32_t);
	case VertexComponent::SkinWeights:
		return sizeof(uint32_t) * 4;
	case VertexComponent::SkinJIndices:
		return sizeof(uint32_t) * 4;
	default:
		n_error("Unknown vertex input semantic!");
		return 0;
	}
}

//------------------------------------------------------------------------------
/**
*/
uint32_t
VkTypes::AsVkSize(CoreGraphics::VertexComponent::Format f)
{
	switch (f)
	{
	case VertexComponent::Float:    return 4;
	case VertexComponent::Float2:   return 8;
	case VertexComponent::Float3:   return 12;
	case VertexComponent::Float4:   return 16;
	case VertexComponent::UByte4:   return 4;
	case VertexComponent::Byte4:    return 4;
	case VertexComponent::Short2:   return 4;
	case VertexComponent::Short4:   return 8;
	case VertexComponent::UByte4N:  return 4;
	case VertexComponent::Byte4N:   return 4;
	case VertexComponent::Short2N:  return 4;
	case VertexComponent::Short4N:  return 8;
	default:
		n_error("OGL4Types::AsOGL4Size(): invalid input parameter!");
		return 1;
	}
}

//------------------------------------------------------------------------------
/**
*/
uint32_t
VkTypes::AsVkNumComponents(CoreGraphics::VertexComponent::Format f)
{
	switch (f)
	{
	case VertexComponent::Float:    return 1;
	case VertexComponent::Float2:   return 2;
	case VertexComponent::Float3:   return 3;
	case VertexComponent::Float4:   return 4;
	case VertexComponent::UByte4:   return 4;
	case VertexComponent::Byte4:    return 4;
	case VertexComponent::Short2:   return 2;
	case VertexComponent::Short4:   return 4;
	case VertexComponent::UByte4N:  return 4;
	case VertexComponent::Byte4N:   return 4;
	case VertexComponent::Short2N:  return 2;
	case VertexComponent::Short4N:  return 4;
	default:
		n_error("OGL4Types::AsOGL4Size(): invalid input parameter!");
		return 1;
	}
}

//------------------------------------------------------------------------------
/**
*/
VkFormat
VkTypes::AsVkVertexType(CoreGraphics::VertexComponent::Format f)
{
	switch (f)
	{
	case VertexComponent::Float:    return VK_FORMAT_R32_SFLOAT;
	case VertexComponent::Float2:   return VK_FORMAT_R32G32_SFLOAT;
	case VertexComponent::Float3:   return VK_FORMAT_R32G32B32_SFLOAT;
	case VertexComponent::Float4:   return VK_FORMAT_R32G32B32A32_SFLOAT;
	case VertexComponent::UByte4:   return VK_FORMAT_R8G8B8A8_UINT;
	case VertexComponent::Byte4:	return VK_FORMAT_R8G8B8A8_SINT;
	case VertexComponent::Short2:   return VK_FORMAT_R16G16_SINT;
	case VertexComponent::Short4:   return VK_FORMAT_R16G16B16A16_SINT;
	case VertexComponent::UByte4N:  return VK_FORMAT_R8G8B8A8_UNORM;
	case VertexComponent::Byte4N:	return VK_FORMAT_R8G8B8A8_SNORM;
	case VertexComponent::Short2N:  return VK_FORMAT_R16G16_SNORM;
	case VertexComponent::Short4N:  return VK_FORMAT_R16G16B16A16_SNORM;
	default:
		n_error("OGL4Types::AsOGL4SymbolicType(): invalid input parameter!");
		return VK_FORMAT_R32G32B32A32_SFLOAT;
	}
}

} // namespace Vulkan