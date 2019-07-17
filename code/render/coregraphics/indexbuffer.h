#pragma once
//------------------------------------------------------------------------------
/**
	Index buffer related functions

	The actual allocation is handled by the MemoryIndexBufferPool

	(C)2017-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "resources/resourceid.h"
#include "coregraphics/indextype.h"
#include "gpubuffertypes.h"
namespace CoreGraphics
{

ID_24_8_TYPE(IndexBufferId);
//RESOURCE_ID_TYPE(IndexBufferId);

struct IndexBufferCreateInfo
{
	Resources::ResourceName name;
	Util::StringAtom tag;
	CoreGraphics::GpuBufferTypes::Access access;
	CoreGraphics::GpuBufferTypes::Usage usage;
	CoreGraphics::GpuBufferTypes::Syncing sync;
	CoreGraphics::IndexType::Code type;
	SizeT numIndices;
	void* data;
	PtrDiff dataSize;
};

struct IndexBufferCreateDirectInfo
{
	Resources::ResourceName name;
	Util::StringAtom tag;
	CoreGraphics::GpuBufferTypes::Access access;
	CoreGraphics::GpuBufferTypes::Usage usage;
	CoreGraphics::GpuBufferTypes::Syncing sync;
	CoreGraphics::IndexType::Code type;
	SizeT size;
};

/// create new index buffer with intended usage, access and CPU syncing parameters, together with size of buffer
const IndexBufferId CreateIndexBuffer(const IndexBufferCreateInfo& info);
/// create new index buffer directly
const IndexBufferId CreateIndexBuffer(const IndexBufferCreateDirectInfo& info);
/// destroy Index buffer
void DestroyIndexBuffer(const IndexBufferId id);

/// update Index buffer
void IndexBufferUpdate(const IndexBufferId id, void* data, PtrDiff size, PtrDiff offset);
/// request lock for Index buffer, such that it can be updated
void IndexBufferLock(const IndexBufferId id, const PtrDiff offset, const PtrDiff range);
/// request unlock for Index buffer
void IndexBufferUnlock(const IndexBufferId id, const PtrDiff offset, const PtrDiff range);
/// map GPU memory
void* IndexBufferMap(const IndexBufferId id, const CoreGraphics::GpuBufferTypes::MapType type);
/// unmap GPU memory
void IndexBufferUnmap(const IndexBufferId id);

/// get index type of index buffer
const CoreGraphics::IndexType::Code IndexBufferGetType(const IndexBufferId id);
/// get number of indices
const SizeT IndexBufferGetNumIndices(const IndexBufferId id);

} // CoreGraphics
