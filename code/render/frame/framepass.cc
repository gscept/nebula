//------------------------------------------------------------------------------
// framepass.cc
// (C) 2016-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "framepass.h"
#include "coregraphics/graphicsdevice.h"

using namespace CoreGraphics;
namespace Frame
{

//------------------------------------------------------------------------------
/**
*/
FramePass::FramePass()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
FramePass::~FramePass()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
FramePass::AddSubpass(FrameSubpass* subpass)
{
	this->subpasses.Append(subpass);
}

//------------------------------------------------------------------------------
/**
*/
void
FramePass::Discard()
{
	FrameOp::Discard();

	DiscardPass(this->pass);
	this->pass = PassId::Invalid();

	IndexT i;
	for (i = 0; i < this->subpasses.Size(); i++) this->subpasses[i]->Discard();
	this->subpasses.Clear();
}

//------------------------------------------------------------------------------
/**
*/
void
FramePass::CompiledImpl::Run(const IndexT frameIndex)
{

#if NEBULA_GRAPHICS_DEBUG
	CoreGraphics::CommandBufferBeginMarker(GraphicsQueueType, NEBULA_MARKER_GREEN, this->name.Value());
#endif

	// begin pass
	PassBegin(this->pass);
	
	// run subpasses
	IndexT i;
	for (i = 0; i < this->subpasses.Size(); i++)
	{
		// progress to next subpass if not on first iteration
		if (i > 0) PassNextSubpass(this->pass);

		// execute contents of this subpass and synchronize
		// note that we overload the cross queue sync so we do it outside the render pass
		this->subpasses[i]->QueuePreSync();
		this->subpasses[i]->Run(frameIndex);
		this->subpasses[i]->QueuePostSync();
	}

	// end pass
	PassEnd(this->pass);

#if NEBULA_GRAPHICS_DEBUG
	CoreGraphics::CommandBufferEndMarker(GraphicsQueueType);
#endif
}

//------------------------------------------------------------------------------
/**
*/
void
FramePass::CompiledImpl::Discard()
{
	for (IndexT i = 0; i < this->subpasses.Size(); i++)
		this->subpasses[i]->Discard();
}

//------------------------------------------------------------------------------
/**
*/
void
FramePass::OnWindowResized()
{
	// resize pass
	PassWindowResizeCallback(this->pass);
}

//------------------------------------------------------------------------------
/**
*/
FrameOp::Compiled* 
FramePass::AllocCompiled(Memory::ArenaAllocator<BIG_CHUNK>& allocator)
{
	CompiledImpl* ret = allocator.Alloc<CompiledImpl>();
	ret->pass = this->pass;
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
void 
FramePass::Build(
	Memory::ArenaAllocator<BIG_CHUNK>& allocator,
	Util::Array<FrameOp::Compiled*>& compiledOps, 
	Util::Array<CoreGraphics::EventId>& events,
	Util::Array<CoreGraphics::BarrierId>& barriers,
	Util::Dictionary<CoreGraphics::ShaderRWBufferId, Util::Array<BufferDependency>>& rwBuffers,
	Util::Dictionary<CoreGraphics::TextureId, Util::Array<TextureDependency>>& textures)
{
	CompiledImpl* myCompiled = (CompiledImpl*)this->AllocCompiled(allocator);

#if NEBULA_GRAPHICS_DEBUG
	myCompiled->name = this->name;
#endif

	Util::Array<FrameOp::Compiled*> subpassOps;
	for (IndexT i = 0; i < this->subpasses.Size(); i++)
	{
		this->subpasses[i]->Build(allocator, subpassOps, events, barriers, rwBuffers, textures);
	}
	myCompiled->subpasses = subpassOps;
	this->compiled = myCompiled;
	this->SetupSynchronization(allocator, events, barriers, rwBuffers, textures);

	// first add dependency for color attachments
	const Util::Array<CoreGraphics::TextureId>& attachments = CoreGraphics::PassGetAttachments(this->pass);
	for (IndexT i = 0; i < attachments.Size(); i++)
	{
		IndexT idx = textures.FindIndex(attachments[i]);
		n_assert(idx != InvalidIndex);
		Util::Array<TextureDependency>& deps = textures.ValueAtIndex(idx);
		uint layers = CoreGraphics::TextureGetNumLayers(attachments[i]);
		uint mips = CoreGraphics::TextureGetNumMips(attachments[i]);
		CoreGraphics::ImageSubresourceInfo subres{ 
			CoreGraphicsImageAspect::ColorBits,
			0, mips, 0, layers };
		TextureDependency dep{
			this->compiled, 
			this->queue, 
			CoreGraphicsImageLayout::ShaderRead,
			CoreGraphics::BarrierStage::PassOutput,
			CoreGraphics::BarrierAccess::ColorAttachmentWrite,
			DependencyIntent::Write, 
			this->index,
			subres};
		deps.Append(dep);
	}

	// then add potential dependency for depth-stencil attachment
	CoreGraphics::TextureId depthStencilAttachment = CoreGraphics::PassGetDepthStencilAttachment(this->pass);
	if (depthStencilAttachment != CoreGraphics::TextureId::Invalid())
	{
		IndexT idx = textures.FindIndex(depthStencilAttachment);
		n_assert(idx != InvalidIndex);
		Util::Array<TextureDependency>& deps = textures.ValueAtIndex(idx);
		uint layers = CoreGraphics::TextureGetNumLayers(depthStencilAttachment);
		uint mips = CoreGraphics::TextureGetNumMips(depthStencilAttachment);
		CoreGraphics::ImageSubresourceInfo subres{
			CoreGraphicsImageAspect::DepthBits | CoreGraphicsImageAspect::StencilBits,
			0, mips, 0, layers };
		TextureDependency dep{
			this->compiled,
			this->queue,
			CoreGraphicsImageLayout::DepthStencilRead,
			CoreGraphics::BarrierStage::LateDepth,
			CoreGraphics::BarrierAccess::DepthAttachmentWrite,
			DependencyIntent::Write,
			this->index,
			subres };
		deps.Append(dep);
	}
	compiledOps.Append(myCompiled);
}

} // namespace Frame2