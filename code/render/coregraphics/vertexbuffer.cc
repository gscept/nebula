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
	if (vboPool->GetState(id) == Resources::Resource::Pending)
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
VertexBufferUpdate(const VertexBufferId id, void* data, PtrDiff size, PtrDiff offset)
{
	n_error("Not implemented");
}

//------------------------------------------------------------------------------
/**
*/
void
VertexBufferLock(const VertexBufferId id, const PtrDiff offset, const PtrDiff range)
{
	n_error("Not implemented");
}

//------------------------------------------------------------------------------
/**
*/
void
VertexBufferUnlock(const VertexBufferId id, const PtrDiff offset, const PtrDiff range)
{
	n_error("Not implemented");
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

