#pragma once
//------------------------------------------------------------------------------
/**
    Utility functions to convert from Nebula enums to Vulkan values.
    
    @copyright
    (C) 2016-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/types.h"
#include "coregraphics/pixelformat.h"
#include "coregraphics/vertexcomponent.h"
#include "coregraphics/primitivetopology.h"
#include "coregraphics/config.h"

namespace Vulkan
{

static const VkComponentSwizzle VkSwizzle[] =
{
    VK_COMPONENT_SWIZZLE_IDENTITY,
    VK_COMPONENT_SWIZZLE_R,
    VK_COMPONENT_SWIZZLE_G,
    VK_COMPONENT_SWIZZLE_B,
    VK_COMPONENT_SWIZZLE_A,
    VK_COMPONENT_SWIZZLE_ZERO,
    VK_COMPONENT_SWIZZLE_ONE,
};

class VkTypes
{
public:

    struct VkBlockDimensions
    {
        uint32_t width;
        uint32_t height;
    };



#pragma region Pixel stuff
    /// convert Nebula pixel format to Vulkan pixel format
    static VkFormat AsVkFormat(CoreGraphics::PixelFormat::Code p);
    /// returns true if format is compressed
    static bool IsCompressedFormat(VkFormat p);
    /// convert pixel format to block size
    static VkBlockDimensions AsVkBlockSize(CoreGraphics::PixelFormat::Code p);
    /// convert pixel format to block size
    static VkBlockDimensions AsVkBlockSize(VkFormat fmt);
    /// convert Vulkan image compressed format to one which is mappable
    static VkFormat AsVkMappableImageFormat(VkFormat fmt);
    /// convert Nebula pixel format to Vulkan data format
    static VkFormat AsVkDataFormat(CoreGraphics::PixelFormat::Code p);
    /// convert uint to vulkan sample count
    static VkSampleCountFlagBits AsVkSampleFlags(const SizeT samples);
    /// convert texture type to vk image type
    static VkImageType AsVkImageType(CoreGraphics::TextureType type);
    /// convert texture type to vk view type
    static VkImageViewType AsVkImageViewType(CoreGraphics::TextureType type);
    /// convert vulkan format back to nebula format
    static CoreGraphics::PixelFormat::Code AsNebulaPixelFormat(VkFormat f);
    /// convert image aspects to Vulkan
    static VkImageAspectFlags AsVkImageAspectFlags(const CoreGraphics::ImageBits bits);
    /// convert shader visibility to vulkan
    static VkShaderStageFlags AsVkShaderVisibility(const CoreGraphics::ShaderVisibility vis);
    /// convert image layout to vulkan
    static VkImageLayout AsVkImageLayout(const CoreGraphics::ImageLayout layout);
    /// Convert pipeline stage to Vk pipeline stage
    static VkPipelineStageFlags AsVkPipelineStage(const CoreGraphics::PipelineStage stage);
    /// Convert pipeline stage and pipeline access to Vk access flags
    static VkAccessFlags AsVkAccessFlags(const CoreGraphics::PipelineStage stage);
    /// Convert pipeline stage to image layout
    static VkImageLayout AsVkImageLayout(const CoreGraphics::PipelineStage stage, bool depthStencil = false);

#pragma endregion

    /// convert primitive topology to vulkan
    static VkPrimitiveTopology AsVkPrimitiveType(CoreGraphics::PrimitiveTopology::Code t);
    /// convert vertex format to size
    static uint32_t AsVkSize(CoreGraphics::VertexComponent::Format f);
    /// convert vertex format to number of components
    static uint32_t AsVkNumComponents(CoreGraphics::VertexComponent::Format f);
    /// convert vertex component type to OGL4 symbolic type (single-element)
    static VkFormat AsVkVertexType(CoreGraphics::VertexComponent::Format f);
};
} // namespace Vulkan
