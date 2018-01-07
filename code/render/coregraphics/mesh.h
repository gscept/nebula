#pragma once
//------------------------------------------------------------------------------
/**
	Mesh collects vertex and index buffers with primitive groups which can be used to render with

	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/id.h"
#include "ids/idallocator.h"
#include "coregraphics/vertexbuffer.h"
#include "coregraphics/indexbuffer.h"
#include "coregraphics/vertexlayout.h"
#include "coregraphics/primitivetopology.h"
#include "coregraphics/primitivegroup.h"
namespace CoreGraphics
{
ID_24_8_TYPE(MeshId);

struct MeshCreateInfo
{
	VertexBufferId vertexBuffer;
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

class MemoryMeshPool;
extern MemoryMeshPool* meshPool;
} // CoreGraphics
