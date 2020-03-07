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

#define NEBULA_ENABLE_MT_DRAW 0
namespace CoreGraphics
{

enum IdType
{
	VertexBufferIdType,
	IndexBufferIdType,
	TextureIdType,
	VertexLayoutIdType,
	ConstantBufferIdType,
	ShaderRWBufferIdType,
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
	SubmissionContextIdType
};

enum QueueType
{
	GraphicsQueueType,
	ComputeQueueType,
	TransferQueueType,
	SparseQueueType,
	InvalidQueueType,

	NumQueueTypes
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

enum VertexBufferMemoryType
{
	MainThreadVertexMemory,
	VisibilityThreadVertexMemory,

	NumVertexBufferMemoryTypes
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

enum BufferUpdateMode
{
	HostWriteable,              // host can write to the buffer
	DeviceWriteable             // only device can write to the buffer
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
#define NEBULA_MARKER_BLUE Math::float4(0.8f, 0.8f, 1.0f, 1.0f)
#define NEBULA_MARKER_RED Math::float4(1.0f, 0.8f, 0.8f, 1.0f)
#define NEBULA_MARKER_GREEN Math::float4(0.8f, 1.0f, 0.8f, 1.0f)
#define NEBULA_MARKER_DARK_GREEN Math::float4(0.6f, 0.8f, 0.6f, 1.0f)
#define NEBULA_MARKER_DARK_DARK_GREEN Math::float4(0.5f, 0.7f, 0.5f, 1.0f)
#define NEBULA_MARKER_PINK Math::float4(1.0f, 0.8f, 0.9f, 1.0f)
#define NEBULA_MARKER_PURPLE Math::float4(0.9f, 0.7f, 0.9f, 1.0f)
#define NEBULA_MARKER_ORANGE Math::float4(1.0f, 0.9f, 0.8f, 1.0f)
#define NEBULA_MARKER_TURQOISE Math::float4(0.8f, 0.9f, 1.0f, 1.0f)
#define NEBULA_MARKER_GRAY Math::float4(0.9f, 0.9f, 0.9f, 1.0f)
#define NEBULA_MARKER_BLACK Math::float4(0.001f)
#define NEBULA_MARKER_WHITE Math::float4(1)
#endif

//------------------------------------------------------------------------------
#if __DX11__ || __DX9__
    #if __DX11__
        #define SHADER_MODEL_5 (1)
    #endif
	#define COREGRAPHICS_PIXEL_CENTER_HALF_PIXEL (1)
	#define COREGRAPHICS_TRIANGLE_FRONT_FACE_CCW (1)

	#define NEBULA_USEDIRECT3D9 (1)
	#define NEBULA_USEDIRECT3D10 (0)

	#define NEBULA_DIRECT3D_USENVPERFHUD (0)
	#define NEBULA_DIRECT3D_DEBUG (0)

	#if NEBULA_DIRECT3D_USENVPERFHUD
	#define NEBULA_DIRECT3D_DEVICETYPE D3DDEVTYPE_REF
	#else
	#define NEBULA_DIRECT3D_DEVICETYPE D3DDEVTYPE_HAL
	#endif
	
#elif __OGL4__
    #define SHADER_MODEL_5 (1)
    #ifdef _DEBUG
	    #define NEBULA_OPENGL4_DEBUG (1)
    #else
        #define	NEBULA_OPENGL4_DEBUG (0)
    #endif
	#define PROJECTION_HANDEDNESS_LH (1)
#elif __VULKAN__
	#define COREGRAPHICS_TRIANGLE_FRONT_FACE_CCW (1)
	// define the same descriptor set slots as we do in the shaders
	#define NEBULA_TICK_GROUP 0				// set per tick (once for all views) by the system
	#define NEBULA_FRAME_GROUP 1				// set per frame (once per view) by the system
	#define NEBULA_PASS_GROUP 2				// set per pass by the system
	#define NEBULA_BATCH_GROUP 3				// set per batch (material settings or system stuff)
	#define NEBULA_INSTANCE_GROUP 4			// set a batch-internal copy of some specific settings
	#define NEBULA_DYNAMIC_OFFSET_GROUP 5		// set once per shader and is offset for each instance
	#define NEBULA_NUM_GROUPS (NEBULA_DYNAMIC_OFFSET_GROUP + 1)

	#define MAX_INPUT_ATTACHMENTS 32

	#define MAX_2D_TEXTURES 2048
	#define MAX_2D_MS_TEXTURES 64
	#define MAX_2D_ARRAY_TEXTURES 8
	#define MAX_CUBE_TEXTURES 128
	#define MAX_3D_TEXTURES 128

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