#pragma once
//------------------------------------------------------------------------------
/**
    GPU side buffer

    @copyright
    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "config.h"
#include "ids/id.h"
#include "ids/idpool.h"
#include "util/stringatom.h"
#include "coregraphics/config.h"
#include "coregraphics/commandbuffer.h"
#include "gpubuffertypes.h"
#include "ids/idallocator.h"

namespace CoreGraphics
{

struct CmdBufferId;
ID_24_8_TYPE(BufferId);
_DECL_ACQUIRE_RELEASE(BufferId);

enum BufferAccessMode
{
    DeviceLocal,		// Buffer can only be used by the GPU
    HostLocal,			// Buffer memory is coherent on the CPU
    HostCached,		    // Buffer memory is cached on the CPU and requires flush to update CPU->GPU or invalidate to update GPU->CPU
    DeviceAndHost		// Buffer memory lives on both CPU and GPU and requires a flush/invalidate like HostCached
};

enum BufferUsageFlag
{
    InvalidBufferUsage          = 0x0,
    TransferBufferSource        = 0x1,
    TransferBufferDestination   = 0x2,
    ConstantBuffer              = 0x4,
    ConstantTexelBuffer         = 0x8,
    ReadWriteBuffer             = 0x10,
    ReadWriteTexelBuffer        = 0x20,
    VertexBuffer                = 0x40,
    IndexBuffer                 = 0x80,
    IndirectBuffer              = 0x100
};
typedef uint BufferUsageFlags;

enum BufferQueueSupport
{
    AutomaticQueueSupport       = 0x0,
    GraphicsQueueSupport        = 0x1,
    ComputeQueueSupport         = 0x2,
    TransferQueueSupport        = 0x4
};
typedef uint BufferQueueSupportFlags;

struct BufferCreateInfo
{
    BufferCreateInfo()
        : name(""_atm)
        , size(1)
        , elementSize(1)
        , byteSize(0)
        , mode(DeviceLocal)
        , usageFlags(InvalidBufferUsage)
        , queueSupport(AutomaticQueueSupport)
        , data(nullptr)
        , dataSize(0)
    {}

    Util::StringAtom name;
    SizeT size;                 // this should be the number of items, for vertex buffers, this is vertex count
    SizeT elementSize;          // this should be the size of each item, for vertex buffers, this is the vertex byte size as received from the vertex layout
    SizeT byteSize;             // if set, uses this for size, otherwise calculates the size from size and elementSize
    BufferAccessMode mode;
    BufferUsageFlags usageFlags;
    BufferQueueSupportFlags queueSupport;
    const void* data;
    uint dataSize;
};


/// create buffer
const BufferId CreateBuffer(const BufferCreateInfo& info);
/// destroy buffer
void DestroyBuffer(const BufferId id);

/// get type of buffer
const BufferUsageFlags BufferGetType(const BufferId id);
/// get buffer size, which is the number of elements
const SizeT BufferGetSize(const BufferId id);
/// get buffer element size, this is the size of a single element, like a vertex or index, and is multiplied with the size
const SizeT BufferGetElementSize(const BufferId id);
/// get buffer total byte size
const SizeT BufferGetByteSize(const BufferId id);
/// get maximum size of upload for BufferUpload
const SizeT BufferGetUploadMaxSize();

/// map memory
void* BufferMap(const BufferId id);
/// map memory and cast to type
template <class T> T* BufferMap(const BufferId id);
/// unmap memory
void BufferUnmap(const BufferId id);

/// update buffer data
void BufferUpdate(const BufferId id, const void* data, const uint size, const uint offset = 0);

/// update buffer directly on command buffer during frame update, asserts if size is too big
void BufferUpload(const CoreGraphics::CmdBufferId cmdBuf, const BufferId id, const void* data, const uint size, const uint offset);
/// update buffer data
template<class TYPE> void BufferUpdate(const BufferId id, const TYPE& data, const uint offset = 0);
/// update buffer data as array
template<class TYPE> void BufferUpdateArray(const BufferId id, const TYPE* data, const uint count, const uint offset = 0);
/// upload data from pointer directly to buffer through submission context
template<class TYPE> void BufferUpload(const CoreGraphics::CmdBufferId cmdBuf, const BufferId id, const TYPE* data, const uint count = 1, const uint offset = 0);

/// fill buffer with data much like memset
void BufferFill(const CoreGraphics::CmdBufferId cmdBuf, const BufferId id, char pattern);

/// flush any changes done to the buffer CPU side so they are visible on the GPU
void BufferFlush(const BufferId id, IndexT offset = 0, SizeT size = NEBULA_WHOLE_BUFFER_SIZE);
/// invalidate buffer CPU side, such that any GPU changes will be made visible
void BufferInvalidate(const BufferId id, IndexT offset = 0, SizeT size = NEBULA_WHOLE_BUFFER_SIZE);

//------------------------------------------------------------------------------
/**
*/
template <class T>
inline T*
BufferMap(const BufferId id)
{
    return reinterpret_cast<T*>(BufferMap(id));
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
inline void
BufferUpdate(const BufferId id, const TYPE& data, const uint offset)
{
    BufferUpdate(id, (const void*)&data, sizeof(TYPE), offset);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
inline void
BufferUpdateArray(const BufferId id, const TYPE* data, const uint count, const uint offset)
{
    BufferUpdate(id, (const void*)data, sizeof(TYPE) * count, offset);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
inline void
BufferUpload(const CoreGraphics::CmdBufferId cmdBuf, const BufferId id, const TYPE* data, const uint count, const uint offset)
{
    BufferUpload(cmdBuf, id, (const void*)data, sizeof(TYPE) * count, offset);
}

} // namespace CoreGraphics
