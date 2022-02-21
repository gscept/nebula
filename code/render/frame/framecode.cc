//------------------------------------------------------------------------------
//  @file framecode.cc
//  @copyright (C) 2021 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "framecode.h"
namespace Frame
{

//------------------------------------------------------------------------------
/**
*/
FrameCode::FrameCode()
{
}

//------------------------------------------------------------------------------
/**
*/
FrameCode::~FrameCode()
{
}

//------------------------------------------------------------------------------
/**
*/
void
FrameCode::CompiledImpl::Run(const CoreGraphics::CmdBufferId cmdBuf, const IndexT frameIndex, const IndexT bufferIndex)
{
    this->func(cmdBuf, frameIndex, bufferIndex);
}

//------------------------------------------------------------------------------
/**
*/
FrameOp::Compiled*
FrameCode::AllocCompiled(Memory::ArenaAllocator<BIG_CHUNK>& allocator)
{
    CompiledImpl* ret = allocator.Alloc<CompiledImpl>();

#if NEBULA_GRAPHICS_DEBUG
    ret->name = this->name;
#endif
    ret->func = this->func;
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
FrameCode::Build(
    Memory::ArenaAllocator<BIG_CHUNK>& allocator
    , Util::Array<FrameOp::Compiled*>& compiledOps
    , Util::Array<CoreGraphics::EventId>& events
    , Util::Array<CoreGraphics::BarrierId>& barriers
    , Util::Dictionary<CoreGraphics::BufferId, Util::Array<BufferDependency>>& rwBuffers
    , Util::Dictionary<CoreGraphics::TextureId, Util::Array<TextureDependency>>& textures
)
{
    n_assert(this->func != nullptr);

    // if not enable, abort early
    if (!this->enabled)
        return;

    CompiledImpl* myCompiled = (CompiledImpl*)this->AllocCompiled(allocator);
    this->compiled = myCompiled;

    this->SetupSynchronization(allocator, events, barriers, rwBuffers, textures);
    compiledOps.Append(myCompiled);
}

} // namespace Frame
