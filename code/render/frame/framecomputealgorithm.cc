//------------------------------------------------------------------------------
// framecomputealgorithm.cc
// (C) 2016-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "framecomputealgorithm.h"

namespace Frame
{

//------------------------------------------------------------------------------
/**
*/
FrameComputeAlgorithm::FrameComputeAlgorithm()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
FrameComputeAlgorithm::~FrameComputeAlgorithm()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
FrameComputeAlgorithm::CompiledImpl::Run(const IndexT frameIndex)
{
	this->func(frameIndex);
}

//------------------------------------------------------------------------------
/**
*/
void
FrameComputeAlgorithm::CompiledImpl::Discard()
{
	this->func = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
FrameOp::Compiled* 
FrameComputeAlgorithm::AllocCompiled(Memory::ArenaAllocator<BIG_CHUNK>& allocator)
{
	CompiledImpl* ret = allocator.Alloc<CompiledImpl>();
	ret->func = this->func;
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
FrameComputeAlgorithm::Build(
	Memory::ArenaAllocator<BIG_CHUNK>& allocator,
	Util::Array<FrameOp::Compiled*>& compiledOps,
	Util::Array<CoreGraphics::EventId>& events,
	Util::Array<CoreGraphics::BarrierId>& barriers,
	Util::Dictionary<CoreGraphics::ShaderRWTextureId, Util::Array<TextureDependency>>& rwTextures,
	Util::Dictionary<CoreGraphics::ShaderRWBufferId, Util::Array<BufferDependency>>& rwBuffers,
	Util::Dictionary<CoreGraphics::RenderTextureId, Util::Array<TextureDependency>>& renderTextures)
{
	CompiledImpl* myCompiled = (CompiledImpl*)this->AllocCompiled(allocator);

	this->compiled = myCompiled;
	this->SetupSynchronization(allocator, events, barriers, rwTextures, rwBuffers, renderTextures);
	compiledOps.Append(myCompiled);
}

} // namespace Frame2