//------------------------------------------------------------------------------
//  vertexbuffer.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
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
	n_assert(id.allocType == VertexBufferIdType);
	vboPool->LoadFromMemory(id, &info);
	return id;
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyVertexBuffer(const VertexBufferId id)
{
	vboPool->DiscardResource(id);
}

//------------------------------------------------------------------------------
/**
*/
void
VertexBufferBind(const VertexBufferId id, const IndexT slot, const IndexT vertexOffset)
{
	vboPool->Bind(id, slot, vertexOffset);
}

//------------------------------------------------------------------------------
/**
*/
void
VertexBufferUpdate(const VertexBufferId id, void* data, PtrDiff size, PtrDiff offset)
{
	//vboPool->LoadFromMemory(Resources::SharedId(id), 
}

//------------------------------------------------------------------------------
/**
*/
void
VertexBufferLock(const VertexBufferId id, const PtrDiff offset, const PtrDiff range)
{
	// implement me?	
}

//------------------------------------------------------------------------------
/**
*/
void
VertexBufferUnlock(const VertexBufferId id, const PtrDiff offset, const PtrDiff range)
{
	// implement me?
}

//------------------------------------------------------------------------------
/**
*/
void*
VertexBufferMap(const VertexBufferId id, const CoreGraphics::GpuBufferTypes::MapType type)
{
	return vboPool->Map(id, type);
}

//------------------------------------------------------------------------------
/**
*/
void
VertexBufferUnmap(const VertexBufferId id)
{
	vboPool->Unmap(id);
}

//------------------------------------------------------------------------------
/**
*/
const SizeT
VertexBufferGetNumVertices(const VertexBufferId id)
{
	return vboPool->GetNumVertices(id);
}

//------------------------------------------------------------------------------
/**
*/
const VertexLayoutId
VertexBufferGetLayout(const VertexBufferId id)
{
	return vboPool->GetLayout(id);
}

} // namespace CoreGraphics

