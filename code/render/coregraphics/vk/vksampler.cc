//------------------------------------------------------------------------------
//  vksampler.cc
//  (C) 2018-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "vksampler.h"
#include "vkgraphicsdevice.h"
#include "coregraphics/config.h"
namespace Vulkan
{

VkSamplerAllocator samplerAllocator;

//------------------------------------------------------------------------------
/**
*/
const VkSampler&
SamplerGetVk(const CoreGraphics::SamplerId& id)
{
    return samplerAllocator.Get<1>(id.id);
}

} // namespace Vulkan

namespace CoreGraphics
{

using namespace Vulkan;

//------------------------------------------------------------------------------
/**
*/
VkFilter 
ToVkSamplerFilter(SamplerFilter filter)
{
    switch (filter)
    {
    case LinearFilter:  return VK_FILTER_LINEAR;
    case NearestFilter: return VK_FILTER_NEAREST;
    case CubicFilter:   return VK_FILTER_CUBIC_IMG;
    default:
        n_error("No sampler filter mode %d supported!", filter);
        return VK_FILTER_LINEAR;
    }
}

//------------------------------------------------------------------------------
/**
*/
VkSamplerMipmapMode
ToVkSamplerMipMapMode(SamplerMipMode mode)
{
    switch (mode)
    {
    case LinearMipMode:     return VK_SAMPLER_MIPMAP_MODE_LINEAR;
    case NearestMipMode:    return VK_SAMPLER_MIPMAP_MODE_NEAREST;
    default:
        n_error("No sampler mipmap mode %d supported!", mode);
        return VK_SAMPLER_MIPMAP_MODE_LINEAR;
    }
}

//------------------------------------------------------------------------------
/**
*/
VkSamplerAddressMode
ToVkSamplerAddressMode(SamplerAddressMode mode)
{
    switch (mode)
    {
    case RepeatAddressMode:             return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    case MirroredRepeatAddressMode:     return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    case ClampToEdgeAddressMode:        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    case ClampToBorderAddressMode:      return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    case MirrorClampToEdgeAddressMode:  return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
    default:
        n_error("No sampler address mode %d supported!", mode);
        return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    }
}

//------------------------------------------------------------------------------
/**
*/
VkCompareOp
ToVkCompareOperation(SamplerCompareOperation op)
{
    switch (op)
    {
    case NeverCompare:          return VK_COMPARE_OP_NEVER;
    case LessCompare:           return VK_COMPARE_OP_LESS;
    case EqualCompare:          return VK_COMPARE_OP_EQUAL;
    case LessOrEqualCompare:    return VK_COMPARE_OP_LESS_OR_EQUAL;
    case GreaterCompare:        return VK_COMPARE_OP_GREATER;
    case NotEqualCompare:       return VK_COMPARE_OP_NOT_EQUAL;
    case GreaterOrEqualCompare: return VK_COMPARE_OP_GREATER_OR_EQUAL;
    case AlwaysCompare:         return VK_COMPARE_OP_ALWAYS;
    default:
        n_error("No comparison mode %d supported!", op);
        return VK_COMPARE_OP_NEVER;
    }
}

//------------------------------------------------------------------------------
/**
*/
VkBorderColor
ToVkBorderMode(SamplerBorderMode mode)
{
    switch (mode)
    {
    case FloatTransparentBlackBorder:   return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    case IntTransparentBlackBorder:     return VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
    case FloatOpaqueBlackBorder:        return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    case IntOpaqueBlackBorder:          return VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    case FloatOpaqueWhiteBorder:        return VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    case IntOpaqueWhiteBorder:          return VK_BORDER_COLOR_INT_OPAQUE_WHITE;
    default:
        n_error("No border mode %d supported!", mode);
        return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    }
}


//------------------------------------------------------------------------------
/**
*/
SamplerFilter
FromVkSamplerFilter(VkFilter filter)
{
    switch (filter)
    {
    case VK_FILTER_LINEAR:      return LinearFilter;
    case VK_FILTER_NEAREST:     return NearestFilter;
    case VK_FILTER_CUBIC_IMG:   return CubicFilter;
    default:
        n_error("No filter mode %d supported!", filter);
        return LinearFilter;
    }
}

//------------------------------------------------------------------------------
/**
*/
SamplerMipMode
FromVkSamplerMipMapMode(VkSamplerMipmapMode mode)
{
    switch (mode)
    {
    case VK_SAMPLER_MIPMAP_MODE_LINEAR:     return LinearMipMode;
    case VK_SAMPLER_MIPMAP_MODE_NEAREST:    return NearestMipMode;
    default:
        n_error("No sampler mipmap mode %d supported!", mode);
        return LinearMipMode;
    }
}

//------------------------------------------------------------------------------
/**
*/
SamplerAddressMode
FromVkSamplerAddressMode(VkSamplerAddressMode mode)
{
    switch (mode)
    {
    case VK_SAMPLER_ADDRESS_MODE_REPEAT:                return RepeatAddressMode;
    case VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT:       return MirroredRepeatAddressMode;
    case VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE:         return ClampToEdgeAddressMode;
    case VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER:       return ClampToBorderAddressMode;
    case VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE:  return MirrorClampToEdgeAddressMode;
    default:
        n_error("No sampler address mode %d supported!", mode);
        return RepeatAddressMode;
    }
}

//------------------------------------------------------------------------------
/**
*/
SamplerCompareOperation
FromVkCompareOperation(VkCompareOp op)
{
    switch (op)
    {
    case VK_COMPARE_OP_NEVER:           return NeverCompare;
    case VK_COMPARE_OP_LESS:            return LessCompare;
    case VK_COMPARE_OP_EQUAL:           return EqualCompare;
    case VK_COMPARE_OP_LESS_OR_EQUAL:   return LessOrEqualCompare;
    case VK_COMPARE_OP_GREATER:         return GreaterCompare;
    case VK_COMPARE_OP_NOT_EQUAL:       return NotEqualCompare;
    case VK_COMPARE_OP_GREATER_OR_EQUAL:return GreaterOrEqualCompare;
    case VK_COMPARE_OP_ALWAYS:          return AlwaysCompare;
    default:
        n_error("No comparison mode %d supported!", op);
        return NeverCompare;
    }
}

//------------------------------------------------------------------------------
/**
*/
SamplerBorderMode
FromVkBorderMode(VkBorderColor mode)
{
    switch (mode)
    {
    case VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK :  return FloatTransparentBlackBorder;
    case VK_BORDER_COLOR_INT_TRANSPARENT_BLACK:     return IntTransparentBlackBorder;
    case VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK:        return FloatOpaqueBlackBorder;
    case VK_BORDER_COLOR_INT_OPAQUE_BLACK:          return IntOpaqueBlackBorder;
    case VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE:        return FloatOpaqueWhiteBorder;
    case VK_BORDER_COLOR_INT_OPAQUE_WHITE:          return IntOpaqueWhiteBorder;
    default:
        n_error("No border mode %d supported!", mode);
        return FloatTransparentBlackBorder;
    }
}

Util::Dictionary<uint32_t, Ids::Id32> UniqueSamplerHashes;


//------------------------------------------------------------------------------
/**
*/
SamplerId
CreateSampler(const SamplerCreateInfo& info)
{
    uint32_t hash = info.HashCode();
    IndexT i = UniqueSamplerHashes.FindIndex(hash);
    if (i == InvalidIndex)
    {
        Ids::Id32 id = samplerAllocator.Alloc();

        VkDevice& dev = samplerAllocator.Get<0>(id);
        VkSampler& sampler = samplerAllocator.Get<1>(id);
        samplerAllocator.Set<2>(id, hash);

        dev = Vulkan::GetCurrentDevice();
        VkSamplerCreateInfo samplerInfo =
        {
            VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            NULL,
            0,
            ToVkSamplerFilter(info.minFilter),
            ToVkSamplerFilter(info.magFilter),
            ToVkSamplerMipMapMode(info.mipmapMode),
            ToVkSamplerAddressMode(info.addressModeU),
            ToVkSamplerAddressMode(info.addressModeV),
            ToVkSamplerAddressMode(info.addressModeW),
            info.mipLodBias,
            info.anisotropyEnable,
            info.maxAnisotropy,
            info.compareEnable,
            ToVkCompareOperation(info.compareOp),
            -info.minLod,
            info.maxLod,
            ToVkBorderMode(info.borderColor),
            info.unnormalizedCoordinates
        };
        VkResult res = vkCreateSampler(dev, &samplerInfo, nullptr, &sampler);
        n_assert(res == VK_SUCCESS);

        UniqueSamplerHashes.Add(hash, id);

        SamplerId ret = id;
        return ret;
    }
    else
    {
        SamplerId ret = UniqueSamplerHashes.ValueAtIndex(i);
        return ret;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
DestroySampler(const SamplerId& id)
{
    UniqueSamplerHashes.Erase(samplerAllocator.Get<2>(id.id));
    VkDevice& dev = samplerAllocator.Get<0>(id.id);
    VkSampler& sampler = samplerAllocator.Get<1>(id.id);
    vkDestroySampler(dev, sampler, nullptr);

    samplerAllocator.Dealloc(id.id);
}

//------------------------------------------------------------------------------
/**
*/
SamplerCreateInfo
ToNebulaSamplerCreateInfo(const VkSamplerCreateInfo& info)
{
    SamplerCreateInfo samplerInfo =
    {
        FromVkSamplerFilter(info.magFilter),
        FromVkSamplerFilter(info.minFilter),
        FromVkSamplerMipMapMode(info.mipmapMode),
        FromVkSamplerAddressMode(info.addressModeU),
        FromVkSamplerAddressMode(info.addressModeV),
        FromVkSamplerAddressMode(info.addressModeW),
        info.mipLodBias,
        info.anisotropyEnable == 1,
        info.maxAnisotropy,
        info.compareEnable == 1,
        FromVkCompareOperation(info.compareOp),
        info.minLod,
        info.maxLod,
        FromVkBorderMode(info.borderColor),
        info.unnormalizedCoordinates == 1
    };
    return samplerInfo;
}

} // namespace CoreGraphics
