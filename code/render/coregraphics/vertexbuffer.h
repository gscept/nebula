#pragma once
//------------------------------------------------------------------------------
/**
	Vertex buffer related functions.

	The actual allocation is handled by the MemoryVertexBufferPool

	(C)2017-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/id.h"
#include "ids/idpool.h"
#include "resources/resourceid.h"
#include "gpubuffertypes.h"
#include "coregraphics/vertexcomponent.h"
#include "vertexlayout.h"
namespace CoreGraphics
{

RESOURCE_ID_TYPE(VertexBufferId);

struct VertexBufferCreateInfo
{
	Resources::ResourceName name;
	Util::StringAtom tag;
	CoreGraphics::GpuBufferTypes::Access access;
	CoreGraphics::GpuBufferTypes::Usage usage;
	CoreGraphics::GpuBufferTypes::Syncing sync;
	SizeT numVerts;
	Util::Array<VertexComponent> comps;
	void* data;
	PtrDiff dataSize;
};

struct VertexBufferCreateDirectInfo
{
	Resources::ResourceName name;
	Util::StringAtom tag;
	CoreGraphics::GpuBufferTypes::Access access;
	CoreGraphics::GpuBufferTypes::Usage usage;
	CoreGraphics::GpuBufferTypes::Syncing sync;
	SizeT size;
};

/// create new vertex buffer with intended usage, access and CPU syncing parameters, together with size of buffer
const VertexBufferId CreateVertexBuffer(const VertexBufferCreateInfo& info);
/// create a new vertex buffer to be used directly (not as a resource)
const VertexBufferId CreateVertexBuffer(const VertexBufferCreateDirectInfo& info);
/// destroy vertex buffer
void DestroyVertexBuffer(const VertexBufferId id);

/// update vertex buffer
void VertexBufferUpdate(const VertexBufferId id, void* data, PtrDiff size, PtrDiff offset);
/// request lock for vertex buffer, such that it can be updated
void VertexBufferLock(const VertexBufferId id, const PtrDiff offset, const PtrDiff range);
/// request unlock for vertex buffer
void VertexBufferUnlock(const VertexBufferId id, const PtrDiff offset, const PtrDiff range);
/// map GPU memory
void* VertexBufferMap(const VertexBufferId id, const CoreGraphics::GpuBufferTypes::MapType type);
/// unmap GPU memory
void VertexBufferUnmap(const VertexBufferId id);

/// get number of vertices
const SizeT VertexBufferGetNumVertices(const VertexBufferId id);
/// get vertex layout
const VertexLayoutId VertexBufferGetLayout(const VertexBufferId id);

class MemoryVertexBufferPool;
extern MemoryVertexBufferPool* vboPool;


} // CoreGraphics
