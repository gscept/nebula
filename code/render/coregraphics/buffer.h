#pragma once
//------------------------------------------------------------------------------
/**
    GPU side buffer

    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/id.h"
#include "ids/idpool.h"
#include "util/stringatom.h"
#include "coregraphics/indextype.h"
#include "coregraphics/vertexlayout.h"
#include "coregraphics/config.h"
#include "gpubuffertypes.h"
namespace CoreGraphics
{

struct SubmissionContextId;
ID_24_8_TYPE(BufferId);

enum BufferUsageFlag
{
    InvalidBufferType           = 0x0,
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

struct BufferCreateInfo
{
    BufferCreateInfo()
        : name(""_atm)
        , size(1)
        , elementSize(1)
        , byteSize(0)
        , mode(DeviceLocal)
        , usageFlags(InvalidBufferType)
        , data(nullptr)
        , dataSize(0)
    {}

    Util::StringAtom name;
    SizeT size;                 // this should be the number of items, for vertex buffers, this is vertex count
    SizeT elementSize;          // this should be the size of each item, for vertex buffers, this is the vertex byte size as received from the vertex layout
    SizeT byteSize;             // if set, uses this for size, otherwise calculates the size from size and elementSize
    BufferAccessMode mode;
    BufferUsageFlags usageFlags;
    void* data;
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

/// map memory
void* BufferMap(const BufferId id);
/// unmap memory
void BufferUnmap(const BufferId id);

/// update buffer data
void BufferUpdate(const BufferId id, const void* data, const uint size, const uint offset = 0);
/// update buffer data as array
void BufferUpdateArray(const BufferId id, const void* data, const uint size, const uint count, const uint offset = 0);
/// update buffer through submission instead of mapped buffer (slower, but allows for updates to DeviceLocal buffers)
void BufferUpload(const BufferId id, const void* data, const uint size, const uint count, const uint offset, const CoreGraphics::SubmissionContextId sub);
/// update buffer data
template<class TYPE> void BufferUpdate(const BufferId id, const TYPE& data, const uint offset = 0);
/// update buffer data as array
template<class TYPE> void BufferUpdateArray(const BufferId id, const TYPE* data, const uint count, const uint offset = 0);
/// upload data from pointer directly to buffer through submission context
template<class TYPE> void BufferUpload(const BufferId id, const TYPE* data, const uint count, const uint offset, const CoreGraphics::SubmissionContextId sub);

/// fill buffer with data much like memset
void BufferFill(const BufferId id, char pattern, const CoreGraphics::SubmissionContextId sub);

/// flush any changes done to the buffer CPU side so they are visible on the GPU
void BufferFlush(const BufferId id, IndexT offset = 0, SizeT size = NEBULA_WHOLE_BUFFER_SIZE);
/// invalidate buffer CPU side, such that any GPU changes will be made visible
void BufferInvalidate(const BufferId id, IndexT offset = 0, SizeT size = NEBULA_WHOLE_BUFFER_SIZE);

//------------------------------------------------------------------------------
/**
*/
template<>
inline void BufferUpdate(const BufferId id, const Util::Variant& data, const uint offset)
{
    BufferUpdate(id, data.AsVoidPtr(), data.Size(), offset);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void BufferUpdate(const BufferId id, const TYPE& data, const uint offset)
{
    BufferUpdate(id, &data, sizeof(TYPE), offset);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void BufferUpdateArray(const BufferId id, const TYPE* data, const uint count, const uint offset)
{
    BufferUpdateArray(id, data, sizeof(TYPE), count, offset);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void BufferUpload(const BufferId id, const TYPE* data, const uint count, const uint offset, const CoreGraphics::SubmissionContextId sub)
{
    BufferUpload(id, data, sizeof(TYPE), count, offset, sub);
}

} // namespace CoreGraphics
