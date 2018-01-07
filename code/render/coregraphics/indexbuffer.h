#pragma once
//------------------------------------------------------------------------------
/**
	Index buffer related functions

	The actual allocation is handled by the MemoryIndexBufferPool

	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "resources/resourceid.h"
#include "coregraphics/indextype.h"
#include "gpubuffertypes.h"
namespace CoreGraphics
{

ID_32_24_8_TYPE(IndexBufferId);

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

/// create new Index buffer with intended usage, access and CPU syncing parameters, together with size of buffer
const IndexBufferId CreateIndexBuffer(IndexBufferCreateInfo info);
/// destroy Index buffer
void DestroyIndexBuffer(const IndexBufferId id);
/// bind Index buffer resource individually
void IndexBufferBind(const IndexBufferId id, const IndexT offset);
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


class MemoryIndexBufferPool;
extern MemoryIndexBufferPool* iboPool;

} // CoreGraphics
