#pragma once
//------------------------------------------------------------------------------
/**
    @file sampler.h
    
    Samplers

    @copyright
    (C) 2018-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/id.h"
#include "util/bit.h"
namespace CoreGraphics
{

ID_24_8_TYPE(SamplerId);

enum SamplerFilter
{
    NearestFilter,
    LinearFilter,
    CubicFilter
};

enum SamplerMipMode
{
    NearestMipMode,
    LinearMipMode
};

enum SamplerAddressMode
{
    RepeatAddressMode,
    MirroredRepeatAddressMode,
    ClampToEdgeAddressMode,
    ClampToBorderAddressMode,
    MirrorClampToEdgeAddressMode,
};

enum SamplerCompareOperation
{
    NeverCompare,
    LessCompare,
    EqualCompare,
    LessOrEqualCompare,
    GreaterCompare,
    GreaterOrEqualCompare,
    NotEqualCompare,
    AlwaysCompare
};

enum SamplerBorderMode
{
    FloatTransparentBlackBorder,
    IntTransparentBlackBorder,
    FloatOpaqueBlackBorder,
    IntOpaqueBlackBorder,
    FloatOpaqueWhiteBorder,
    IntOpaqueWhiteBorder,
};

struct SamplerCreateInfo
{
    SamplerFilter               magFilter;
    SamplerFilter               minFilter;
    SamplerMipMode              mipmapMode;
    SamplerAddressMode          addressModeU;
    SamplerAddressMode          addressModeV;
    SamplerAddressMode          addressModeW;
    float                       mipLodBias;
    bool                        anisotropyEnable;
    float                       maxAnisotropy;
    bool                        compareEnable;
    SamplerCompareOperation     compareOp;
    float                       minLod;
    float                       maxLod;
    SamplerBorderMode           borderColor;
    bool                        unnormalizedCoordinates;

    const uint32_t HashCode() const
    {
        uint32_t res = 0;
        Util::HashCombine(res, this->magFilter);
        Util::HashCombine(res, this->minFilter);
        Util::HashCombine(res, this->mipmapMode);
        Util::HashCombine(res, this->addressModeU);
        Util::HashCombine(res, this->addressModeV);
        Util::HashCombine(res, this->addressModeW);
        Util::HashCombine(res, this->mipLodBias);
        Util::HashCombine(res, this->anisotropyEnable);
        Util::HashCombine(res, this->maxAnisotropy);
        Util::HashCombine(res, this->compareEnable);
        Util::HashCombine(res, this->compareOp);
        Util::HashCombine(res, this->minLod);
        Util::HashCombine(res, this->maxLod);
        Util::HashCombine(res, this->borderColor);
        Util::HashCombine(res, this->unnormalizedCoordinates);
        return res;
    }
};

/// create sampler
SamplerId CreateSampler(const SamplerCreateInfo& info);
/// destroy sampler
void DestroySampler(const SamplerId& id);

} // namespace CoreGraphics
