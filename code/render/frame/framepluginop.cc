//------------------------------------------------------------------------------
// framepluginop.cc
// (C) 2016-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "framepluginop.h"

namespace Frame
{

//------------------------------------------------------------------------------
/**
*/
FramePluginOp::FramePluginOp()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
FramePluginOp::~FramePluginOp()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
FramePluginOp::CompiledImpl::Run(const IndexT frameIndex)
{
	this->func(frameIndex);
}

//------------------------------------------------------------------------------
/**
*/
void
FramePluginOp::CompiledImpl::Discard()
{
	this->func = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
FrameOp::Compiled* 
FramePluginOp::AllocCompiled(Memory::ArenaAllocator<BIG_CHUNK>& allocator)
{
	CompiledImpl* ret = allocator.Alloc<CompiledImpl>();
	ret->func = this->func;
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
FramePluginOp::Build(
	Memory::ArenaAllocator<BIG_CHUNK>& allocator,
	Util::Array<FrameOp::Compiled*>& compiledOps,
	Util::Array<CoreGraphics::EventId>& events,
	Util::Array<CoreGraphics::BarrierId>& barriers,
	Util::Dictionary<CoreGraphics::ShaderRWBufferId, Util::Array<BufferDependency>>& rwBuffers,
	Util::Dictionary<CoreGraphics::TextureId, Util::Array<TextureDependency>>& textures)
{
	CompiledImpl* myCompiled = (CompiledImpl*)this->AllocCompiled(allocator);

	this->compiled = myCompiled;
	this->SetupSynchronization(allocator, events, barriers, rwBuffers, textures);
	compiledOps.Append(myCompiled);
}

} // namespace Frame2