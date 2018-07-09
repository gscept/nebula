//------------------------------------------------------------------------------
// framepass.cc
// (C) 2016 Individual contributors, see AUTHORS file
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
	// begin pass
	PassBegin(this->pass);

	// run subpasses
	IndexT i;
	for (i = 0; i < this->subpasses.Size(); i++)
	{
		// progress to next subpass if not on first iteration
		if (i > 0) CoreGraphics::SetToNextSubpass();

		// execute contents of this subpass
		this->subpasses[i]->Run(frameIndex);		
	}

	// end pass
	PassEnd(this->pass);
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
FramePass::AllocCompiled(Memory::ChunkAllocator<0xFFFF>& allocator)
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
	Memory::ChunkAllocator<0xFFFF>& allocator, 
	Util::Array<FrameOp::Compiled*>& compiledOps, 
	Util::Array<CoreGraphics::EventId>& events,
	Util::Array<CoreGraphics::BarrierId>& barriers,
	Util::Array<CoreGraphics::SemaphoreId>& semaphores,
	Util::Dictionary<CoreGraphics::ShaderRWTextureId, Util::Array<std::tuple<CoreGraphics::ImageSubresourceInfo, TextureDependency>>>& rwTextures,
	Util::Dictionary<CoreGraphics::ShaderRWBufferId, BufferDependency>& rwBuffers,
	Util::Dictionary<CoreGraphics::RenderTextureId, Util::Array<std::tuple<CoreGraphics::ImageSubresourceInfo, TextureDependency>>>& renderTextures)
{
	CompiledImpl* myCompiled = (CompiledImpl*)this->AllocCompiled(allocator);

	Util::Array<FrameOp::Compiled*> subpassOps;
	for (IndexT i = 0; i < this->subpasses.Size(); i++)
	{
		this->subpasses[i]->Build(allocator, subpassOps, events, barriers, semaphores, rwTextures, rwBuffers, renderTextures);
	}
	myCompiled->subpasses = subpassOps;

	// don't add subpasses to array, since they will be called by the pass
	compiledOps.Append(myCompiled);
}

} // namespace Frame2