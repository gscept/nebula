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
#include "coregraphics/shaderrwtexture.h"
#include "coregraphics/shaderrwbuffer.h"
#include "coregraphics/cmdbuffer.h"
#include <tuple>
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
	VertexShader = (1 << 1),	// blocks vertex shader
	HullShader = (1 << 2),		// blocks hull (tessellation control) shader
	DomainShader = (1 << 3),	// blocks domain (tessellation evaluation) shader
	GeometryShader = (1 << 4),	// blocks geometry shader
	PixelShader = (1 << 5),		// blocks pixel shader
	ComputeShader = (1 << 6),	// blocks compute shaders to complete

	VertexInput = (1 << 7),		// blocks vertex input
	EarlyDepth = (1 << 8),		// blocks early fragment test
	LateDepth = (1 << 9),		// blocks late fragment test

	Transfer = (1 << 10),		// blocks transfers
	Host = (1 << 11),			// blocks host operations
	PassOutput = (1 << 12),		// blocks outputs from render texture attachments		

	Top = (1 << 13),			// blocks start of pipeline
	Bottom = (1 << 14)			// blocks end of pipeline
};

enum class BarrierAccess
{
	NoAccess = (1 << 0),
	IndirectRead = (1 << 1),			// indirect buffers are read
	IndexRead = (1 << 2),				// index buffers are read
	VertexRead = (1 << 3),				// vertex buffers are read
	UniformRead = (1 << 4),				// uniforms are read
	InputAttachmentRead = (1 << 5),		// input attachments (cross-pass attachments) are read
	ShaderRead = (1 << 6),				// shader reads  (compute?)
	ShaderWrite = (1 << 7),				// shader writes (compute?)
	ColorAttachmentRead = (1 << 8),		// color attachments (render textures) are read
	ColorAttachmentWrite = (1 << 9),	// color attachments (render textures) are written
	DepthRead = (1 << 10),				// depth-stencil attachments are read
	DepthWrite = (1 << 11),				// depth-stencil attachments are written
	TransferRead = (1 << 12),			// transfers are read
	TransferWrite = (1 << 13),			// transfers are written
	HostRead = (1 << 14),				// host reads
	HostWrite = (1 << 15),				// host writes
	MemoryRead = (1 << 16),				// memory is read locally
	MemoryWrite = (1 << 17)				// memory is written locally
};

__ImplementEnumBitOperators(BarrierDependency);
__ImplementEnumComparisonOperators(BarrierDependency);
__ImplementEnumBitOperators(BarrierAccess);
__ImplementEnumComparisonOperators(BarrierAccess);

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

/// insert barrier into command buffer
void InsertBarrier(const BarrierId id, const CmdBufferId cmd);

//------------------------------------------------------------------------------
/**
*/
inline BarrierDependency
BarrierDependencyFromString(const Util::String& str)
{
	if (str == "VertexShader")			return BarrierDependency::VertexShader;
	else if (str == "HullShader")		return BarrierDependency::HullShader;
	else if (str == "DomainShader")		return BarrierDependency::DomainShader;
	else if (str == "GeometryShader")	return BarrierDependency::GeometryShader;
	else if (str == "PixelShader")		return BarrierDependency::PixelShader;
	else if (str == "ComputeShader")	return BarrierDependency::ComputeShader;
	else if (str == "VertexInput")		return BarrierDependency::VertexInput;
	else if (str == "EarlyDepth")		return BarrierDependency::EarlyDepth;
	else if (str == "LateDepth")		return BarrierDependency::LateDepth;
	else if (str == "Transfer")			return BarrierDependency::Transfer;
	else if (str == "Host")				return BarrierDependency::Host;
	else if (str == "PassOutput")		return BarrierDependency::PassOutput;
	else if (str == "Top")				return BarrierDependency::Top;
	else if (str == "Bottom")			return BarrierDependency::Bottom;
	else
	{
		n_error("Invalid dependency string '%s'\n", str.AsCharPtr());
		return BarrierDependency::NoDependencies;
	}
}

//------------------------------------------------------------------------------
/**
*/
inline BarrierAccess
BarrierAccessFromString(const Util::String& str)
{
	if (str == "IndirectRead")					return BarrierAccess::IndirectRead;
	else if (str == "IndexRead")				return BarrierAccess::IndexRead;
	else if (str == "VertexRead")				return BarrierAccess::VertexRead;
	else if (str == "UniformRead")				return BarrierAccess::UniformRead;
	else if (str == "InputAttachmentRead")		return BarrierAccess::InputAttachmentRead;
	else if (str == "ShaderRead")				return BarrierAccess::ShaderRead;
	else if (str == "ShaderWrite")				return BarrierAccess::ShaderWrite;
	else if (str == "ColorAttachmentRead")		return BarrierAccess::ColorAttachmentRead;
	else if (str == "ColorAttachmentWrite")		return BarrierAccess::ColorAttachmentWrite;
	else if (str == "DepthRead")				return BarrierAccess::DepthRead;
	else if (str == "DepthWrite")				return BarrierAccess::DepthWrite;
	else if (str == "TransferRead")				return BarrierAccess::TransferRead;
	else if (str == "TransferWrite")			return BarrierAccess::TransferWrite;
	else if (str == "HostRead")					return BarrierAccess::HostRead;
	else if (str == "HostWrite")				return BarrierAccess::HostWrite;
	else if (str == "MemoryRead")				return BarrierAccess::MemoryRead;
	else if (str == "MemoryWrite")				return BarrierAccess::MemoryWrite;
	else
	{
		n_error("Invalid access string '%s'\n", str.AsCharPtr());
		return BarrierAccess::NoAccess;
	}
}
} // namespace CoreGraphics
