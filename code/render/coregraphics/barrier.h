#pragma once
//------------------------------------------------------------------------------
/**
	A barrier is a memory barrier between two GPU operations, 
	and thus allows for a guarantee of concurrency.

	(C)2017-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/id.h"
#include "util/array.h"
#include "coregraphics/rendertexture.h"
#include "coregraphics/shaderrwtexture.h"
#include "coregraphics/shaderrwbuffer.h"
#include "coregraphics/cmdbuffer.h"
#include "coregraphics/config.h"
#include <tuple>
namespace CoreGraphics
{

enum class BarrierDomain
{
	Global,
	Pass
};

enum class BarrierStage
{
	NoDependencies = (1 << 0),
	VertexShader = (1 << 1),	// blocks vertex shader
	HullShader = (1 << 2),		// blocks hull (tessellation control) shader
	DomainShader = (1 << 3),	// blocks domain (tessellation evaluation) shader
	GeometryShader = (1 << 4),	// blocks geometry shader
	PixelShader = (1 << 5),		// blocks pixel shader
	ComputeShader = (1 << 6),	// blocks compute shaders to complete
	AllGraphicsShaders	= VertexShader | HullShader | DomainShader | GeometryShader | PixelShader,

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


__ImplementEnumBitOperators(BarrierStage);
__ImplementEnumComparisonOperators(BarrierStage);
__ImplementEnumBitOperators(BarrierAccess);
__ImplementEnumComparisonOperators(BarrierAccess);


#define __BeginEnumString(clazz) inline clazz clazz##FromString(const Util::String& str) {
#define __EnumString(clazz, x) if (str == #x) return clazz::x;
#define __EndEnumString(clazz) return (clazz)0; }

#define __BeginStringEnum(clazz) inline Util::String clazz##ToString(const clazz val) { switch (val) {
#define __StringEnum(clazz, x) case clazz##x: return #x;
#define __EndStringEnum() default: n_error("No enum value for %d\n", val); return ""; } }

ID_24_8_TYPE(BarrierId);

struct ImageSubresourceInfo
{
	ImageAspect aspect;
	uint mip, mipCount, layer, layerCount;

	ImageSubresourceInfo() :
		aspect(ImageAspect::ColorBits | ImageAspect::DepthBits | ImageAspect::StencilBits),
		mip(0),
		mipCount(1),
		layer(0),
		layerCount(1)
	{}

	const bool Overlaps(const ImageSubresourceInfo& rhs) const
	{
		return ((this->aspect & rhs.aspect) != 0) && (this->mip <= rhs.mip && this->mip + this->mipCount >= rhs.mip) && (this->layer <= rhs.layer && this->layer + this->layerCount >= rhs.layer);
	}

};
struct BarrierCreateInfo
{
	BarrierDomain domain;
	BarrierStage leftDependency;
	BarrierStage rightDependency;
	Util::Array<std::tuple<RenderTextureId, ImageSubresourceInfo, ImageLayout, ImageLayout, BarrierAccess, BarrierAccess>> renderTextures;
	Util::Array<std::tuple<ShaderRWBufferId, BarrierAccess, BarrierAccess>> shaderRWBuffers;
	Util::Array<std::tuple<ShaderRWTextureId, ImageSubresourceInfo, ImageLayout, ImageLayout, BarrierAccess, BarrierAccess>> shaderRWTextures;
};

/// create barrier object
BarrierId CreateBarrier(const BarrierCreateInfo& info);
/// destroy barrier object
void DestroyBarrier(const BarrierId id);

/// insert barrier into command buffer
void BarrierInsert(const BarrierId id, const CoreGraphicsQueueType queue);
/// reset resources previously set in barrier
void BarrierReset(const BarrierId id);

//------------------------------------------------------------------------------
/**
*/
inline BarrierStage
BarrierDependencyFromString(const Util::String& str)
{
	if (str == "VertexShader")			return BarrierStage::VertexShader;
	else if (str == "HullShader")		return BarrierStage::HullShader;
	else if (str == "DomainShader")		return BarrierStage::DomainShader;
	else if (str == "GeometryShader")	return BarrierStage::GeometryShader;
	else if (str == "PixelShader")		return BarrierStage::PixelShader;
	else if (str == "ComputeShader")	return BarrierStage::ComputeShader;
	else if (str == "VertexInput")		return BarrierStage::VertexInput;
	else if (str == "EarlyDepth")		return BarrierStage::EarlyDepth;
	else if (str == "LateDepth")		return BarrierStage::LateDepth;
	else if (str == "Transfer")			return BarrierStage::Transfer;
	else if (str == "Host")				return BarrierStage::Host;
	else if (str == "PassOutput")		return BarrierStage::PassOutput;
	else if (str == "Top")				return BarrierStage::Top;
	else if (str == "Bottom")			return BarrierStage::Bottom;
	else
	{
		n_error("Invalid dependency string '%s'\n", str.AsCharPtr());
		return BarrierStage::NoDependencies;
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

//------------------------------------------------------------------------------
/**
*/
inline ImageAspect
ImageAspectFromString(const Util::String& str)
{
	if (str == "Color")				return ImageAspect::ColorBits;
	else if (str == "Depth")		return ImageAspect::DepthBits;
	else if (str == "Stencil")		return ImageAspect::StencilBits;
	else if (str == "Metadata")		return ImageAspect::MetaBits;
	else if (str == "Plane0")		return ImageAspect::Plane0Bits;
	else if (str == "Plane1")		return ImageAspect::Plane1Bits;
	else if (str == "Plane2")		return ImageAspect::Plane2Bits;
	else
	{
		n_error("Invalid access string '%s'\n", str.AsCharPtr());
		return ImageAspect::ColorBits;
	}
}


//------------------------------------------------------------------------------
/**
*/
inline ImageLayout
ImageLayoutFromString(const Util::String& str)
{
	if (str == "Undefined")					return ImageLayout::Undefined;
	else if (str == "General")				return ImageLayout::General;
	else if (str == "ColorRenderTexture")	return ImageLayout::ColorRenderTexture;
	else if (str == "DepthRenderTexture")	return ImageLayout::DepthStencilRenderTexture;
	else if (str == "StencilRead")			return ImageLayout::DepthStencilRead;
	else if (str == "ShaderRead")			return ImageLayout::ShaderRead;
	else if (str == "TransferSource")		return ImageLayout::TransferSource;
	else if (str == "TransferDestination")	return ImageLayout::TransferDestination;
	else if (str == "Preinitialized")		return ImageLayout::Preinitialized;
	else if (str == "Present")				return ImageLayout::Present;
	return ImageLayout::Undefined;
}
} // namespace CoreGraphics
