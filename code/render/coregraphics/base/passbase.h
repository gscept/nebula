#pragma once
//------------------------------------------------------------------------------
/**
	A pass contains a sequence of drawing operations, and begins with attaching
	and optionally clearing its render attachments.

	A pass contains sub-passes, which binds a subset of the attachments bound 
	to the pass. Sub-passes cannot perform a clear, instead the pass does that.

	Sub-passes are responsible for chaining together draws, and can efficiently
	optimize which order they are needed to render in, and which sub-passes has to
	rely on others. 

	Rendering is done in the actual sub-pass, while the pass itself is an entry point
	to a sequence of sub-passes. While it is valid to have only one sub-pass, it is
	probably more efficient to bind all possible combinations of render targets in the pass
	then have a subpass select which ones will be used.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "coregraphics/rendertexture.h"
#include "coregraphics/framebatchtype.h"
namespace Base
{
class PassBase : public Core::RefCounted
{
	__DeclareClass(PassBase);
public:
	struct Subpass;
	enum AttachmentFlagBits
	{
		NoFlags = 0,
		Clear = 1 << 0,
		ClearStencil = 1 << 1,
		Load = 1 << 2,
		LoadStencil = 1 << 3,
		Store = 1 << 4,
		StoreStencil = 1 << 5
	};

	/// constructor
	PassBase();
	/// destructor
	virtual ~PassBase();

	/// set depth-stencil attachment
	void SetDepthStencilAttachment(const CoreGraphics::RenderTextureId depthStencilAttachment);
	/// set depth-stencil clear
	void SetDepthStencilClear(float depth, uint stencil);
	/// set depth-stencil flags
	void SetDepthStencilFlags(const AttachmentFlagBits& flags);
	/// add color attachment
	void AddColorAttachment(const CoreGraphics::RenderTextureId colorAttachment);
	/// get number of color attachments
	const SizeT GetNumColorAttachments() const;
	/// get color attachment
	const CoreGraphics::RenderTextureId GetColorAttachment(const IndexT index);
	/// set color attachment clear
	void SetColorAttachmentClear(const IndexT index, const Math::float4& clearValue);
	/// set color attachment flags
	void SetColorAttachmentFlags(const IndexT index, const AttachmentFlagBits& flags);
	/// add subpass
	void AddSubpass(const Subpass& subpass);

	/// setup pass
	void Setup();
	/// discard pass
	void Discard();

	/// starts pass
	void Begin();
	/// begin a batch
	void BeginBatch(CoreGraphics::FrameBatchType::Code batchType);
	/// end current batch
	void EndBatch();
	/// progress to next subpass
	void NextSubpass();
	/// ends pass
	void End();

	/// handle window resizing
	void OnWindowResized();

	/// get subpasses
	const Util::Array<Subpass>& GetSubpasses() const;

	struct Subpass
	{
		Util::Array<IndexT> attachments;
		Util::Array<IndexT> dependencies;
		Util::Array<IndexT> inputs;
		bool bindDepth;
		bool resolve;

		Subpass() : bindDepth(false), resolve(false) {};
	};

protected:
	Util::Array<CoreGraphics::RenderTextureId> colorAttachments;
	Util::Array<Math::float4> colorAttachmentClears;
	Util::Array<AttachmentFlagBits> colorAttachmentFlags;
	Util::Array<Subpass> subpasses;
	CoreGraphics::FrameBatchType::Code batchType;

	CoreGraphics::RenderTextureId depthStencilAttachment;
	float clearDepth;
	uint clearStencil;
	AttachmentFlagBits depthStencilFlags;

	uint currentSubpass;
	bool inBegin;
	bool inBatch;
};

//------------------------------------------------------------------------------
/**
*/
inline void
PassBase::SetDepthStencilAttachment(const CoreGraphics::RenderTextureId depthStencilAttachment)
{
	this->depthStencilAttachment = depthStencilAttachment;
}

//------------------------------------------------------------------------------
/**
*/
inline void
PassBase::SetDepthStencilClear(float depth, uint stencil)
{
	this->clearDepth = depth;
	this->clearStencil = stencil;
}

//------------------------------------------------------------------------------
/**
*/
inline void
PassBase::SetDepthStencilFlags(const AttachmentFlagBits& flags)
{
	this->depthStencilFlags = flags;
}

//------------------------------------------------------------------------------
/**
*/
inline void
PassBase::AddColorAttachment(const CoreGraphics::RenderTextureId colorAttachment)
{
	this->colorAttachments.Append(colorAttachment);
	this->colorAttachmentClears.Append(Math::float4(0));
	this->colorAttachmentFlags.Append(NoFlags);
}

//------------------------------------------------------------------------------
/**
*/
inline const SizeT
PassBase::GetNumColorAttachments() const
{
	return this->colorAttachments.Size();
}

//------------------------------------------------------------------------------
/**
*/
inline const CoreGraphics::RenderTextureId
PassBase::GetColorAttachment(const IndexT index)
{
	return this->colorAttachments[index];
}

//------------------------------------------------------------------------------
/**
*/
inline void
PassBase::SetColorAttachmentClear(const IndexT index, const Math::float4& clearValue)
{
	this->colorAttachmentClears[index] = clearValue;
}

//------------------------------------------------------------------------------
/**
*/
inline void
PassBase::SetColorAttachmentFlags(const IndexT index, const AttachmentFlagBits& flags)
{
	this->colorAttachmentFlags[index] = flags;
}

//------------------------------------------------------------------------------
/**
*/
inline void
PassBase::AddSubpass(const Subpass& subpass)
{
	this->subpasses.Append(subpass);
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<PassBase::Subpass>&
PassBase::GetSubpasses() const
{
	return this->subpasses;
}
} // namespace Base