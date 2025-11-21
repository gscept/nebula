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
void
BufferSet::Create(const BufferCreateInfo& createInfo)
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
void
BufferSet::Destroy()
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
const CoreGraphics::BufferId
BufferSet::Buffer()
{
    return this->buffers[CoreGraphics::GetBufferedFrameIndex()];
}

//------------------------------------------------------------------------------
/**
*/
void
BufferWithStaging::Create(const BufferCreateInfo& createInfo)
{
    BufferCreateInfo bufferInfo = createInfo;
    bufferInfo.name = Util::String::Sprintf("%s Host Buffer", createInfo.name.Value());
    bufferInfo.mode = CoreGraphics::HostLocal;
    bufferInfo.usageFlags |= CoreGraphics::BufferUsage::TransferSource;
    this->hostBuffers.Create(bufferInfo);

    bufferInfo.name = Util::String::Sprintf("%s Device Buffer", createInfo.name.Value());
    bufferInfo.mode = CoreGraphics::DeviceLocal;
    bufferInfo.usageFlags |= CoreGraphics::BufferUsage::TransferDestination;
    this->deviceBuffer = CoreGraphics::CreateBuffer(bufferInfo);
}

//------------------------------------------------------------------------------
/**
*/
void
BufferWithStaging::Destroy()
{
    this->hostBuffers.Destroy();
    if (this->deviceBuffer != InvalidBufferId)
        DestroyBuffer(this->deviceBuffer);
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
    n_assert(numBytes > 0);
    CoreGraphics::BufferCopy copy;
    copy.offset = 0;
    CoreGraphics::CmdCopy(cmdBuf, this->hostBuffers.Buffer(), {copy}, this->deviceBuffer, {copy}, numBytes);
}

} // namespace CoreGraphics
