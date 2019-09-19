//------------------------------------------------------------------------------
// framesubpass.cc
// (C) 2016-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "framesubpass.h"
#include "coregraphics/graphicsdevice.h"

using namespace CoreGraphics;
namespace Frame
{

//------------------------------------------------------------------------------
/**
*/
FrameSubpass::FrameSubpass()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
FrameSubpass::~FrameSubpass()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
FrameSubpass::AddOp(Frame::FrameOp* op)
{
	this->ops.Append(op);
}

//------------------------------------------------------------------------------
/**
*/
void
FrameSubpass::Discard()
{
	FrameOp::Discard();

	IndexT i;
	for (i = 0; i < this->ops.Size(); i++)
	{
		this->ops[i]->Discard();
	}
}

//------------------------------------------------------------------------------
/**
*/
void
FrameSubpass::CompiledImpl::Run(const IndexT frameIndex)
{
	IndexT i;

	// bind scissors and viewports, if any
	for (i = 0; i < this->viewports.Size(); i++) CoreGraphics::SetViewport(this->viewports[i], i);
	for (i = 0; i < this->scissors.Size(); i++) CoreGraphics::SetScissorRect(this->scissors[i], i);

	// run ops
	for (i = 0; i < this->ops.Size(); i++)
	{
		this->ops[i]->Run(frameIndex);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
FrameSubpass::CompiledImpl::Discard()
{
	for (IndexT i = 0; i < this->ops.Size(); i++)
		this->ops[i]->Discard();
}

//------------------------------------------------------------------------------
/**
*/
FrameOp::Compiled*
FrameSubpass::AllocCompiled(Memory::ArenaAllocator<BIG_CHUNK>& allocator)
{
	CompiledImpl* ret = allocator.Alloc<CompiledImpl>();
	ret->viewports = this->viewports;
	ret->scissors = this->scissors;
	// don't set ops here, we have to do it when we build
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
void 
FrameSubpass::Build(
	Memory::ArenaAllocator<BIG_CHUNK>& allocator,
	Util::Array<FrameOp::Compiled*>& compiledOps, 
	Util::Array<CoreGraphics::EventId>& events,
	Util::Array<CoreGraphics::BarrierId>& barriers,
	Util::Dictionary<CoreGraphics::ShaderRWTextureId, Util::Array<std::tuple<CoreGraphics::ImageSubresourceInfo, TextureDependency>>>& rwTextures,
	Util::Dictionary<CoreGraphics::ShaderRWBufferId, BufferDependency>& rwBuffers,
	Util::Dictionary<CoreGraphics::RenderTextureId, Util::Array<std::tuple<CoreGraphics::ImageSubresourceInfo, TextureDependency>>>& renderTextures)
{
	CompiledImpl* myCompiled = (CompiledImpl*)this->AllocCompiled(allocator);
	
	Util::Array<FrameOp::Compiled*> subpassOps;
	for (IndexT i = 0; i < this->ops.Size(); i++)
	{
		this->ops[i]->Build(allocator, subpassOps, events, barriers, rwTextures, rwBuffers, renderTextures);
	}
	myCompiled->ops = subpassOps;
	this->compiled = myCompiled;
	compiledOps.Append(myCompiled);
}

} // namespace Frame2