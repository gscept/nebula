//------------------------------------------------------------------------------
// framesubpass.cc
// (C) 2016-2020 Individual contributors, see AUTHORS file
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
FrameSubpass::OnWindowResized()
{

	IndexT i;
	for (i = 0; i < this->ops.Size(); i++)
	{
		this->ops[i]->OnWindowResized();
	}
}

//------------------------------------------------------------------------------
/**
*/
void
FrameSubpass::CompiledImpl::Run(const IndexT frameIndex)
{
	IndexT i;

#if NEBULA_GRAPHICS_DEBUG
	CoreGraphics::CommandBufferBeginMarker(GraphicsQueueType, NEBULA_MARKER_GREEN, this->name.Value());
#endif

	// run ops
	for (i = 0; i < this->ops.Size(); i++)
	{
		this->ops[i]->Run(frameIndex);
	}

#if NEBULA_GRAPHICS_DEBUG
	CoreGraphics::CommandBufferEndMarker(GraphicsQueueType);
#endif
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
#if NEBULA_GRAPHICS_DEBUG
	ret->name = this->name;
#endif
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
	Util::Dictionary<CoreGraphics::BufferId, Util::Array<BufferDependency>>& rwBuffers,
	Util::Dictionary<CoreGraphics::TextureId, Util::Array<TextureDependency>>& textures)
{
	CompiledImpl* myCompiled = (CompiledImpl*)this->AllocCompiled(allocator);
	
	Util::Array<FrameOp::Compiled*> subpassOps;
	for (IndexT i = 0; i < this->ops.Size(); i++)
	{
		this->ops[i]->Build(allocator, subpassOps, events, barriers, rwBuffers, textures);
	}
	myCompiled->ops = subpassOps;
	this->compiled = myCompiled;
	compiledOps.Append(myCompiled);
}

} // namespace Frame2