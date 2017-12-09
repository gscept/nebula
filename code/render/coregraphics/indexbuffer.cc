//------------------------------------------------------------------------------
//  indexbuffer.cc
//  (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
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
	iboPool->LoadFromMemory(id.id24, &info);
	return id;
}

//------------------------------------------------------------------------------
/**
*/
inline void
DestroyIndexBuffer(const IndexBufferId id)
{
	iboPool->DiscardResource(id.id24);
}

//------------------------------------------------------------------------------
/**
*/
inline void
BindIndexBuffer(const IndexBufferId id, const IndexT IndexOffset)
{
	iboPool->BindIndexBuffer(id.id24);
}

//------------------------------------------------------------------------------
/**
*/
inline void
UpdateIndexBuffer(const IndexBufferId id, void* data, PtrDiff size, PtrDiff offset)
{
}

//------------------------------------------------------------------------------
/**
*/
inline void
LockIndexBuffer(const IndexBufferId id, const PtrDiff offset, const PtrDiff range)
{
	// implement me?
}

//------------------------------------------------------------------------------
/**
*/
inline void
UnlockIndexBuffer(const IndexBufferId id, const PtrDiff offset, const PtrDiff range)
{
	// implement me?
}

//------------------------------------------------------------------------------
/**
*/
inline void*
MapIndexBuffer(const IndexBufferId id, const CoreGraphics::GpuBufferTypes::MapType type)
{
	return iboPool->Map(id.id24, type);
}

//------------------------------------------------------------------------------
/**
*/
inline void
UnmapIndexBuffer(const IndexBufferId id)
{
	iboPool->Unmap(id.id24);
}

} // CoreGraphics
