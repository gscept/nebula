#pragma once
//------------------------------------------------------------------------------
/**
    @file coregraphics/config.h
    
    Compile time configuration options for the CoreGraphics subsystem.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/types.h"

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
	ShaderVariableIdType,
	CommandBufferIdType,
	RenderTextureIdType,
	MeshIdType,
	EventIdType,
	BarrierIdType,
	WindowIdType,
	PassIdType
};

enum CoreGraphicsQueueType
{
	GraphicsQueueType,
	ComputeQueueType,
	TransferQueueType,
	SparseQueueType
};

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
	#define NEBULAT_OBJECT_TRANSFORM_GROUP 5	// set for all objects before rendering

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