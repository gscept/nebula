//------------------------------------------------------------------------------
// framecompute.cc
// (C) 2016-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "framecompute.h"
#include "coregraphics/graphicsdevice.h"

using namespace CoreGraphics;
namespace Frame
{

//------------------------------------------------------------------------------
/**
*/
FrameCompute::FrameCompute()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
FrameCompute::~FrameCompute()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void 
FrameCompute::Discard()
{
    this->program = InvalidShaderProgramId;

    DestroyResourceTable(this->resourceTable);
    IndexT i;
    for (i = 0; i < this->constantBuffers.Size(); i++)
        DestroyBuffer(this->constantBuffers.ValueAtIndex(i));
}

//------------------------------------------------------------------------------
/**
*/
FrameOp::Compiled*
FrameCompute::AllocCompiled(Memory::ArenaAllocator<BIG_CHUNK>& allocator)
{
    CompiledImpl* ret = allocator.Alloc<CompiledImpl>();
    ret->program = this->program;
    ret->resourceTable = this->resourceTable;
    ret->x = this->x;
    ret->y = this->y;
    ret->z = this->z;
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
FrameCompute::CompiledImpl::Run(const CoreGraphics::CmdBufferId cmdBuf, const IndexT frameIndex, const IndexT bufferIndex)
{
    n_assert(this->program != InvalidShaderProgramId);

    CoreGraphics::CmdSetShaderProgram(cmdBuf, this->program);

    // compute
    CoreGraphics::CmdSetResourceTable(cmdBuf, this->resourceTable, NEBULA_BATCH_GROUP, CoreGraphics::ComputePipeline, nullptr);
    CoreGraphics::CmdDispatch(cmdBuf, this->x, this->y, this->z);
}

} // namespace Frame2
