//------------------------------------------------------------------------------
// vktypes.cc
// (C) 2016-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "vktypes.h"
#include "vkloader.h"
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
    case PixelFormat::B8G8R8A8:         return VK_FORMAT_B8G8R8A8_UNORM;
    case PixelFormat::R8G8B8:           return VK_FORMAT_R8G8B8_UNORM;
    case PixelFormat::R5G6B5:           return VK_FORMAT_R5G6B5_UNORM_PACK16;
    case PixelFormat::SRGBA8:           return VK_FORMAT_R8G8B8A8_SRGB;
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
    case PixelFormat::BC4:              return VK_FORMAT_BC4_UNORM_BLOCK;
    case PixelFormat::BC5:              return VK_FORMAT_BC5_UNORM_BLOCK;
    case PixelFormat::BC7:              return VK_FORMAT_BC7_UNORM_BLOCK;
    case PixelFormat::BC7sRGB:          return VK_FORMAT_BC7_SRGB_BLOCK;
    case PixelFormat::R8:               return VK_FORMAT_R8_UNORM;
    case PixelFormat::R16F:             return VK_FORMAT_R16_SFLOAT;
    case PixelFormat::R16:              return VK_FORMAT_R16_UINT;
    case PixelFormat::R16G16F:          return VK_FORMAT_R16G16_SFLOAT;
    case PixelFormat::R16G16:           return VK_FORMAT_R16G16_UINT;
    case PixelFormat::R16G16B16A16F:    return VK_FORMAT_R16G16B16A16_SFLOAT;
    case PixelFormat::R16G16B16A16:     return VK_FORMAT_R16G16B16A16_UINT;
    case PixelFormat::R11G11B10F:       return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
    case PixelFormat::R32F:             return VK_FORMAT_R32_SFLOAT;
    case PixelFormat::R32:              return VK_FORMAT_R32_UINT;
    case PixelFormat::R32G32F:          return VK_FORMAT_R32G32_SFLOAT;
    case PixelFormat::R32G32:           return VK_FORMAT_R32G32_UINT;
    case PixelFormat::R32G32B32A32F:    return VK_FORMAT_R32G32B32A32_SFLOAT;
    case PixelFormat::R32G32B32A32:     return VK_FORMAT_R32G32B32A32_UINT;
    case PixelFormat::R32G32B32F:       return VK_FORMAT_R32G32B32_SFLOAT;
    case PixelFormat::R32G32B32:        return VK_FORMAT_R32G32B32_UINT;
    case PixelFormat::R10G10B10A2:      return VK_FORMAT_A2B10G10R10_UINT_PACK32;
    case PixelFormat::D32S8:            return VK_FORMAT_D32_SFLOAT_S8_UINT;
    case PixelFormat::D24X8:            return VK_FORMAT_X8_D24_UNORM_PACK32;
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
bool 
VkTypes::IsDepthFormat(CoreGraphics::PixelFormat::Code p)
{
    switch (p)
    {
    case PixelFormat::D32S8:
    case PixelFormat::D24X8:
    case PixelFormat::D24S8:
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
VkFormat
VkTypes::AsVkFormat(ILenum p)
{
    switch (p)
    {
    case PF_RGBA:               return VK_FORMAT_R8G8B8A8_UNORM;
    case PF_RGB:                return VK_FORMAT_R8G8B8_UNORM;
    case PF_SRGB:               return VK_FORMAT_R8G8B8A8_SRGB;
    case PF_BGRA:               return VK_FORMAT_B8G8R8A8_UNORM;
    case PF_BGR:                return VK_FORMAT_B8G8R8_UNORM;
    case PF_DXT1:               return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
    case PF_DXT3:               return VK_FORMAT_BC2_UNORM_BLOCK;
    case PF_DXT5:               return VK_FORMAT_BC3_UNORM_BLOCK;
    case PF_BC7:                return VK_FORMAT_BC7_UNORM_BLOCK;
    case PF_DXT1_sRGB:          return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
    case PF_DXT3_sRGB:          return VK_FORMAT_BC2_SRGB_BLOCK;
    case PF_DXT5_sRGB:          return VK_FORMAT_BC3_SRGB_BLOCK;
    case PF_BC7_sRGB:           return VK_FORMAT_BC7_SRGB_BLOCK;
    case PF_3DC:                return VK_FORMAT_BC5_UNORM_BLOCK;
    case PF_R8:                 return VK_FORMAT_R8_UNORM;
    case PF_R16F:               return VK_FORMAT_R16_SFLOAT;
    case PF_R16:                return VK_FORMAT_R16_UINT;
    case PF_R32F:               return VK_FORMAT_R32_SFLOAT;
    case PF_R32:                return VK_FORMAT_R32_UINT;
    case PF_R16G16F:            return VK_FORMAT_R16G16_SFLOAT;
    case PF_R16G16:             return VK_FORMAT_R16G16_UINT;
    case PF_R32G32F:            return VK_FORMAT_R32G32_SFLOAT;
    case PF_R32G32:             return VK_FORMAT_R32G32_UINT;
    case PF_R16G16B16A16:       return VK_FORMAT_R16G16B16A16_UINT;
    case PF_R16G16B16A16F:      return VK_FORMAT_R16G16B16A16_SFLOAT;
    case PF_R32G32B32:          return VK_FORMAT_R32G32B32_UINT;
    case PF_R32G32B32F:         return VK_FORMAT_R32G32B32_SFLOAT;
    case PF_R32G32B32A32:       return VK_FORMAT_R32G32B32A32_UINT;
    case PF_R32G32B32A32F:      return VK_FORMAT_R32G32B32A32_SFLOAT;
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

        case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:    return IL_DXT1;
        case VK_FORMAT_BC2_UNORM_BLOCK:         return IL_DXT3;
        case VK_FORMAT_BC3_UNORM_BLOCK:         return IL_DXT5;
        case VK_FORMAT_BC7_UNORM_BLOCK:         return IL_BPTC;
        case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:     return IL_DXT1A_sRGB;
        case VK_FORMAT_BC2_SRGB_BLOCK:          return IL_DXT3_sRGB;
        case VK_FORMAT_BC3_SRGB_BLOCK:          return IL_DXT5_sRGB;
        case VK_FORMAT_BC7_SRGB_BLOCK:          return IL_BPTC_sRGB;
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
    case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:    return true;
    case VK_FORMAT_BC2_UNORM_BLOCK:         return true;
    case VK_FORMAT_BC3_UNORM_BLOCK:         return true;
    case VK_FORMAT_BC4_UNORM_BLOCK:         return true;
    case VK_FORMAT_BC5_UNORM_BLOCK:         return true;
    case VK_FORMAT_BC7_UNORM_BLOCK:         return true;
    case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:     return true;
    case VK_FORMAT_BC2_SRGB_BLOCK:          return true;
    case VK_FORMAT_BC3_SRGB_BLOCK:          return true;
    case VK_FORMAT_BC7_SRGB_BLOCK:          return true;
    default:                                return false;
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
    case PixelFormat::R8G8B8:           return { 1, 1 };
    case PixelFormat::R5G6B5:           return { 1, 1 };
    case PixelFormat::SRGBA8:           return { 1, 1 };
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
    case PixelFormat::BC4:              return { 4, 4 };
    case PixelFormat::BC5:              return { 4, 4 };
    case PixelFormat::BC7:              return { 4, 4 };
    case PixelFormat::BC7sRGB:          return { 4, 4 };
    case PixelFormat::R8:               return { 1, 1 };
    case PixelFormat::R16F:             return { 1, 1 };
    case PixelFormat::R16:              return { 1, 1 };
    case PixelFormat::R16G16F:          return { 1, 1 };
    case PixelFormat::R16G16:           return { 1, 1 };
    case PixelFormat::R16G16B16A16F:    return { 1, 1 };
    case PixelFormat::R16G16B16A16:     return { 1, 1 };
    case PixelFormat::R11G11B10F:       return { 1, 1 };
    case PixelFormat::R32F:             return { 1, 1 };
    case PixelFormat::R32:              return { 1, 1 };
    case PixelFormat::R32G32F:          return { 1, 1 };
    case PixelFormat::R32G32:           return { 1, 1 };
    case PixelFormat::R32G32B32A32F:    return { 1, 1 };
    case PixelFormat::R32G32B32A32:     return { 1, 1 };
    case PixelFormat::R32G32B32F:       return { 1, 1 };
    case PixelFormat::R32G32B32:        return { 1, 1 };
    case PixelFormat::R10G10B10A2:      return { 1, 1 };
    case PixelFormat::D32S8:            return { 1, 1 };
    case PixelFormat::D24X8:            return { 1, 1 };
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
    case VK_FORMAT_BC1_RGB_UNORM_BLOCK:     return {4, 4};
    case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:    return {4, 4};
    case VK_FORMAT_BC2_UNORM_BLOCK:         return {4, 4};
    case VK_FORMAT_BC3_UNORM_BLOCK:         return {4, 4};
    case VK_FORMAT_BC1_RGB_SRGB_BLOCK:      return {4, 4};
    case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:     return {4, 4};
    case VK_FORMAT_BC2_SRGB_BLOCK:          return {4, 4};
    case VK_FORMAT_BC3_SRGB_BLOCK:          return {4, 4};
    case VK_FORMAT_BC4_UNORM_BLOCK:         return {4, 4};
    case VK_FORMAT_BC5_UNORM_BLOCK:         return {4, 4};
    case VK_FORMAT_BC7_UNORM_BLOCK:         return {4, 4};
    case VK_FORMAT_BC7_SRGB_BLOCK:          return {4, 4};
    case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:    return {4, 4};
    case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:     return {4, 4};
    case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:    return {5, 4};
    case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:     return {5, 4};
    case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:    return {5, 5};
    case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:     return {5, 5};
    case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:    return {6, 5};
    case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:     return {6, 5};
    case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:    return {6, 6};
    case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:     return {6, 6};
    case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:    return {8, 5};
    case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:     return {8, 5};
    case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:    return {8, 6};
    case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:     return {8, 6};
    case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:    return {8, 8};
    case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:     return {8, 8};
    case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:   return {10, 5};
    case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:    return {10, 5};
    case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:   return {10, 6};
    case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:    return {10, 6};
    case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:   return {10, 8};
    case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:    return {10, 8};
    case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:  return {10, 10};
    case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:   return {10, 10};
    case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:  return {12, 10};
    case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:   return {12, 10};
    case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:  return {12, 12};
    case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:   return {12, 12};
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
    case VK_FORMAT_BC1_RGB_UNORM_BLOCK:     return VK_FORMAT_R8G8B8_UNORM;
    case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:    return VK_FORMAT_R8G8B8A8_UNORM;
    case VK_FORMAT_BC2_UNORM_BLOCK:         return VK_FORMAT_R8G8B8A8_UNORM;
    case VK_FORMAT_BC3_UNORM_BLOCK:         return VK_FORMAT_R8G8B8A8_UNORM;
    case VK_FORMAT_BC1_RGB_SRGB_BLOCK:      return VK_FORMAT_R8G8B8_UNORM;
    case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:     return VK_FORMAT_R8G8B8A8_UNORM;
    case VK_FORMAT_BC2_SRGB_BLOCK:          return VK_FORMAT_R8G8B8A8_UNORM;
    case VK_FORMAT_BC3_SRGB_BLOCK:          return VK_FORMAT_R8G8B8A8_UNORM;
    case VK_FORMAT_BC4_UNORM_BLOCK:         return VK_FORMAT_R8_UNORM;
    case VK_FORMAT_BC5_UNORM_BLOCK:         return VK_FORMAT_R8G8B8A8_UNORM;
    case VK_FORMAT_BC7_UNORM_BLOCK:         return VK_FORMAT_R8G8B8A8_UNORM;
    case VK_FORMAT_BC7_SRGB_BLOCK:          return VK_FORMAT_R8G8B8A8_UNORM;
    case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:    return VK_FORMAT_R8G8B8A8_UNORM;
    case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:     return VK_FORMAT_R8G8B8A8_UNORM;
    case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:    return VK_FORMAT_R8G8B8A8_UNORM;
    case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:     return VK_FORMAT_R8G8B8A8_UNORM;
    case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:    return VK_FORMAT_R8G8B8A8_UNORM;
    case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:     return VK_FORMAT_R8G8B8A8_UNORM;
    case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:    return VK_FORMAT_R8G8B8A8_UNORM;
    case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:     return VK_FORMAT_R8G8B8A8_UNORM;
    case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:    return VK_FORMAT_R8G8B8A8_UNORM;
    case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:     return VK_FORMAT_R8G8B8A8_UNORM;
    case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:    return VK_FORMAT_R8G8B8A8_UNORM;
    case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:     return VK_FORMAT_R8G8B8A8_UNORM;
    case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:    return VK_FORMAT_R8G8B8A8_UNORM;
    case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:     return VK_FORMAT_R8G8B8A8_UNORM;
    case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:    return VK_FORMAT_R8G8B8A8_UNORM;
    case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:     return VK_FORMAT_R8G8B8A8_UNORM;
    case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:   return VK_FORMAT_R8G8B8A8_UNORM;
    case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:    return VK_FORMAT_R8G8B8A8_UNORM;
    case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:   return VK_FORMAT_R8G8B8A8_UNORM;
    case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:    return VK_FORMAT_R8G8B8A8_UNORM;
    case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:   return VK_FORMAT_R8G8B8A8_UNORM;
    case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:    return VK_FORMAT_R8G8B8A8_UNORM;
    case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:  return VK_FORMAT_R8G8B8A8_UNORM;
    case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:   return VK_FORMAT_R8G8B8A8_UNORM;
    case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:  return VK_FORMAT_R8G8B8A8_UNORM;
    case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:   return VK_FORMAT_R8G8B8A8_UNORM;
    case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:  return VK_FORMAT_R8G8B8A8_UNORM;
    case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:   return VK_FORMAT_R8G8B8A8_UNORM;
    default:
        return fmt;
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
        case PixelFormat::R8G8B8:           return VK_FORMAT_R8G8B8_UINT;
        case PixelFormat::R8:               return VK_FORMAT_R8_UINT;
        case PixelFormat::R16F:             return VK_FORMAT_R16_SFLOAT;
        case PixelFormat::R16:              return VK_FORMAT_R16_UINT;
        case PixelFormat::R16G16F:          return VK_FORMAT_R16G16_SFLOAT;
        case PixelFormat::R16G16:           return VK_FORMAT_R16G16_UINT;
        case PixelFormat::R16G16B16A16F:    return VK_FORMAT_R16G16B16A16_SFLOAT;
        case PixelFormat::R16G16B16A16:     return VK_FORMAT_R16G16B16A16_UINT;
        case PixelFormat::R32F:             return VK_FORMAT_R32_SFLOAT;
        case PixelFormat::R32:              return VK_FORMAT_R32_UINT;
        case PixelFormat::R32G32F:          return VK_FORMAT_R32G32_SFLOAT;
        case PixelFormat::R32G32:           return VK_FORMAT_R32G32_UINT;
        case PixelFormat::R32G32B32A32F:    return VK_FORMAT_R32G32B32A32_SFLOAT;
        case PixelFormat::R32G32B32A32:     return VK_FORMAT_R32G32B32A32_UINT;
        case PixelFormat::R32G32B32F:       return VK_FORMAT_R32G32B32_SFLOAT;
        case PixelFormat::R32G32B32:        return VK_FORMAT_R32G32B32_UINT;
        case PixelFormat::R11G11B10F:       return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
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
VkSampleCountFlagBits
VkTypes::AsVkSampleFlags(const SizeT samples)
{
    switch (samples)
    {
    case 1: return VK_SAMPLE_COUNT_1_BIT;
    case 2: return VK_SAMPLE_COUNT_2_BIT;
    case 4: return VK_SAMPLE_COUNT_4_BIT;
    case 8: return VK_SAMPLE_COUNT_8_BIT;
    case 16: return VK_SAMPLE_COUNT_16_BIT;
    case 32: return VK_SAMPLE_COUNT_32_BIT;
    case 64: return VK_SAMPLE_COUNT_64_BIT;
    default:
        n_error("Unknown sample bits '%d'", samples);
        return VK_SAMPLE_COUNT_1_BIT;
    }
}

//------------------------------------------------------------------------------
/**
*/
VkImageType 
VkTypes::AsVkImageType(CoreGraphics::TextureType type)
{
    switch (type)
    {
        case Texture1D:
            return VK_IMAGE_TYPE_1D;
        case Texture2D:
            return VK_IMAGE_TYPE_2D;
        case Texture3D:
            return VK_IMAGE_TYPE_3D;
        case TextureCube:
            return VK_IMAGE_TYPE_2D;
        case Texture1DArray:
            return VK_IMAGE_TYPE_1D;
        case Texture2DArray:
            return VK_IMAGE_TYPE_2D;
        case TextureCubeArray:
            return VK_IMAGE_TYPE_2D;
    }
    n_error("Should not happen");
    return VK_IMAGE_TYPE_MAX_ENUM;
}

//------------------------------------------------------------------------------
/**
*/
VkImageViewType 
VkTypes::AsVkImageViewType(CoreGraphics::TextureType type)
{
    switch (type)
    {
    case Texture1D:
        return VK_IMAGE_VIEW_TYPE_1D;
    case Texture2D:
        return VK_IMAGE_VIEW_TYPE_2D;
    case Texture3D:
        return VK_IMAGE_VIEW_TYPE_3D;
    case TextureCube:
        return VK_IMAGE_VIEW_TYPE_CUBE;
    case Texture1DArray:
        return VK_IMAGE_VIEW_TYPE_1D_ARRAY;
    case Texture2DArray:
        return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    case TextureCubeArray:
        return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
    }
    n_error("Should not happen");
    return VK_IMAGE_VIEW_TYPE_MAX_ENUM;
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::PixelFormat::Code
VkTypes::AsNebulaPixelFormat(VkFormat f)
{
    
    switch (f)
    {
    case VK_FORMAT_R8G8B8A8_UINT:                   return PixelFormat::R8G8B8A8;
    case VK_FORMAT_R8G8B8_UINT:                     return PixelFormat::R8G8B8;
    case VK_FORMAT_R8G8B8A8_UNORM:                  return PixelFormat::R8G8B8A8;
    case VK_FORMAT_R8G8B8_UNORM:                    return PixelFormat::R8G8B8;
    case VK_FORMAT_B8G8R8_UNORM:                    return PixelFormat::R8G8B8;
    case VK_FORMAT_B8G8R8A8_UNORM:                  return PixelFormat::R8G8B8A8;
    case VK_FORMAT_R5G6B5_UNORM_PACK16:             return PixelFormat::R5G6B5;
    case VK_FORMAT_R8G8B8A8_SRGB:                   return PixelFormat::SRGBA8;
    case VK_FORMAT_B8G8R8A8_SRGB:                   return PixelFormat::SRGBA8;
    case VK_FORMAT_R5G5B5A1_UNORM_PACK16:           return PixelFormat::R5G5B5A1;
    case VK_FORMAT_R4G4B4A4_UNORM_PACK16:           return PixelFormat::R4G4B4A4;
    case VK_FORMAT_BC1_RGB_UNORM_BLOCK:             return PixelFormat::DXT1;
    case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:            return PixelFormat::DXT1A;
    case VK_FORMAT_BC2_UNORM_BLOCK:                 return PixelFormat::DXT3;
    case VK_FORMAT_BC3_UNORM_BLOCK:                 return PixelFormat::DXT5;
    case VK_FORMAT_BC1_RGB_SRGB_BLOCK:              return PixelFormat::DXT1sRGB;
    case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:             return PixelFormat::DXT1AsRGB;
    case VK_FORMAT_BC2_SRGB_BLOCK:                  return PixelFormat::DXT3sRGB;
    case VK_FORMAT_BC3_SRGB_BLOCK:                  return PixelFormat::DXT5sRGB;
    case VK_FORMAT_BC4_UNORM_BLOCK:                 return PixelFormat::BC4;
    case VK_FORMAT_BC5_UNORM_BLOCK:                 return PixelFormat::BC5;
    case VK_FORMAT_BC7_UNORM_BLOCK:                 return PixelFormat::BC7;
    case VK_FORMAT_BC7_SRGB_BLOCK:                  return PixelFormat::BC7sRGB;
    case VK_FORMAT_R16_SFLOAT:                      return PixelFormat::R16F;
    case VK_FORMAT_R16G16_SFLOAT:                   return PixelFormat::R16G16F;
    case VK_FORMAT_R16G16B16A16_SFLOAT:             return PixelFormat::R16G16B16A16F;
    case VK_FORMAT_R16G16B16A16_UINT:               return PixelFormat::R16G16B16A16;
    case VK_FORMAT_R16G16B16A16_UNORM:              return PixelFormat::R16G16B16A16;
    case VK_FORMAT_B10G11R11_UFLOAT_PACK32:         return PixelFormat::R11G11B10F;
    case VK_FORMAT_R32_SFLOAT:                      return PixelFormat::R32F;
    case VK_FORMAT_R32G32_SFLOAT:                   return PixelFormat::R32G32F;
    case VK_FORMAT_R32G32B32A32_SFLOAT:             return PixelFormat::R32G32B32A32F;
    case VK_FORMAT_R32G32B32_SFLOAT:                return PixelFormat::R32G32B32F;
    case VK_FORMAT_R8_UNORM:                        return PixelFormat::R8;
    case VK_FORMAT_A2B10G10R10_UINT_PACK32:         return PixelFormat::R10G10B10A2;
    case VK_FORMAT_R16G16_UINT:                     return PixelFormat::R16G16;
    case VK_FORMAT_X8_D24_UNORM_PACK32:             return PixelFormat::D24X8;
    case VK_FORMAT_D24_UNORM_S8_UINT:               return PixelFormat::D24S8;
    case VK_FORMAT_D32_SFLOAT_S8_UINT:              return PixelFormat::D32S8;
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
    mapping.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    mapping.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    mapping.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    mapping.a = VK_COMPONENT_SWIZZLE_IDENTITY;

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
    case PF_RGBA:
    case PF_RGB:
    case PF_DXT1:               
    case PF_DXT3:               
    case PF_DXT5:               
    case PF_BC7:                
    case PF_DXT1_sRGB:          
    case PF_DXT3_sRGB:          
    case PF_DXT5_sRGB:          
    case PF_BC7_sRGB:           
    case PF_3DC:            
    case PF_R8:
    case PF_R16F:
    case PF_R16:
    case PF_R32F:
    case PF_R32:
    case PF_R16G16F:
    case PF_R16G16:
    case PF_R32G32F:
    case PF_R32G32:
    case PF_R16G16B16A16:
    case PF_R16G16B16A16F:
    case PF_R32G32B32:
    case PF_R32G32B32F:
    case PF_R32G32B32A32:
    case PF_R32G32B32A32F:
        break;

    case PF_BGRA:
    case PF_BGR:
        mapping.r = VK_COMPONENT_SWIZZLE_B;
        mapping.b = VK_COMPONENT_SWIZZLE_R;
        break;
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
VkImageAspectFlags
VkTypes::AsVkImageAspectFlags(const CoreGraphics::ImageAspect aspect)
{
    VkImageAspectFlags flags = 0;
    uint32_t bit;
    for (bit = 1; aspect >= bit; bit *= 2)
    {
        if ((aspect & bit) == bit) switch ((CoreGraphics::ImageAspect)bit)
        {
        case CoreGraphics::ImageAspect::ColorBits:
            flags |= VK_IMAGE_ASPECT_COLOR_BIT;
            break;
        case CoreGraphics::ImageAspect::DepthBits:
            flags |= VK_IMAGE_ASPECT_DEPTH_BIT;
            break;
        case CoreGraphics::ImageAspect::StencilBits:
            flags |= VK_IMAGE_ASPECT_STENCIL_BIT;
            break;
        case CoreGraphics::ImageAspect::MetaBits:
            flags |= VK_IMAGE_ASPECT_METADATA_BIT;
            break;
        case CoreGraphics::ImageAspect::Plane0Bits:
            flags |= VK_IMAGE_ASPECT_PLANE_0_BIT;
            break;
        case CoreGraphics::ImageAspect::Plane1Bits:
            flags |= VK_IMAGE_ASPECT_PLANE_1_BIT;
            break;
        case CoreGraphics::ImageAspect::Plane2Bits:
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
VkTypes::AsVkShaderVisibility(const CoreGraphics::ShaderVisibility vis)
{
    VkShaderStageFlags ret = 0;
    if ((vis & VertexShaderVisibility) == VertexShaderVisibility)       ret |= VK_SHADER_STAGE_VERTEX_BIT;
    if ((vis & HullShaderVisibility) == HullShaderVisibility)           ret |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
    if ((vis & DomainShaderVisibility) == DomainShaderVisibility)       ret |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
    if ((vis & GeometryShaderVisibility) == GeometryShaderVisibility)   ret |= VK_SHADER_STAGE_GEOMETRY_BIT;
    if ((vis & PixelShaderVisibility) == PixelShaderVisibility)         ret |= VK_SHADER_STAGE_FRAGMENT_BIT;
    if ((vis & ComputeShaderVisibility) == ComputeShaderVisibility)     ret |= VK_SHADER_STAGE_COMPUTE_BIT;
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
VkImageLayout
VkTypes::AsVkImageLayout(const CoreGraphics::ImageLayout layout)
{
    switch (layout)
    {
        case CoreGraphics::ImageLayout::Undefined:                  return VK_IMAGE_LAYOUT_UNDEFINED;
        case CoreGraphics::ImageLayout::General:                    return VK_IMAGE_LAYOUT_GENERAL;
        case CoreGraphics::ImageLayout::ColorRenderTexture:         return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        case CoreGraphics::ImageLayout::DepthStencilRenderTexture:  return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        case CoreGraphics::ImageLayout::DepthStencilRead:           return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        case CoreGraphics::ImageLayout::ShaderRead:                 return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        case CoreGraphics::ImageLayout::TransferSource:             return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        case CoreGraphics::ImageLayout::TransferDestination:        return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        case CoreGraphics::ImageLayout::Preinitialized:             return VK_IMAGE_LAYOUT_PREINITIALIZED;
        case CoreGraphics::ImageLayout::Present:                    return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    }
    return VK_IMAGE_LAYOUT_UNDEFINED;
}

//------------------------------------------------------------------------------
/**
*/
VkPipelineStageFlags
VkTypes::AsVkPipelineStage(const CoreGraphics::PipelineStage stage)
{
    switch (stage)
    {
        case CoreGraphics::PipelineStage::Top:
            return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        case CoreGraphics::PipelineStage::Bottom:
            return VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        case CoreGraphics::PipelineStage::Indirect:
            return VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
        case CoreGraphics::PipelineStage::Index:
            return VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
        case CoreGraphics::PipelineStage::Vertex:
            return VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
        case CoreGraphics::PipelineStage::InputAttachment:
            return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        case CoreGraphics::PipelineStage::GraphicsShadersRead:
        case CoreGraphics::PipelineStage::GraphicsShadersWrite:
            return VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
        case CoreGraphics::PipelineStage::AllShadersRead:
        case CoreGraphics::PipelineStage::AllShadersWrite:
        case CoreGraphics::PipelineStage::Uniform:
            return VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        case CoreGraphics::PipelineStage::VertexShaderRead:
        case CoreGraphics::PipelineStage::VertexShaderWrite:
            return VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
        case CoreGraphics::PipelineStage::HullShaderRead:
        case CoreGraphics::PipelineStage::HullShaderWrite:
            return VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT;
        case CoreGraphics::PipelineStage::DomainShaderRead:
        case CoreGraphics::PipelineStage::DomainShaderWrite:
            return VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;
        case CoreGraphics::PipelineStage::GeometryShaderRead:
        case CoreGraphics::PipelineStage::GeometryShaderWrite:
            return VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;
        case CoreGraphics::PipelineStage::PixelShaderRead:
        case CoreGraphics::PipelineStage::PixelShaderWrite:
            return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        case CoreGraphics::PipelineStage::ComputeShaderRead:
        case CoreGraphics::PipelineStage::ComputeShaderWrite:
            return VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        case CoreGraphics::PipelineStage::ColorRead:
        case CoreGraphics::PipelineStage::ColorWrite:
            return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        case CoreGraphics::PipelineStage::DepthStencilRead:
            return VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        case CoreGraphics::PipelineStage::DepthStencilWrite:
            return VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        case CoreGraphics::PipelineStage::TransferRead:
        case CoreGraphics::PipelineStage::TransferWrite:
            return VK_PIPELINE_STAGE_TRANSFER_BIT;
        case CoreGraphics::PipelineStage::HostRead:
        case CoreGraphics::PipelineStage::HostWrite:
            return VK_PIPELINE_STAGE_HOST_BIT;
        case CoreGraphics::PipelineStage::MemoryRead:
        case CoreGraphics::PipelineStage::MemoryWrite:
            return VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        case CoreGraphics::PipelineStage::ImageInitial:
            return VK_PIPELINE_STAGE_HOST_BIT;
        case CoreGraphics::PipelineStage::Present:
            return VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    return VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
}

//------------------------------------------------------------------------------
/**
*/
VkAccessFlags
VkTypes::AsVkAccessFlags(const CoreGraphics::PipelineStage stage)
{
    switch (stage)
    {
        case CoreGraphics::PipelineStage::Top:
            return VK_ACCESS_MEMORY_READ_BIT;
        case CoreGraphics::PipelineStage::Bottom:
            return VK_ACCESS_MEMORY_WRITE_BIT;
        case CoreGraphics::PipelineStage::Indirect:
            return VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
        case CoreGraphics::PipelineStage::Index:
            return VK_ACCESS_INDEX_READ_BIT;
        case CoreGraphics::PipelineStage::Vertex:
            return VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
        case CoreGraphics::PipelineStage::InputAttachment:
            return VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
        case CoreGraphics::PipelineStage::Uniform:
            return VK_ACCESS_UNIFORM_READ_BIT;
        case CoreGraphics::PipelineStage::VertexShaderRead:
        case CoreGraphics::PipelineStage::HullShaderRead:
        case CoreGraphics::PipelineStage::DomainShaderRead:
        case CoreGraphics::PipelineStage::GeometryShaderRead:
        case CoreGraphics::PipelineStage::PixelShaderRead:
        case CoreGraphics::PipelineStage::GraphicsShadersRead:
        case CoreGraphics::PipelineStage::ComputeShaderRead:
        case CoreGraphics::PipelineStage::AllShadersRead:
            return VK_ACCESS_SHADER_READ_BIT;
        case CoreGraphics::PipelineStage::VertexShaderWrite:
        case CoreGraphics::PipelineStage::HullShaderWrite:
        case CoreGraphics::PipelineStage::DomainShaderWrite:
        case CoreGraphics::PipelineStage::GeometryShaderWrite:
        case CoreGraphics::PipelineStage::PixelShaderWrite:
        case CoreGraphics::PipelineStage::GraphicsShadersWrite:
        case CoreGraphics::PipelineStage::ComputeShaderWrite:
        case CoreGraphics::PipelineStage::AllShadersWrite:
            return VK_ACCESS_SHADER_WRITE_BIT;
        case CoreGraphics::PipelineStage::ColorRead:
            return VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
        case CoreGraphics::PipelineStage::ColorWrite:
            return VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        case CoreGraphics::PipelineStage::DepthStencilRead:
            return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
        case CoreGraphics::PipelineStage::DepthStencilWrite:
            return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        case CoreGraphics::PipelineStage::TransferRead:
            return VK_ACCESS_TRANSFER_READ_BIT;
        case CoreGraphics::PipelineStage::TransferWrite:
            return VK_ACCESS_TRANSFER_WRITE_BIT;
        case CoreGraphics::PipelineStage::HostRead:
            return VK_ACCESS_HOST_READ_BIT;
        case CoreGraphics::PipelineStage::HostWrite:
            return VK_ACCESS_HOST_WRITE_BIT;
        case CoreGraphics::PipelineStage::MemoryRead:
            return VK_ACCESS_MEMORY_READ_BIT;
        case CoreGraphics::PipelineStage::MemoryWrite:
            return VK_ACCESS_MEMORY_WRITE_BIT;
        case CoreGraphics::PipelineStage::ImageInitial:
            return VK_ACCESS_HOST_WRITE_BIT;
        case CoreGraphics::PipelineStage::Present:
            return VK_ACCESS_TRANSFER_READ_BIT;
    }
    return VK_ACCESS_MEMORY_WRITE_BIT;
}

//------------------------------------------------------------------------------
/**
*/
VkImageLayout
VkTypes::AsVkImageLayout(const CoreGraphics::PipelineStage stage, bool depthStencil)
{
    switch (stage)
    {
        case CoreGraphics::PipelineStage::Top:
        case CoreGraphics::PipelineStage::Bottom:
        case CoreGraphics::PipelineStage::Indirect:
        case CoreGraphics::PipelineStage::Index:
        case CoreGraphics::PipelineStage::Vertex:
        case CoreGraphics::PipelineStage::InputAttachment:
        case CoreGraphics::PipelineStage::Uniform:
        case CoreGraphics::PipelineStage::VertexShaderRead:
        case CoreGraphics::PipelineStage::HullShaderRead:
        case CoreGraphics::PipelineStage::DomainShaderRead:
        case CoreGraphics::PipelineStage::GeometryShaderRead:
        case CoreGraphics::PipelineStage::PixelShaderRead:
        case CoreGraphics::PipelineStage::GraphicsShadersRead:
        case CoreGraphics::PipelineStage::ComputeShaderRead:
        case CoreGraphics::PipelineStage::AllShadersRead:
        case CoreGraphics::PipelineStage::ColorWrite:           // The image layout from a pass is read on finished
        case CoreGraphics::PipelineStage::DepthStencilWrite:    // The image layout from a pass is read on finished
            if (depthStencil)
                return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
            else
                return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        case CoreGraphics::PipelineStage::VertexShaderWrite:
        case CoreGraphics::PipelineStage::HullShaderWrite:
        case CoreGraphics::PipelineStage::DomainShaderWrite:
        case CoreGraphics::PipelineStage::GeometryShaderWrite:
        case CoreGraphics::PipelineStage::PixelShaderWrite:
        case CoreGraphics::PipelineStage::GraphicsShadersWrite:
        case CoreGraphics::PipelineStage::ComputeShaderWrite:
        case CoreGraphics::PipelineStage::AllShadersWrite:
            return VK_IMAGE_LAYOUT_GENERAL;
        case CoreGraphics::PipelineStage::ColorRead:
            return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        case CoreGraphics::PipelineStage::DepthStencilRead:
            return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        case CoreGraphics::PipelineStage::TransferRead:
            return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        case CoreGraphics::PipelineStage::TransferWrite:
            return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        case CoreGraphics::PipelineStage::HostRead:
            return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        case CoreGraphics::PipelineStage::HostWrite:
            return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        case CoreGraphics::PipelineStage::MemoryRead:
            return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        case CoreGraphics::PipelineStage::MemoryWrite:
            return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        case CoreGraphics::PipelineStage::ImageInitial:
            return VK_IMAGE_LAYOUT_UNDEFINED;
        case CoreGraphics::PipelineStage::Present:
            return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
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
    case CoreGraphics::PrimitiveTopology::LineListAdjacency: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY;
    case CoreGraphics::PrimitiveTopology::LineStripAdjacency: return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY;
    case CoreGraphics::PrimitiveTopology::TriangleList: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    case CoreGraphics::PrimitiveTopology::TriangleStrip: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    case CoreGraphics::PrimitiveTopology::TriangleListAdjacency: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY;
    case CoreGraphics::PrimitiveTopology::TriangleStripAdjacency: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY;
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
    case VertexComponent::Short:    return 2;
    case VertexComponent::Short2:   return 4;
    case VertexComponent::Short4:   return 8;
    case VertexComponent::UShort4:  return 8;
    case VertexComponent::UByte4N:  return 4;
    case VertexComponent::Byte4N:   return 4;
    case VertexComponent::Short2N:  return 4;
    case VertexComponent::Short4N:  return 8;
    case VertexComponent::UShort:   return 2;
    case VertexComponent::UInt:     return 4;
    case VertexComponent::UInt2:    return 8;
    case VertexComponent::UInt3:    return 12;
    case VertexComponent::UInt4:    return 16;
    case VertexComponent::Int:      return 4;
    case VertexComponent::Int2:     return 8;
    case VertexComponent::Int3:     return 12;
    case VertexComponent::Int4:     return 16;
    default:
        n_error("VkTypes::AsVkSize(): invalid input parameter!");
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
    case VertexComponent::UInt:     return 1;
    case VertexComponent::UInt2:    return 2;
    case VertexComponent::UInt3:    return 3;
    case VertexComponent::UInt4:    return 4;
    case VertexComponent::Int:      return 1;
    case VertexComponent::Int2:     return 2;
    case VertexComponent::Int3:     return 3;
    case VertexComponent::Int4:     return 4;
    case VertexComponent::UShort:   return 1;
    case VertexComponent::UShort2:  return 2;
    case VertexComponent::UShort3:  return 3;
    case VertexComponent::UShort4:  return 4;
    case VertexComponent::Short:    return 1;
    case VertexComponent::Short2:   return 2;
    case VertexComponent::Short3:   return 3;
    case VertexComponent::Short4:   return 4;

    case VertexComponent::UByte4:   return 4;
    case VertexComponent::Byte4:    return 4;
    case VertexComponent::UByte4N:  return 4;
    case VertexComponent::Byte4N:   return 4;
    case VertexComponent::Short2N:  return 2;
    case VertexComponent::Short4N:  return 4;
    case VertexComponent::UShort2N: return 2;
    case VertexComponent::UShort4N: return 4;

    default:
        n_error("VkTypes::AsVkNumComponents(): invalid input parameter!");
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
    case VertexComponent::UInt:     return VK_FORMAT_R32_UINT;
    case VertexComponent::UInt2:    return VK_FORMAT_R32G32_UINT;
    case VertexComponent::UInt3:    return VK_FORMAT_R32G32B32_UINT;
    case VertexComponent::UInt4:    return VK_FORMAT_R32G32B32A32_UINT;
    case VertexComponent::Int:      return VK_FORMAT_R32_SINT;
    case VertexComponent::Int2:     return VK_FORMAT_R32G32_SINT;
    case VertexComponent::Int3:     return VK_FORMAT_R32G32B32_SINT;
    case VertexComponent::Int4:     return VK_FORMAT_R32G32B32A32_SINT;
    case VertexComponent::UShort:   return VK_FORMAT_R16_UINT;
    case VertexComponent::UShort2:  return VK_FORMAT_R16G16_UINT;
    case VertexComponent::UShort3:  return VK_FORMAT_R16G16B16_UINT;
    case VertexComponent::UShort4:  return VK_FORMAT_R16G16B16A16_UINT;
    case VertexComponent::Short:    return VK_FORMAT_R16_SINT;
    case VertexComponent::Short2:   return VK_FORMAT_R16G16_SINT;
    case VertexComponent::Short3:   return VK_FORMAT_R16G16B16_SINT;
    case VertexComponent::Short4:   return VK_FORMAT_R16G16B16A16_SINT;

    case VertexComponent::UByte4:   return VK_FORMAT_R8G8B8A8_UINT;
    case VertexComponent::Byte4:    return VK_FORMAT_R8G8B8A8_SINT;
    case VertexComponent::UByte4N:  return VK_FORMAT_R8G8B8A8_UNORM;
    case VertexComponent::Byte4N:   return VK_FORMAT_R8G8B8A8_SNORM;
    case VertexComponent::Short2N:  return VK_FORMAT_R16G16_SNORM;
    case VertexComponent::Short4N:  return VK_FORMAT_R16G16B16A16_SNORM;
    case VertexComponent::UShort2N: return VK_FORMAT_R16G16_UNORM;
    case VertexComponent::UShort4N: return VK_FORMAT_R16G16B16A16_UNORM;

    default:
        n_error("VkTypes::AsVkVertexType(): invalid input parameter!");
        return VK_FORMAT_R32G32B32A32_SFLOAT;
    }
}

} // namespace Vulkan
