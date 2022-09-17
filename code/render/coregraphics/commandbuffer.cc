//------------------------------------------------------------------------------
//  @file commandbuffer.cc
//  @copyright (C) 2022 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "commandbuffer.h"
#include "barrier.h"
namespace CoreGraphics
{

//------------------------------------------------------------------------------
/**
*/
void
CoreGraphics::CmdBarrier(const CmdBufferId id
                         , CoreGraphics::PipelineStage fromStage
                         , CoreGraphics::PipelineStage toStage
                         , CoreGraphics::BarrierDomain domain
                         , const Util::FixedArray<TextureBarrierInfo>& textures
                         , const IndexT fromQueue
                         , const IndexT toQueue
                         , const char* name)
{
    CmdBarrier(id, fromStage, toStage, domain, textures, nullptr, fromQueue, toQueue, name);
}

//------------------------------------------------------------------------------
/**
*/
void
CoreGraphics::CmdBarrier(const CmdBufferId id
                         , CoreGraphics::PipelineStage fromStage
                         , CoreGraphics::PipelineStage toStage
                         , CoreGraphics::BarrierDomain domain
                         , const Util::FixedArray<BufferBarrierInfo>& buffers
                         , const IndexT fromQueue
                         , const IndexT toQueue
                         , const char* name)
{
    CmdBarrier(id, fromStage, toStage, domain, nullptr, buffers, fromQueue, toQueue, name);
}

//------------------------------------------------------------------------------
/**
*/
void
CoreGraphics::CmdBarrier(const CmdBufferId id
                         , CoreGraphics::PipelineStage fromStage
                         , CoreGraphics::PipelineStage toStage
                         , CoreGraphics::BarrierDomain domain
                         , const IndexT fromQueue
                         , const IndexT toQueue
                         , const char* name)
{
    CmdBarrier(id, fromStage, toStage, domain, nullptr, nullptr, fromQueue, toQueue, name);
}

} // namespace CoreGraphics
