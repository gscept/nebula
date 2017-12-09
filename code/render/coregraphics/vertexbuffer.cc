//------------------------------------------------------------------------------
//  vertexbuffer.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/vertexbuffer.h"
#include "memoryvertexbufferpool.h"
#include "config.h"

namespace CoreGraphics
{
MemoryVertexBufferPool* vboPool = nullptr;

using namespace Ids;

//------------------------------------------------------------------------------
/**
*/
const VertexBufferId
CreateVertexBuffer(VertexBufferCreateInfo info)
{
	VertexBufferId id = vboPool->ReserveResource(info.name, info.tag);
	vboPool->LoadFromMemory(id.id24, &info);
	return id;
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyVertexBuffer(const VertexBufferId id)
{
	vboPool->DiscardResource(id.id24);
}

//------------------------------------------------------------------------------
/**
*/
inline void
BindVertexBuffer(const VertexBufferId id, const IndexT slot, const IndexT vertexOffset)
{
	vboPool->BindVertexBuffer(id.id24, slot, vertexOffset);
}

//------------------------------------------------------------------------------
/**
*/
inline void
UpdateVertexBuffer(const VertexBufferId id, void* data, PtrDiff size, PtrDiff offset)
{
	//vboPool->LoadFromMemory(Resources::SharedId(id), 
}

//------------------------------------------------------------------------------
/**
*/
inline void
LockVertexBuffer(const VertexBufferId id, const PtrDiff offset, const PtrDiff range)
{
	// implement me?	
}

//------------------------------------------------------------------------------
/**
*/
inline void
UnlockVertexBuffer(const VertexBufferId id, const PtrDiff offset, const PtrDiff range)
{
	// implement me?
}

//------------------------------------------------------------------------------
/**
*/
inline void*
MapVertexBuffer(const VertexBufferId id, const CoreGraphics::GpuBufferTypes::MapType type)
{
	return vboPool->Map(id.id24, type);
}

//------------------------------------------------------------------------------
/**
*/
inline void
UnmapVertexBuffer(const VertexBufferId id)
{
	vboPool->Unmap(id.id24);
}

}

