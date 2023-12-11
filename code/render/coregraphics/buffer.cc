//------------------------------------------------------------------------------
//  @file buffer.cc
//  @copyright (C) 2022 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "coregraphics/graphicsdevice.h"
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
        this->buffers[i] = CoreGraphics::CreateBuffer(createInfo);
    }
}

//------------------------------------------------------------------------------
/**
*/
BufferSet::BufferSet(BufferSet&& rhs)
{
    for (IndexT i = 0; i < this->buffers.Size(); i++)
    {
        CoreGraphics::DestroyBuffer(this->buffers[i]);
    }
    this->buffers = rhs.buffers;
    rhs.buffers.Clear();
}

//------------------------------------------------------------------------------
/**
*/
void
BufferSet::operator=(BufferSet&& rhs)
{
    for (IndexT i = 0; i < this->buffers.Size(); i++)
    {
        CoreGraphics::DestroyBuffer(this->buffers[i]);
    }
    this->buffers = rhs.buffers;
    rhs.buffers.Clear();
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::BufferId
BufferSet::Buffer()
{
    return this->buffers[CoreGraphics::GetBufferedFrameIndex()];
}

//------------------------------------------------------------------------------
/**
*/
BufferSet::~BufferSet()
{
    for (IndexT i = 0; i < this->buffers.Size(); i++)
    {
        CoreGraphics::DestroyBuffer(this->buffers[i]);
    }
    this->buffers.Clear();
}

//------------------------------------------------------------------------------
/**
*/
BufferWithStaging::BufferWithStaging(const BufferCreateInfo& createInfo)
{
    BufferCreateInfo bufferInfo = createInfo;
    bufferInfo.mode = CoreGraphics::HostLocal;
    bufferInfo.usageFlags |= CoreGraphics::TransferBufferSource;
    this->hostBuffers = BufferSet(bufferInfo);

    bufferInfo.mode = CoreGraphics::DeviceLocal;
    bufferInfo.usageFlags |= CoreGraphics::TransferBufferDestination;
    this->deviceBuffer = CoreGraphics::CreateBuffer(bufferInfo);
}

//------------------------------------------------------------------------------
/**
*/
BufferWithStaging::BufferWithStaging(BufferWithStaging&& rhs)
{
    this->hostBuffers = std::move(rhs.hostBuffers);
    if (this->deviceBuffer != InvalidBufferId)
        DestroyBuffer(this->deviceBuffer);
    this->deviceBuffer = std::move(rhs.deviceBuffer);
}

//------------------------------------------------------------------------------
/**
*/
void
BufferWithStaging::operator=(BufferWithStaging&& rhs)
{
    this->hostBuffers = std::move(rhs.hostBuffers);
    if (this->deviceBuffer != InvalidBufferId)
        DestroyBuffer(this->deviceBuffer);
    this->deviceBuffer = std::move(rhs.deviceBuffer);
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::BufferId
BufferWithStaging::DeviceBuffer()
{
    return this->deviceBuffer;
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::BufferId
BufferWithStaging::HostBuffer()
{
    return this->hostBuffers.Buffer();
}

//------------------------------------------------------------------------------
/**
*/
void
BufferWithStaging::Flush(const CoreGraphics::CmdBufferId cmdBuf, SizeT numBytes)
{
    IndexT bufferIndex = CoreGraphics::GetBufferedFrameIndex();
    CoreGraphics::BufferCopy copy;
    copy.offset = 0;
    CoreGraphics::CmdCopy(cmdBuf, this->hostBuffers.Buffer(), {copy}, this->deviceBuffer, {copy}, numBytes);
}

//------------------------------------------------------------------------------
/**
*/
BufferWithStaging::~BufferWithStaging()
{
}

} // namespace CoreGraphics
