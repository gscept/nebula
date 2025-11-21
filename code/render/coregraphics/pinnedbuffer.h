#pragma once
//------------------------------------------------------------------------------
/**
    A pinned buffer is a type of GPU buffer which handles virtual paging automatically

    @copyright
    (C) 2024 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "buffer.h"
namespace CoreGraphics  
{

template <typename STORAGE>
struct PinnedBuffer
{
    /// Default constructor
    PinnedBuffer() {};
    /// Constructor
    PinnedBuffer(const BufferCreateInfo& createInfo);
    /// Move constructor
    PinnedBuffer(PinnedBuffer&& rhs);
    /// Move assignment
    void operator=(PinnedBuffer&& rhs);

    /// Append a copy of an element to a region
    void Write(const STORAGE& item, IndexT offset);
    /// Write a series of items
    void Write(const STORAGE* items, SizeT num, IndexT offset);

    /// Copy from storage unto device buffer
    void Flush(const CoreGraphics::CmdBufferId buf);

    uint iterator;
    Util::Array<CoreGraphics::BufferCopy> fromCopies, toCopies;
    Util::Array<uint> sizes;
    BufferSet hostBuffers;
    CoreGraphics::BufferId deviceBuffer;
};

//------------------------------------------------------------------------------
/**
*/
template<typename STORAGE> inline 
PinnedBuffer<STORAGE>::PinnedBuffer(const BufferCreateInfo& createInfo)
    : iterator(0)
{
    n_assert(createInfo.sparse == true);
    BufferCreateInfo hostBufferInfo = createInfo;
    hostBufferInfo.mode = CoreGraphics::HostLocal;
    hostBufferInfo.usageFlags |= CoreGraphics::TransferSource;

    this->hostBuffers.Create(hostBufferInfo);
    BufferCreateInfo deviceBufferInfo = createInfo;
    deviceBufferInfo.mode = CoreGraphics::DeviceLocal;
    deviceBufferInfo.usageFlags |= CoreGraphics::TransferDestination;
    this->deviceBuffer = CoreGraphics::CreateBuffer(deviceBufferInfo);
}

//------------------------------------------------------------------------------
/**
*/
template<typename STORAGE> inline 
PinnedBuffer<STORAGE>::PinnedBuffer(PinnedBuffer&& rhs)
{
    this->hostBuffers = std::move(rhs.hostBuffers);
    if (this->deviceBuffer != InvalidBufferId)
        DestroyBuffer(this->deviceBuffer);
    this->deviceBuffer = std::move(rhs.deviceBuffer);
    this->iterator = rhs.iterator;
    rhs.iterator = 0;
}

//------------------------------------------------------------------------------
/**
*/
template<typename STORAGE> inline void 
PinnedBuffer<STORAGE>::operator=(PinnedBuffer&& rhs)
{
    this->hostBuffers = std::move(rhs.hostBuffers);
    if (this->deviceBuffer != InvalidBufferId)
        DestroyBuffer(this->deviceBuffer);
    this->deviceBuffer = std::move(rhs.deviceBuffer);
    this->iterator = rhs.iterator;
    rhs.iterator = 0;
}

//------------------------------------------------------------------------------
/**
*/
template<typename STORAGE> inline void 
PinnedBuffer<STORAGE>::Write(const STORAGE& item, IndexT offset)
{
    this->fromCopies.Append(BufferCopy{ .offset = this->iterator });
    this->toCopies.Append(BufferCopy{ .offset = offset });
    this->sizes.Append(sizeof(STORAGE));
    CoreGraphics::BufferUpdate(this->hostBuffers.Buffer(), item, this->iterator);
    this->iterator += sizeof(STORAGE);
}

//------------------------------------------------------------------------------
/**
*/
template<typename STORAGE> inline void 
PinnedBuffer<STORAGE>::Write(const STORAGE* items, SizeT num, IndexT offset)
{
    this->fromCopies.Append(BufferCopy{ .offset = this->iterator });
    this->toCopies.Append(BufferCopy{ .offset = offset });
    uint byteSize = sizeof(STORAGE) * num;
    this->sizes.Append(byteSize);
    CoreGraphics::BufferUpdate(this->hostBuffers.Buffer(), items, byteSize, this->iterator);
    this->iterator += byteSize;
}

//------------------------------------------------------------------------------
/**
*/
template<typename STORAGE> inline void 
PinnedBuffer<STORAGE>::Flush(const CoreGraphics::CmdBufferId buf)
{
    N_CMD_SCOPE(buf, NEBULA_MARKER_TRANSFER, "PinnedBuffer Update");
    for (auto size : this->sizes)
    {
        CoreGraphics::CmdCopy(buf, this->hostBuffers.Buffer(), this->fromCopies, this->deviceBuffer, this->toCopies, size);
    }
    this->fromCopies.Clear();
    this->toCopies.Clear();
    this->sizes.Clear();
    this->iterator = 0;
}

} // namespace CoreGraphics
