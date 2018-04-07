//------------------------------------------------------------------------------
//  indexbuffer.cc
//  (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "config.h"
#include "indexbuffer.h"
#include "coregraphics/memoryindexbufferpool.h"
namespace CoreGraphics
{

MemoryIndexBufferPool* iboPool = nullptr;

//------------------------------------------------------------------------------
/**
*/
const IndexBufferId
CreateIndexBuffer(IndexBufferCreateInfo info)
{
	IndexBufferId id = iboPool->ReserveResource(info.name, info.tag);
	n_assert(id.allocType == IndexBufferIdType);
	iboPool->LoadFromMemory(id, &info);
	return id;
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyIndexBuffer(const IndexBufferId id)
{
	iboPool->DiscardResource(id);
}

//------------------------------------------------------------------------------
/**
*/
void
IndexBufferBind(const IndexBufferId id, const IndexT offset)
{
	iboPool->Bind(id, offset);
}

//------------------------------------------------------------------------------
/**
*/
void
IndexBufferUpdate(const IndexBufferId id, void* data, PtrDiff size, PtrDiff offset)
{
}

//------------------------------------------------------------------------------
/**
*/
void
IndexBufferLock(const IndexBufferId id, const PtrDiff offset, const PtrDiff range)
{
	// implement me?
}

//------------------------------------------------------------------------------
/**
*/
void
IndexBufferUnlock(const IndexBufferId id, const PtrDiff offset, const PtrDiff range)
{
	// implement me?
}

//------------------------------------------------------------------------------
/**
*/
void*
IndexBufferMap(const IndexBufferId id, const CoreGraphics::GpuBufferTypes::MapType type)
{
	return iboPool->Map(id, type);
}

//------------------------------------------------------------------------------
/**
*/
void
IndexBufferUnmap(const IndexBufferId id)
{
	iboPool->Unmap(id);
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::IndexType::Code
IndexBufferGetType(const IndexBufferId id)
{
	return iboPool->GetIndexType(id);
}

//------------------------------------------------------------------------------
/**
*/
const SizeT
IndexBufferGetNumIndices(const IndexBufferId id)
{
	return iboPool->GetNumIndices(id);
}

} // CoreGraphics
