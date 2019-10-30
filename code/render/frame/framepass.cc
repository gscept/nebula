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
	Util::Dictionary<CoreGraphics::ShaderRWTextureId, Util::Array<TextureDependency>>& rwTextures,
	Util::Dictionary<CoreGraphics::ShaderRWBufferId, Util::Array<BufferDependency>>& rwBuffers,
	Util::Dictionary<CoreGraphics::RenderTextureId, Util::Array<TextureDependency>>& renderTextures)
{
	CompiledImpl* myCompiled = (CompiledImpl*)this->AllocCompiled(allocator);

#if NEBULA_GRAPHICS_DEBUG
	myCompiled->name = this->name;
#endif

	Util::Array<FrameOp::Compiled*> subpassOps;
	for (IndexT i = 0; i < this->subpasses.Size(); i++)
	{
		this->subpasses[i]->Build(allocator, subpassOps, events, barriers, rwTextures, rwBuffers, renderTextures);
	}
	myCompiled->subpasses = subpassOps;
	this->compiled = myCompiled;
	this->SetupSynchronization(allocator, events, barriers, rwTextures, rwBuffers, renderTextures);

	// now, add all render textures used by this pass as a dependency for any future command
	const Util::Array<CoreGraphics::RenderTextureId>& attachments = CoreGraphics::PassGetAttachments(this->pass);
	for (IndexT i = 0; i < attachments.Size(); i++)
	{
		IndexT idx = renderTextures.FindIndex(attachments[i]);
		n_assert(idx != InvalidIndex);
		Util::Array<TextureDependency>& deps = renderTextures.ValueAtIndex(idx);
		CoreGraphicsImageLayout layout = CoreGraphics::RenderTextureGetLayout(attachments[i]);
		uint layers = CoreGraphics::RenderTextureGetNumLayers(attachments[i]);
		uint mips = CoreGraphics::RenderTextureGetNumMips(attachments[i]);
		CoreGraphics::ImageSubresourceInfo subres{ 
			layout == CoreGraphicsImageLayout::DepthStencilRead ? CoreGraphicsImageAspect::DepthBits : CoreGraphicsImageAspect::ColorBits, 
			0, mips, 0, layers };
		TextureDependency dep{
			this->compiled, 
			this->queue, 
			layout, 
			layout == CoreGraphicsImageLayout::DepthStencilRead ? CoreGraphics::BarrierStage::LateDepth : CoreGraphics::BarrierStage::PassOutput,
			layout == CoreGraphicsImageLayout::DepthStencilRead ? CoreGraphics::BarrierAccess::DepthAttachmentWrite : CoreGraphics::BarrierAccess::ColorAttachmentWrite,
			DependencyIntent::Write, 
			this->index,
			subres};
		deps.Append(dep);
	}
	compiledOps.Append(myCompiled);
}

} // namespace Frame2