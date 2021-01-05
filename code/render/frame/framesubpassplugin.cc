//------------------------------------------------------------------------------
// framesubpassplugin.cc
// (C) 2016-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "framesubpassplugin.h"

using namespace CoreGraphics;
namespace Frame
{

//------------------------------------------------------------------------------
/**
*/
FrameSubpassPlugin::FrameSubpassPlugin()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
FrameSubpassPlugin::~FrameSubpassPlugin()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
FrameSubpassPlugin::Setup()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
FrameSubpassPlugin::Discard()
{
    FrameOp::Discard();

    this->func = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
FrameOp::Compiled*
FrameSubpassPlugin::AllocCompiled(Memory::ArenaAllocator<BIG_CHUNK>& allocator)
{
    CompiledImpl* ret = allocator.Alloc<CompiledImpl>();
    ret->func = this->func;
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
FrameSubpassPlugin::CompiledImpl::Run(const IndexT frameIndex, const IndexT bufferIndex)
{
    this->func(frameIndex, bufferIndex);
}

} // namespace Frame2
