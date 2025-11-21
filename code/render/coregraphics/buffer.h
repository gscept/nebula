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

enum class BufferUsage
{
    InvalidBufferUsage              = 0x0,
    TransferSource                  = 0x1,
    TransferDestination             = 0x2,
    ConstantBuffer                  = 0x4,
    ConstantTexelBuffer             = 0x8,
    ReadWrite                       = 0x10,
    ReadWriteTexelBuffer            = 0x20,
    Vertex                          = 0x40,
    Index                           = 0x80,
    IndirectDraw                    = 0x100,
    ShaderAddress                   = 0x200,
    AccelerationStructureData       = 0x400,    // The buffer holding acceleration structures
    AccelerationStructureScratch    = 0x800,    // Scratch buffer for building and updating acceleration structures
    AccelerationStructureInput      = 0x1000,   // Input to acceleration structure, vertices and indices
    AccelerationStructureInstances  = 0x2000,   // Acceleration structure instances buffer
    ShaderTable                     = 0x4000
};
__ImplementEnumBitOperators(CoreGraphics::BufferUsage);

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
        , usageFlags(CoreGraphics::BufferUsage::InvalidBufferUsage)
        , queueSupport(AutomaticQueueSupport)
        , sparse(false)
        , data(nullptr)
        , dataSize(0)
    {}

    Util::StringAtom name;
    SizeT size;                 // this should be the number of items, for vertex buffers, this is vertex count
    SizeT elementSize;          // this should be the size of each item, for vertex buffers, this is the vertex byte size as received from the vertex layout
    SizeT byteSize;             // if not zero, this is a precomputed version of size * elementSize, and will be used instead
    BufferAccessMode mode;
    BufferUsage usageFlags;
    BufferQueueSupportFlags queueSupport;
    bool sparse;
    const void* data;
    uint dataSize;
};

struct BufferSparsePage
{
    SizeT offset;
    CoreGraphics::Alloc alloc;
};


/// create buffer
const BufferId CreateBuffer(const BufferCreateInfo& info);
/// destroy buffer
void DestroyBuffer(const BufferId id);

/// get type of buffer
const BufferUsage BufferGetType(const BufferId id);
/// get buffer size, which is the number of elements
const uint64_t BufferGetSize(const BufferId id);
/// get buffer element size, this is the size of a single element, like a vertex or index, and is multiplied with the size
const uint64_t BufferGetElementSize(const BufferId id);
/// get buffer total byte size
const uint64_t BufferGetByteSize(const BufferId id);
/// get maximum size of upload for BufferUpload
const uint64_t BufferGetUploadMaxSize();

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
/// Update buffer data as array
template<class TYPE> void BufferUpdateArray(const BufferId id, const Util::Array<TYPE>& data, const uint offset = 0);
/// Update buffer data as array
template<class TYPE> void BufferUpdateArray(const BufferId id, const Util::FixedArray<TYPE>& data, const uint offset = 0);
/// upload data from pointer directly to buffer through submission context
template<class TYPE> void BufferUpload(const CoreGraphics::CmdBufferId cmdBuf, const BufferId id, const TYPE* data, const uint count = 1, const uint offset = 0);

/// fill buffer with data much like memset
void BufferFill(const CoreGraphics::CmdBufferId cmdBuf, const BufferId id, char pattern);

/// flush any changes done to the buffer on the CPU side so they are visible on the GPU
void BufferFlush(const BufferId id, uint64_t offset = 0ull, uint64_t size = NEBULA_WHOLE_BUFFER_SIZE);
/// invalidate buffer CPU side, such that any GPU changes will be made visible
void BufferInvalidate(const BufferId id, uint64_t offset = 0ull, uint64_t size = NEBULA_WHOLE_BUFFER_SIZE);

/// Evict a page
void BufferSparseEvict(const BufferId id, IndexT pageIndex);
/// Make a page resident
void BufferSparseMakeResident(const BufferId id, IndexT pageIndex);
/// Get the page index given an offset
IndexT BufferSparseGetPageIndex(const BufferId id, SizeT offset);
/// Get the buffer page size
SizeT BufferSparseGetPageSize(const BufferId id);
/// Commit sparse bindings
void BufferSparseCommitChanges(const BufferId id);

/// Get buffer device address
CoreGraphics::DeviceAddress BufferGetDeviceAddress(const BufferId id);

/// Shortcut for creating a staging buffer and copy
void BufferCopyWithStaging(const CoreGraphics::BufferId dest, const uint offset, const void* data, const uint size);

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
void BufferUpdateArray(const BufferId id, const Util::Array<TYPE>& data, const uint offset)
{
    BufferUpdate(id, (const void*)data.Begin(), data.ByteSize(), offset);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void BufferUpdateArray(const BufferId id, const Util::FixedArray<TYPE>& data, const uint offset)
{
    BufferUpdate(id, (const void*)data.Begin(), data.ByteSize(), offset);
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

//------------------------------------------------------------------------------
/**
    Set of buffers which creates a buffer per each buffered frame
*/
struct BufferSet
{
    /// Default constructor
    BufferSet() {};

    /// Setup set
    void Create(const BufferCreateInfo& createInfo);
    /// Destroy
    void Destroy();

    /// Get current buffer
    const CoreGraphics::BufferId Buffer();
    
    Util::FixedArray<CoreGraphics::BufferId> buffers;
};

//------------------------------------------------------------------------------
/**
*/
struct BufferWithStaging
{
    /// Default constructor
    BufferWithStaging() {};

    /// Setup set
    void Create(const BufferCreateInfo& createInfo);
    /// Destroy
    void Destroy();

    /// Get device buffer
    const CoreGraphics::BufferId DeviceBuffer();
    /// Get staging buffer for this frame
    const CoreGraphics::BufferId HostBuffer();

    /// Flush changes on staging buffer to device
    void Flush(const CoreGraphics::CmdBufferId cmdBuf, SizeT numBytes);
    
    CoreGraphics::BufferId deviceBuffer;
    BufferSet hostBuffers;
};



} // namespace CoreGraphics
