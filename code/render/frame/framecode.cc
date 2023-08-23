//------------------------------------------------------------------------------
//  @file framecode.cc
//  @copyright (C) 2021 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
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
FrameCode::Build(const BuildContext& ctx)
{
    n_assert(this->func != nullptr);

    // if not enable, abort early
    if (!this->enabled)
        return;

    CompiledImpl* myCompiled = (CompiledImpl*)this->AllocCompiled(ctx.allocator);
    this->compiled = myCompiled;

    this->SetupSynchronization(ctx.allocator, ctx.events, ctx.barriers, ctx.buffers, ctx.textures);
    ctx.compiledOps.Append(myCompiled);
}

} // namespace Frame
