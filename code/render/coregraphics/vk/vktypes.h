#pragma once
//------------------------------------------------------------------------------
/**
	Utility functions to convert from Nebula enums to Vulkan values.
	
	(C) 2016-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/types.h"
#include "coregraphics/pixelformat.h"
#include "coregraphics/vertexcomponent.h"
#include "coregraphics/primitivetopology.h"
#include "coregraphics/antialiasquality.h"
#include "coregraphics/imagefileformat.h"
#include "coregraphics/indextype.h"
#include "coregraphics/barrier.h"
#include "coregraphics/gpubuffertypes.h"
#include "coregraphics/config.h"
#include "IL/il.h"

namespace Vulkan
{
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
	/// check if format is depth or color
	static bool IsDepthFormat(CoreGraphics::PixelFormat::Code p);
	/// convert DevIL pixel format to Vulkan format
	static VkFormat AsVkFormat(ILenum p);
	/// convert VkFormat pixel format to DevIL format
	static ILenum AsILDXTFormat(VkFormat p);
	/// returns true if format is compressed
	static bool IsCompressedFormat(VkFormat p);
	/// convert pixel format to block size
	static VkBlockDimensions AsVkBlockSize(CoreGraphics::PixelFormat::Code p);
	/// convert pixel format to block size
	static VkBlockDimensions AsVkBlockSize(VkFormat fmt);
	/// convert Vulkan image compressed format to one which is mappable
	static VkFormat AsVkMappableImageFormat(VkFormat fmt);
	/// convert Nebula pixel format to Vulkan render target format
	static VkFormat AsVkFramebufferFormat(CoreGraphics::PixelFormat::Code p);
	/// convert Nebula pixel format to a guaranteed to be sampleable format
	static VkFormat AsVkSampleableFormat(CoreGraphics::PixelFormat::Code p);
	/// convert Nebula pixel format to Vulkan data format
	static VkFormat AsVkDataFormat(CoreGraphics::PixelFormat::Code p);
	/// convert uint to vulkan sample count
	static VkSampleCountFlagBits AsVkSampleFlags(const SizeT samples);
	/// convert DevIL pixel format to Vulkan component mapping
	static VkComponentMapping AsVkMapping(ILenum p);
	/// convert vulkan format back to nebula format
	static CoreGraphics::PixelFormat::Code AsNebulaPixelFormat(VkFormat f);
	/// convert pixel format to Vulkan component mapping
	static VkComponentMapping AsVkMapping(CoreGraphics::PixelFormat::Code p);
	/// convert dependency flags to vulkan
	static VkPipelineStageFlags AsVkPipelineFlags(const CoreGraphics::BarrierStage dep);
	/// convert dependency flags to vulkan
	static VkPipelineStageFlags AsVkResourceAccessFlags(const CoreGraphics::BarrierAccess access);
	/// convert image aspects to Vulkan
	static VkImageAspectFlags AsVkImageAspectFlags(const CoreGraphics::ImageAspect aspect);
	/// convert shader visibility to vulkan
	static VkShaderStageFlags AsVkShaderVisibility(const CoreGraphics::ShaderVisibility vis);
	/// convert image layout to vulkan
	static VkImageLayout AsVkImageLayout(const CoreGraphics::ImageLayout layout);

#pragma endregion

	/// convert primitive topology to vulkan
	static VkPrimitiveTopology AsVkPrimitiveType(CoreGraphics::PrimitiveTopology::Code t);
	/// convert the format to it's size
	static uint32_t AsByteSize(uint32_t semantic);
	/// convert vertex format to size
	static uint32_t AsVkSize(CoreGraphics::VertexComponent::Format f);
	/// convert vertex format to number of components
	static uint32_t AsVkNumComponents(CoreGraphics::VertexComponent::Format f);
	/// convert vertex component type to OGL4 symbolic type (single-element)
	static VkFormat AsVkVertexType(CoreGraphics::VertexComponent::Format f);
};
} // namespace Vulkan