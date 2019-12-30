#pragma once
//------------------------------------------------------------------------------
/**
	Mesh collects vertex and index buffers with primitive groups which can be used to render with

	(C)2017-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "coregraphics/config.h"
#include "resources/resourceid.h"
#include "ids/id.h"
#include "ids/idallocator.h"
#include "coregraphics/vertexbuffer.h"
#include "coregraphics/indexbuffer.h"
#include "coregraphics/primitivetopology.h"
#include "coregraphics/primitivegroup.h"
namespace CoreGraphics
{

RESOURCE_ID_TYPE(MeshId);

struct MeshCreateInfo
{
	struct Stream
	{
		VertexBufferId vertexBuffer;
		IndexT index;
	};

	Resources::ResourceName name;
	Util::StringAtom tag;
	Util::ArrayStack<Stream, 16> streams;
	IndexBufferId indexBuffer;
	VertexLayoutId vertexLayout;
	CoreGraphics::PrimitiveTopology::Code topology;
	Util::Array<CoreGraphics::PrimitiveGroup> primitiveGroups;
};

/// create new mesh
const MeshId CreateMesh(const MeshCreateInfo& info);
/// destroy mesh
void DestroyMesh(const MeshId id);

/// bind mesh and primitive group
void MeshBind(const MeshId id, const IndexT prim);
/// get number of primitive groups
const Util::Array<CoreGraphics::PrimitiveGroup>& MeshGetPrimitiveGroups(const MeshId id);
/// get vertex buffer
const VertexBufferId MeshGetVertexBuffer(const MeshId id, const IndexT stream);
/// get index buffer
const IndexBufferId MeshGetIndexBuffer(const MeshId id);
/// get topology
const CoreGraphics::PrimitiveTopology::Code MeshGetTopology(const MeshId id);

class MemoryMeshPool;
extern MemoryMeshPool* meshPool;
} // CoreGraphics
