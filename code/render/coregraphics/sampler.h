#pragma once
//------------------------------------------------------------------------------
/**
    Samplers

    @copyright
    (C) 2018-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/id.h"
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
};

/// create sampler
SamplerId CreateSampler(const SamplerCreateInfo& info);
/// destroy sampler
void DestroySampler(const SamplerId& id);

} // namespace CoreGraphics
