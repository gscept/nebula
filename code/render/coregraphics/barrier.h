#pragma once
//------------------------------------------------------------------------------
/**
	A barrier is a memory barrier between two GPU operations, 
	and thus allows for a guarantee of concurrency.

	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/id.h"
#include "util/array.h"
#include "coregraphics/rendertexture.h"
#include "coregraphics/shaderreadwritetexture.h"
#include "coregraphics/shaderreadwritebuffer.h"
namespace CoreGraphics
{

enum class BarrierDomain
{
	Global,
	Pass
};

enum class BarrierDependency
{
	NoDependencies = (1 << 0),
	VertexShader = (1 << 1),		// blocks vertex shader
	HullShader = (1 << 2),		// blocks hull (tessellation control) shader
	DomainShader = (1 << 3),		// blocks domain (tessellation evaluation) shader
	GeometryShader = (1 << 4),		// blocks geometry shader
	PixelShader = (1 << 5),		// blocks pixel shader
	ComputeShader = (1 << 6),		// blocks compute shaders to complete

	VertexInput = (1 << 7),		// blocks vertex input
	EarlyDepth = (1 << 8),		// blocks early fragment test
	LateDepth = (1 << 9),		// blocks late fragment test

	Transfer = (1 << 10),	// blocks transfers
	Host = (1 << 11),	// blocks host operations
	PassOutput = (1 << 12),	// blocks outputs from render texture attachments		

	Top = (1 << 13),	// blocks start of pipeline
	Bottom = (1 << 14)		// blocks end of pipeline
};

enum class BarrierAccess
{
	NoAccess = (1 << 0),
	IndirectRead = (1 << 1),	// indirect buffers are read
	IndexRead = (1 << 2),	// index buffers are read
	VertexRead = (1 << 3), // vertex buffers are read
	UniformRead = (1 << 4), // uniforms are read
	InputAttachmentRead = (1 << 5), // input attachments (cross-pass attachments) are read
	ShaderRead = (1 << 6), // shader reads  (compute?)
	ShaderWrite = (1 << 7), // shader writes (compute?)
	ColorAttachmentRead = (1 << 8), // color attachments (render textures) are read
	ColorAttachmentWrite = (1 << 9), // color attachments (render textures) are written
	DepthRead = (1 << 10), // depth-stencil attachments are read
	DepthWrite = (1 << 11), // depth-stencil attachments are written
	TransferRead = (1 << 12), // transfers are read
	TransferWrite = (1 << 13), // transfers are written
	HostRead = (1 << 14), // host reads
	HostWrite = (1 << 15), // host writes
	MemoryRead = (1 << 16), // memory is read locally
	MemoryWrite = (1 << 17)  // memory is written locally
};

__ImplementEnumBitOperators(BarrierDependency);
__ImplementEnumBitOperators(BarrierAccess);

ID_24_8_TYPE(BarrierId);

struct BarrierCreateInfo
{
	BarrierDomain domain;
	BarrierDependency leftDependency;
	BarrierDependency rightDependency;
	Util::Array<std::tuple<RenderTextureId, BarrierAccess, BarrierAccess>> renderTextureBarriers;
	Util::Array<std::tuple<ShaderRWBufferId, BarrierAccess, BarrierAccess>> shaderRWBuffers;
	Util::Array<std::tuple<ShaderRWTextureId, BarrierAccess, BarrierAccess>> shaderRWTextures;
};

/// create barrier object
BarrierId CreateBarrier(const BarrierCreateInfo& info);
/// destroy barrier object
void DestroyBarrier(const BarrierId id);
} // namespace CoreGraphics
