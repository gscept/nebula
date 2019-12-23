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
#include "coregraphics/texture.h"
#include "coregraphics/shaderrwbuffer.h"
#include "coregraphics/commandbuffer.h"
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
	VertexInput = (1 << 1),				// blocks vertex input
	VertexShader = (1 << 2),			// blocks vertex shader
	HullShader = (1 << 3),				// blocks hull (tessellation control) shader
	DomainShader = (1 << 4),			// blocks domain (tessellation evaluation) shader
	GeometryShader = (1 << 5),			// blocks geometry shader
	EarlyDepth = (1 << 6),				// blocks early fragment test
	PixelShader = (1 << 7),				// blocks pixel shader
	LateDepth = (1 << 8),				// blocks late fragment test
	PassOutput = (1 << 9),				// blocks outputs from render texture attachments		
	AllGraphicsShaders = VertexShader | HullShader | DomainShader | GeometryShader | PixelShader,

	ComputeShader = (1 << 10),			// blocks compute shaders to complete

	Transfer = (1 << 11),				// blocks transfers
	Host = (1 << 12),					// blocks host operations

	Top = (1 << 13),					// blocks start of pipeline 
	Bottom = (1 << 14)					// blocks end of pipeline
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
	DepthAttachmentRead = (1 << 10),	// depth-stencil attachments are read
	DepthAttachmentWrite = (1 << 11),	// depth-stencil attachments are written
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
	CoreGraphics::ImageAspect aspect;
	uint mip, mipCount, layer, layerCount;

	ImageSubresourceInfo() :
		aspect(CoreGraphics::ImageAspect::ColorBits),
		mip(0),
		mipCount(1),
		layer(0),
		layerCount(1)
	{}

	ImageSubresourceInfo(CoreGraphics::ImageAspect aspect, uint mip, uint mipCount, uint layer, uint layerCount) :
		aspect(aspect),
		mip(mip),
		mipCount(mipCount),
		layer(layer),
		layerCount(layerCount)
	{}

	static ImageSubresourceInfo ColorNoMipNoLayer()
	{
		return ImageSubresourceInfo(CoreGraphics::ImageAspect::ColorBits, 0, 1, 0, 1);
	}

	static ImageSubresourceInfo ColorNoMip(uint layerCount)
	{
		return ImageSubresourceInfo(CoreGraphics::ImageAspect::ColorBits, 0, 1, 0, layerCount);
	}

	static ImageSubresourceInfo DepthStencilNoMipNoLayer()
	{
		return ImageSubresourceInfo(CoreGraphics::ImageAspect::DepthBits | CoreGraphics::ImageAspect::StencilBits, 0, 1, 0, 1);
	}

	static ImageSubresourceInfo DepthStencilNoMip(uint layerCount)
	{
		return ImageSubresourceInfo(CoreGraphics::ImageAspect::DepthBits | CoreGraphics::ImageAspect::StencilBits, 0, 1, 0, layerCount);
	}

	const bool Overlaps(const ImageSubresourceInfo& rhs) const
	{
		return ((this->aspect & rhs.aspect) != 0) && (this->mip <= rhs.mip && this->mip + this->mipCount >= rhs.mip) && (this->layer <= rhs.layer && this->layer + this->layerCount >= rhs.layer);
	}
};

struct BufferSubresourceInfo
{
	uint offset, size;

	BufferSubresourceInfo() :
		offset(0),
		size(-1)
	{}

	const bool Overlaps(const BufferSubresourceInfo& rhs) const
	{
		return (this->offset <= rhs.offset && this->offset + this->size >= rhs.offset);
	}
};

struct TextureBarrier
{
	TextureId tex;
	ImageSubresourceInfo subres;
	CoreGraphics::ImageLayout fromLayout;
	CoreGraphics::ImageLayout toLayout;
	BarrierAccess fromAccess;
	BarrierAccess toAccess;
};

struct BufferBarrier
{
	ShaderRWBufferId buf;
	BarrierAccess fromAccess;
	BarrierAccess toAccess;
	IndexT offset;
	SizeT size; // set to -1 to use whole buffer
};

struct BarrierCreateInfo
{
	Util::StringAtom name;
	BarrierDomain domain;
	BarrierStage leftDependency;
	BarrierStage rightDependency;
	Util::Array<TextureBarrier> textures;
	Util::Array<BufferBarrier> rwBuffers;
};

/// create barrier object
BarrierId CreateBarrier(const BarrierCreateInfo& info);
/// destroy barrier object
void DestroyBarrier(const BarrierId id);

/// insert barrier into command buffer
void BarrierInsert(const BarrierId id, const CoreGraphics::QueueType queue);
/// reset resources previously set in barrier
void BarrierReset(const BarrierId id);
/// create and insert a barrier immediately, without allocating an object
void BarrierInsert(
	const CoreGraphics::QueueType queue, 
	CoreGraphics::BarrierStage fromStage, 
	CoreGraphics::BarrierStage toStage, 
	CoreGraphics::BarrierDomain domain,
	const Util::FixedArray<TextureBarrier>& textures, 
	const Util::FixedArray<BufferBarrier>& rwBuffers, 
	const char* name = nullptr);

//------------------------------------------------------------------------------
/**
*/
inline BarrierStage
BarrierStageFromString(const Util::String& str)
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
	else if (str == "DepthAttachmentRead")		return BarrierAccess::DepthAttachmentRead;
	else if (str == "DepthAttachmentWrite")		return BarrierAccess::DepthAttachmentWrite;
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
inline CoreGraphics::ImageAspect
ImageAspectFromString(const Util::String& str)
{
	Util::Array<Util::String> comps = str.Tokenize("|");
	CoreGraphics::ImageAspect aspect = CoreGraphics::ImageAspect(0x0);
	for (IndexT i = 0; i < comps.Size(); i++)
	{
		if (comps[i] == "Color")			aspect |= CoreGraphics::ImageAspect::ColorBits;
		else if (comps[i] == "Depth")		aspect |= CoreGraphics::ImageAspect::DepthBits;
		else if (comps[i] == "Stencil")		aspect |= CoreGraphics::ImageAspect::StencilBits;
		else if (comps[i] == "Metadata")	aspect |= CoreGraphics::ImageAspect::MetaBits;
		else if (comps[i] == "Plane0")		aspect |= CoreGraphics::ImageAspect::Plane0Bits;
		else if (comps[i] == "Plane1")		aspect |= CoreGraphics::ImageAspect::Plane1Bits;
		else if (comps[i] == "Plane2")		aspect |= CoreGraphics::ImageAspect::Plane2Bits;
		else
		{
			n_error("Invalid access string '%s'\n", comps[i].AsCharPtr());
			return CoreGraphics::ImageAspect::ColorBits;
		}
	}
	return aspect;
}

//------------------------------------------------------------------------------
/**
*/
inline CoreGraphics::ImageLayout
ImageLayoutFromString(const Util::String& str)
{
	if (str == "Undefined")					return CoreGraphics::ImageLayout::Undefined;
	else if (str == "General")				return CoreGraphics::ImageLayout::General;
	else if (str == "ColorRenderTexture")	return CoreGraphics::ImageLayout::ColorRenderTexture;
	else if (str == "DepthRenderTexture")	return CoreGraphics::ImageLayout::DepthStencilRenderTexture;
	else if (str == "DepthStencilRead")		return CoreGraphics::ImageLayout::DepthStencilRead;
	else if (str == "ShaderRead")			return CoreGraphics::ImageLayout::ShaderRead;
	else if (str == "TransferSource")		return CoreGraphics::ImageLayout::TransferSource;
	else if (str == "TransferDestination")	return CoreGraphics::ImageLayout::TransferDestination;
	else if (str == "Preinitialized")		return CoreGraphics::ImageLayout::Preinitialized;
	else if (str == "Present")				return CoreGraphics::ImageLayout::Present;
	return CoreGraphics::ImageLayout::Undefined;
}
} // namespace CoreGraphics
