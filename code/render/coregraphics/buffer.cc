//------------------------------------------------------------------------------
//  @file buffer.cc
//  @copyright (C) 2022 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "buffer.h"
namespace CoreGraphics
{

//------------------------------------------------------------------------------
/**
*/
BufferSet::BufferSet(const BufferCreateInfo& createInfo)
{
    const SizeT numBuffers = CoreGraphics::GetNumBufferedFrames();
    this->buffers.Resize(numBuffers);
    for (IndexT i = 0; i < numBuffers; i++)
    {
        this->buffers[i] = CreateBuffer(createInfo);
    }
}

//------------------------------------------------------------------------------
/**
*/
BufferSet::BufferSet(BufferSet&& rhs)
{
    this->buffers = rhs.buffers;
    rhs.buffers.Clear();
}

//------------------------------------------------------------------------------
/**
*/
void
BufferSet::operator=(BufferSet&& rhs)
{
    this->buffers = rhs.buffers;
    rhs.buffers.Clear();
}

//------------------------------------------------------------------------------
/**
*/
BufferSet::~BufferSet()
{
    for (IndexT i = 0; i < this->buffers.Size(); i++)
    {
        DestroyBuffer(this->buffers[i]);
    }
    this->buffers.Clear();
}

} // namespace CoreGraphics
