//------------------------------------------------------------------------------
// coregraphics.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "coregraphics.h"
#include "shaderpool.h"
#include "texturepool.h"
#include "memoryindexbufferpool.h"
#include "memoryvertexbufferpool.h"
#include "models/modelpool.h"
#include "vertexsignaturepool.h"
#include "meshpool.h"

namespace CoreGraphics
{

ShaderPool* shaderPool = nullptr;
TexturePool* texturePool = nullptr;
MemoryVertexBufferPool* vboPool = nullptr;
MemoryIndexBufferPool* iboPool = nullptr;
MeshPool* meshPool = nullptr;
ModelPool* modelPool = nullptr;
VertexSignaturePool* layoutPool = nullptr;

//------------------------------------------------------------------------------
/**
*/
inline void
BindVertexLayout(const Resources::ResourceId id)
{
	layoutPool->BindVertexLayout(id);
}

//------------------------------------------------------------------------------
/**
*/
inline void
BindMesh(const Resources::ResourceId id)
{
	meshPool->BindMesh(id);
}

//------------------------------------------------------------------------------
/**
*/
inline void
BindPrimitiveGroup(const IndexT primgroup)
{
	meshPool->BindPrimitiveGroup(primgroup);
}

//------------------------------------------------------------------------------
/**
*/
inline void
BindShader(const Resources::ResourceId id, const ShaderFeature::Mask& program)
{
	shaderPool->BindShader(id, program);
}

//------------------------------------------------------------------------------
/**
*/
inline void
BindVertexBuffer(const Resources::ResourceId id, const IndexT slot, const IndexT vertexOffset)
{
	vboPool->BindVertexBuffer(id, slot, vertexOffset);
}

//------------------------------------------------------------------------------
/**
*/
inline void
BindIndexBuffer(const Resources::ResourceId id)
{
	iboPool->BindIndexBuffer(id);
}

//------------------------------------------------------------------------------
/**
*/
inline Resources::ResourceId
CreateVertexBuffer(Resources::ResourceName name, Util::StringAtom tag, Base::GpuResourceBase::Access access, Base::GpuResourceBase::Usage usage, SizeT numVerts, const Util::Array<VertexComponent>& comps, void* data, SizeT dataSize)
{
	Resources::ResourceId id = vboPool->ReserveResource(name, tag);
	MemoryVertexBufferPool::VertexBufferLoadInfo info;
	info.access = access;
	info.usage = usage;
	info.numVertices = numVerts;
	info.vertexComponents = comps;
	info.vertexDataPtr = data;
	info.vertexDataSize = dataSize;
	vboPool->UpdateResource(id, &info);
}

} // namespace CoreGraphics

namespace Shaders
{

//------------------------------------------------------------------------------
/**
*/
int
VariableCount(const CoreGraphics::ShaderId id)
{
	return CoreGraphics::shaderPool->GetVariableCount(id);
}

//------------------------------------------------------------------------------
/**
*/
int
VariableBlockCount(const CoreGraphics::ShaderId id)
{
	return CoreGraphics::shaderPool->GetVariableBlockCount(id);
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::ShaderId
GetShaderId(const CoreGraphics::ShaderId id)
{
	Ids::Id32 shaderId = Ids::Id::GetHigh(id);
	return shaderId;
}

}