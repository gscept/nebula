#pragma once
//------------------------------------------------------------------------------
/**
	Vertex buffer related functions.

	The actual allocation is handled by the MemoryVertexBufferPool

	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/id.h"
#include "ids/idpool.h"
#include "resources/resourceid.h"
#include "gpubuffertypes.h"
#include "coregraphics/vertexcomponent.h"
namespace CoreGraphics
{

ID_32_24_8_TYPE(VertexBufferId);

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

/// create new vertex buffer with intended usage, access and CPU syncing parameters, together with size of buffer
const VertexBufferId CreateVertexBuffer(VertexBufferCreateInfo info);
/// destroy vertex buffer
void DestroyVertexBuffer(const VertexBufferId id);
/// bind vertex buffer resource individually
void BindVertexBuffer(const VertexBufferId id, const IndexT slot, const IndexT vertexOffset);
/// update vertex buffer
void UpdateVertexBuffer(const VertexBufferId id, void* data, PtrDiff size, PtrDiff offset);
/// request lock for vertex buffer, such that it can be updated
void LockVertexBuffer(const VertexBufferId id, const PtrDiff offset, const PtrDiff range);
/// request unlock for vertex buffer
void UnlockVertexBuffer(const VertexBufferId id, const PtrDiff offset, const PtrDiff range);
/// map GPU memory
void* MapVertexBuffer(const VertexBufferId id, const CoreGraphics::GpuBufferTypes::MapType type);
/// unmap GPU memory
void UnmapVertexBuffer(const VertexBufferId id);

class MemoryVertexBufferPool;
extern MemoryVertexBufferPool* vboPool;


} // CoreGraphics
