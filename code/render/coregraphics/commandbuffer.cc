//------------------------------------------------------------------------------
//  @file commandbuffer.cc
//  @copyright (C) 2022 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "commandbuffer.h"
#include "barrier.h"
namespace CoreGraphics
{

//------------------------------------------------------------------------------
/**
*/
void
CmdBarrier(const CmdBufferId id
                         , CoreGraphics::PipelineStage fromStage
                         , CoreGraphics::PipelineStage toStage
                         , CoreGraphics::BarrierDomain domain
                         , const Util::FixedArray<TextureBarrierInfo>& textures
                         , const IndexT fromQueue
                         , const IndexT toQueue
                         , const char* name)
{
    CmdBarrier(id, fromStage, toStage, domain, textures, nullptr, nullptr, fromQueue, toQueue, name);
}

//------------------------------------------------------------------------------
/**
*/
void
CmdBarrier(const CmdBufferId id
                         , CoreGraphics::PipelineStage fromStage
                         , CoreGraphics::PipelineStage toStage
                         , CoreGraphics::BarrierDomain domain
                         , const Util::FixedArray<BufferBarrierInfo>& buffers
                         , const IndexT fromQueue
                         , const IndexT toQueue
                         , const char* name)
{
    CmdBarrier(id, fromStage, toStage, domain, nullptr, buffers, nullptr, fromQueue, toQueue, name);
}

//------------------------------------------------------------------------------
/**
*/
void
CmdBarrier(const CmdBufferId id
                         , CoreGraphics::PipelineStage fromStage
                         , CoreGraphics::PipelineStage toStage
                         , CoreGraphics::BarrierDomain domain
                         , const Util::FixedArray<AccelerationStructureBarrierInfo>& accelerationStructures
                         , const IndexT fromQueue
                         , const IndexT toQueue
                         , const char* name)
{
    CmdBarrier(id, fromStage, toStage, domain, nullptr, nullptr, accelerationStructures, fromQueue, toQueue, name);
}

//------------------------------------------------------------------------------
/**
*/
void
CmdBarrier(const CmdBufferId id
                         , CoreGraphics::PipelineStage fromStage
                         , CoreGraphics::PipelineStage toStage
                         , CoreGraphics::BarrierDomain domain
                         , const IndexT fromQueue
                         , const IndexT toQueue
                         , const char* name)
{
    CmdBarrier(id, fromStage, toStage, domain, nullptr, nullptr, nullptr, fromQueue, toQueue, name);
}

} // namespace CoreGraphics
