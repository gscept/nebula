#pragma once
//------------------------------------------------------------------------------
/**
    @file coregraphics/config.h
    
    Compile time configuration options for the CoreGraphics subsystem.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "util/string.h"
#include "core/rttimacros.h"

enum CoreGraphicsIdType
{
	VertexBufferIdType,
	IndexBufferIdType,
	TextureIdType,
	VertexLayoutIdType,
	ConstantBufferIdType,
	ShaderRWBufferIdType,
	ShaderRWTextureIdType,
	ShaderStorageBufferIdType,
	ShaderIdType,
	ShaderProgramIdType,
	ShaderStateIdType,
	ShaderInstanceIdType,
	ShaderConstantIdType,
	CommandBufferIdType,
	RenderTextureIdType,
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
	MaterialIdType
};

enum CoreGraphicsQueueType
{
	GraphicsQueueType,
	ComputeQueueType,
	TransferQueueType,
	SparseQueueType,

	NumQueueTypes
};

enum CoreGraphicsShaderVisibility
{
	InvalidVisibility			= 0,
	VertexShaderVisibility		= 1 << 0,
	HullShaderVisibility		= 1 << 2,
	DomainShaderVisibility		= 1 << 3,
	GeometryShaderVisibility	= 1 << 4,
	PixelShaderVisibility		= 1 << 5,
	ComputeShaderVisibility		= 1 << 6,
	AllVisibility				= VertexShaderVisibility | HullShaderVisibility | DomainShaderVisibility | GeometryShaderVisibility | PixelShaderVisibility | ComputeShaderVisibility
};
__ImplementEnumBitOperators(CoreGraphicsShaderVisibility);

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
__ImplementEnumBitOperators(ImageAspect);
__ImplementEnumComparisonOperators(ImageAspect);

//------------------------------------------------------------------------------
/**
*/
inline CoreGraphicsShaderVisibility
ShaderVisibilityFromString(const Util::String& str)
{
	Util::Array<Util::String> components = str.Tokenize("|");
	CoreGraphicsShaderVisibility ret = InvalidVisibility;
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
inline CoreGraphicsQueueType
CoreGraphicsQueueTypeFromString(const Util::String& str)
{
	if (str == "Graphics")		return GraphicsQueueType;
	else if (str == "Compute")	return ComputeQueueType;
	else if (str == "Transfer") return TransferQueueType;
	else if (str == "Sparse")	return SparseQueueType;
	return GraphicsQueueType;
}



#define SHADER_POSTEFFECT_DEFAULT_FEATURE_MASK "Alt0"

#if !PUBLIC_BUILD
#define NEBULAT_GRAPHICS_DEBUG 1
#endif

//------------------------------------------------------------------------------
#if __DX11__ || __DX9__
    #if __DX11__
        #define SHADER_MODEL_5 (1)
    #endif
	#define COREGRAPHICS_PIXEL_CENTER_HALF_PIXEL (1)
	#define COREGRAPHICS_TRIANGLE_FRONT_FACE_CCW (1)

	#define NEBULA3_USEDIRECT3D9 (1)
	#define NEBULA3_USEDIRECT3D10 (0)

	#define NEBULA3_DIRECT3D_USENVPERFHUD (0)
	#define NEBULA3_DIRECT3D_DEBUG (0)

	#if NEBULA3_DIRECT3D_USENVPERFHUD
	#define NEBULA3_DIRECT3D_DEVICETYPE D3DDEVTYPE_REF
	#else
	#define NEBULA3_DIRECT3D_DEVICETYPE D3DDEVTYPE_HAL
	#endif
	
#elif __OGL4__
    #define SHADER_MODEL_5 (1)
    #ifdef _DEBUG
	    #define NEBULA3_OPENGL4_DEBUG (1)
    #else
        #define	NEBULA3_OPENGL4_DEBUG (0)
    #endif
#elif __VULKAN__
	#define COREGRAPHICS_TRIANGLE_FRONT_FACE_CCW (1)
	// define the same descriptor set slots as we do in the shaders
	#define NEBULAT_TICK_GROUP 0				// set per tick (once for all views) by the system
	#define NEBULAT_FRAME_GROUP 1				// set per frame (once per view) by the system
	#define NEBULAT_PASS_GROUP 2				// set per pass by the system
	#define NEBULAT_BATCH_GROUP 3				// set per batch (material settings or system stuff)
	#define NEBULAT_INSTANCE_GROUP 4			// set a batch-internal copy of some specific settings
	#define NEBULAT_DYNAMIC_OFFSET_GROUP 5		// set once per shader and is offset for each instance

	#define MAX_INPUT_ATTACHMENTS 32

	#define MAX_2D_TEXTURES 2048
	#define MAX_2D_MS_TEXTURES 64
	#define MAX_CUBE_TEXTURES 128
	#define MAX_3D_TEXTURES 128

	#define MAX_2D_IMAGES 64
	#define MAX_CUBE_IMAGES 64
	#define MAX_3D_IMAGES 64

	#define SHADER_MODEL_5 (1)
	#ifdef _DEBUG
		#define NEBULAT_VULKAN_DEBUG (1)
	#else
		#define NEBULAT_VULKAN_DEBUG (0)
	#endif
#if __X64__
	#define VK_DEVICE_SIZE_CONV(x) uint64_t(x)
#else
	#define VK_DEVICE_SIZE_CONV(x) uint32_t(x)
#endif
#endif
//------------------------------------------------------------------------------