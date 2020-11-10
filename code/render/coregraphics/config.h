#pragma once
//------------------------------------------------------------------------------
/**
    @file coregraphics/config.h
    
    Compile time configuration options for the CoreGraphics subsystem.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "util/string.h"
#include "core/rttimacros.h"

#define NEBULA_ENABLE_MT_DRAW 1
#define NEBULA_WHOLE_BUFFER_SIZE (-1)
namespace CoreGraphics
{

enum IdType
{
	BufferIdType,
	TextureIdType,
	TextureViewIdType,
	VertexLayoutIdType,
	ShaderIdType,
	ShaderProgramIdType,
	ShaderStateIdType,
	ShaderInstanceIdType,
	ShaderConstantIdType,
	CommandBufferIdType,
	CommandBufferPoolIdType,
	MeshIdType,
	EventIdType,
	BarrierIdType,
	SemaphoreIdType,
	FenceIdType,
	WindowIdType,
	PassIdType,
	AnimResourceIdType,
	ResourceTableIdType,
	ResourceTableLayoutIdType,
	ResourcePipelineIdType,
	SamplerIdType,
	MaterialIdType,
	SubmissionContextIdType,
	ImageIdType
};

enum QueueType
{
	GraphicsQueueType,
	ComputeQueueType,
	TransferQueueType,
	SparseQueueType,

	NumQueueTypes,

	InvalidQueueType
};

enum ShaderVisibility
{
	InvalidVisibility			= 0,
	VertexShaderVisibility		= 1 << 0,
	HullShaderVisibility		= 1 << 2,
	DomainShaderVisibility		= 1 << 3,
	GeometryShaderVisibility	= 1 << 4,
	PixelShaderVisibility		= 1 << 5,
	AllGraphicsVisibility		= VertexShaderVisibility | HullShaderVisibility | DomainShaderVisibility | GeometryShaderVisibility | PixelShaderVisibility,
	ComputeShaderVisibility		= 1 << 6,
	AllVisibility				= VertexShaderVisibility | HullShaderVisibility | DomainShaderVisibility | GeometryShaderVisibility | PixelShaderVisibility | ComputeShaderVisibility
};
__ImplementEnumBitOperators(CoreGraphics::ShaderVisibility);

enum class ImageAspect
{
	ColorBits = (1 << 0),
	DepthBits = (1 << 1),
	StencilBits = (1 << 2),
	MetaBits = (1 << 3),
	Plane0Bits = (1 << 4),
	Plane1Bits = (1 << 5),
	Plane2Bits = (1 << 6)
};
__ImplementEnumBitOperators(CoreGraphics::ImageAspect);
__ImplementEnumComparisonOperators(CoreGraphics::ImageAspect);

enum class ImageLayout
{
	Undefined,
	General,
	ColorRenderTexture,
	DepthStencilRenderTexture,
	DepthStencilRead,
	ShaderRead,
	TransferSource,
	TransferDestination,
	Preinitialized,
	Present
};

enum GlobalConstantBufferType
{
	MainThreadConstantBuffer,
	VisibilityThreadConstantBuffer, // perform constant updates from the visibility thread (shader state node instance update for example...)

	NumConstantBufferTypes
};

enum QueryType
{
	OcclusionQuery,
	GraphicsTimestampQuery,
	PipelineStatisticsGraphicsQuery,
	QueryGraphicsMax = PipelineStatisticsGraphicsQuery,

	ComputeTimestampQuery,
	PipelineStatisticsComputeQuery,
	QueryComputeMax = PipelineStatisticsComputeQuery,
	
	NumQueryTypes
};

enum BufferAccessMode
{
	DeviceLocal,		// buffer can only be used by the GPU, typical use is for static geometry data that doesn't change
	HostLocal,			// buffer can only be updated by the CPU and can be used for GPU transfer operations, typical use is transient copy buffers
	HostToDevice,		// buffer can be updated on the CPU and sent to the GPU, typical use is for dynamic and frequent buffer updates
	DeviceToHost		// buffer can be updated by the GPU and be read on the CPU, typical use is to map and read back memory
};

//------------------------------------------------------------------------------
/**
*/
inline ShaderVisibility
ShaderVisibilityFromString(const Util::String& str)
{
	Util::Array<Util::String> components = str.Tokenize("|");
	CoreGraphics::ShaderVisibility ret = InvalidVisibility;
	IndexT i;
	for (i = 0; i < components.Size(); i++)
	{
		const Util::String& component = components[i];
		if (component == "VS")		ret |= VertexShaderVisibility;
		else if (component == "HS") ret |= HullShaderVisibility;
		else if (component == "DS") ret |= DomainShaderVisibility;
		else if (component == "GS") ret |= GeometryShaderVisibility;
		else if (component == "PS") ret |= PixelShaderVisibility;
		else if (component == "CS") ret |= ComputeShaderVisibility;
	}

	return ret;
}

//------------------------------------------------------------------------------
/**
*/
inline QueueType
QueueTypeFromString(const Util::String& str)
{
	if (str == "Graphics")		return GraphicsQueueType;
	else if (str == "Compute")	return ComputeQueueType;
	else if (str == "Transfer") return TransferQueueType;
	else if (str == "Sparse")	return SparseQueueType;
	return GraphicsQueueType;
}

//------------------------------------------------------------------------------
/**
*/
inline const char*
QueueNameFromQueueType(const QueueType type)
{
    switch (type)
    {
    case GraphicsQueueType:
        return "Graphics";
    case ComputeQueueType:
        return "Compute";
    case TransferQueueType:
        return "Transfer";
    case SparseQueueType:
        return "Sparse";
    default:
        return "Graphics";
    }
}

} // namespace CoreGraphics


#define SHADER_POSTEFFECT_DEFAULT_FEATURE_MASK "Alt0"

#if !PUBLIC_BUILD
#define NEBULA_GRAPHICS_DEBUG 1
#define NEBULA_MARKER_BLUE Math::vec4(0.8f, 0.8f, 1.0f, 1.0f)
#define NEBULA_MARKER_RED Math::vec4(1.0f, 0.8f, 0.8f, 1.0f)
#define NEBULA_MARKER_GREEN Math::vec4(0.8f, 1.0f, 0.8f, 1.0f)
#define NEBULA_MARKER_DARK_GREEN Math::vec4(0.6f, 0.8f, 0.6f, 1.0f)
#define NEBULA_MARKER_DARK_DARK_GREEN Math::vec4(0.5f, 0.7f, 0.5f, 1.0f)
#define NEBULA_MARKER_PINK Math::vec4(1.0f, 0.8f, 0.9f, 1.0f)
#define NEBULA_MARKER_PURPLE Math::vec4(0.9f, 0.7f, 0.9f, 1.0f)
#define NEBULA_MARKER_ORANGE Math::vec4(1.0f, 0.9f, 0.8f, 1.0f)
#define NEBULA_MARKER_TURQOISE Math::vec4(0.8f, 0.9f, 1.0f, 1.0f)
#define NEBULA_MARKER_GRAY Math::vec4(0.9f, 0.9f, 0.9f, 1.0f)
#define NEBULA_MARKER_BLACK Math::vec4(0.001f)
#define NEBULA_MARKER_WHITE Math::vec4(1)

#define NEBULA_MARKER_COMPUTE NEBULA_MARKER_BLUE
#define NEBULA_MARKER_GRAPHICS NEBULA_MARKER_GREEN
#define NEBULA_MARKER_TRANSFER NEBULA_MARKER_RED

#endif

//------------------------------------------------------------------------------
#if __VULKAN__
	#define COREGRAPHICS_TRIANGLE_FRONT_FACE_CCW (1)
	// define the same descriptor set slots as we do in the shaders
	#define NEBULA_TICK_GROUP 0				// set per tick (once for all views) by the system
	#define NEBULA_FRAME_GROUP 1			// set per frame (once per view) by the system
	#define NEBULA_PASS_GROUP 2				// set per pass by the system
	#define NEBULA_BATCH_GROUP 3			// set per batch (material settings or system stuff)
	#define NEBULA_INSTANCE_GROUP 4			// set a batch-internal copy of some specific settings
	#define NEBULA_SYSTEM_GROUP 5			// set a batch-internal copy of some specific settings
	#define NEBULA_DYNAMIC_OFFSET_GROUP 6	// set once per shader and is offset for each instance
	#define NEBULA_NUM_GROUPS (NEBULA_DYNAMIC_OFFSET_GROUP + 1)

	#define MAX_INPUT_ATTACHMENTS 32

	#define SHADER_MODEL_5 (1)
	#ifdef _DEBUG
		#define NEBULA_VULKAN_DEBUG (1)
	#else
		#define NEBULA_VULKAN_DEBUG (0)
	#endif
	#define PROJECTION_HANDEDNESS_LH (0)
#if __X64__
	#define VK_DEVICE_SIZE_CONV(x) uint64_t(x)
#else
	#define VK_DEVICE_SIZE_CONV(x) uint32_t(x)
#endif
#endif
//------------------------------------------------------------------------------
