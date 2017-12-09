//------------------------------------------------------------------------------
// vkshaperenderer.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "vkshaperenderer.h"
#include "coregraphics/vk/vktypes.h"
#include "coregraphics/renderdevice.h"
#include "coregraphics/transformdevice.h"
#include "coregraphics/renderdevice.h"
#include "coregraphics/vertexbuffer.h"
#include "coregraphics/indexbuffer.h"
#include "coregraphics/meshpool.h"
#include "coregraphics/mesh.h"
#include "coregraphics/vertexlayoutserver.h"
#include "coregraphics/vertexcomponent.h"
#include "threading/thread.h"
#include "coregraphics/shaderserver.h"
#include "resources/resourceid.h"
#include "resources/resourcemanager.h"
#include "models/modelinstance.h"
#include "coregraphics/shadersemantics.h"
#include "coregraphics/memoryvertexbufferpool.h"
#include "coregraphics/memoryindexbufferpool.h"

using namespace Base;
using namespace Threading;
using namespace Math;
using namespace CoreGraphics;
using namespace Threading;
using namespace Resources;
using namespace Models;
namespace Vulkan
{

__ImplementClass(Vulkan::VkShapeRenderer, 'VKSR', Base::ShapeRendererBase);
//------------------------------------------------------------------------------
/**
*/
VkShapeRenderer::VkShapeRenderer()
{
	this->shapeMeshes.Clear();
}

//------------------------------------------------------------------------------
/**
*/
VkShapeRenderer::~VkShapeRenderer()
{
	n_assert(!this->IsOpen());
}

//------------------------------------------------------------------------------
/**
*/
void
VkShapeRenderer::Open()
{
	n_assert(!this->IsOpen());
	n_assert(!this->shapeShader.isvalid());

	ShapeRendererBase::Open();

	// create shape shader instance
	this->shapeShader = ShaderServer::Instance()->CreateShaderState("shd:simple", { NEBULAT_SYSTEM_GROUP });
	this->shapeMeshes.SetSize(CoreGraphics::RenderShape::NumShapeTypes);

	// create default shapes (basically load them from the models)
	this->CreateBoxShape();
	this->CreateCylinderShape();
	this->CreateSphereShape();
	this->CreateTorusShape();
	this->CreateConeShape();

	// lookup ModelViewProjection shader variable
	this->model = this->shapeShader->GetVariableByName("ShapeModel");
	this->diffuseColor = this->shapeShader->GetVariableByName("MatDiffuse");

	// create feature masks
	this->featureBits[RenderShape::AlwaysOnTop] = ShaderServer::Instance()->FeatureStringToMask("Colored");
	this->featureBits[RenderShape::CheckDepth] = ShaderServer::Instance()->FeatureStringToMask("Colored|Alt0");
	this->featureBits[RenderShape::Wireframe] = ShaderServer::Instance()->FeatureStringToMask("Colored|Alt1");

	// use these for primitives
	this->featureBits[RenderShape::AlwaysOnTop + RenderShape::NumDepthFlags] = ShaderServer::Instance()->FeatureStringToMask("Static");
	this->featureBits[RenderShape::CheckDepth + RenderShape::NumDepthFlags] = ShaderServer::Instance()->FeatureStringToMask("Static|Alt0");
	this->featureBits[RenderShape::Wireframe + RenderShape::NumDepthFlags] = ShaderServer::Instance()->FeatureStringToMask("Static|Alt1");

	// setup vbo
	Util::Array<VertexComponent> comps;
	comps.Append(VertexComponent(VertexComponent::Position, 0, VertexComponent::Float4, 0));
	comps.Append(VertexComponent(VertexComponent::Color, 0, VertexComponent::Float4, 0));
	Ptr<MemoryVertexBufferPool> vboLoader = MemoryVertexBufferPool::Create();
	vboLoader->Setup(comps, MaxNumVertices, NULL, 0, VertexBuffer::UsageDynamic, VertexBuffer::AccessWrite, VertexBuffer::SyncingCoherent);

	// create vbo
	this->vbo = VertexBuffer::Create();
	this->vbo->SetLoader(vboLoader.upcast<ResourceLoader>());
	this->vbo->SetAsyncEnabled(false);
	this->vbo->Load();
	n_assert(this->vbo->IsLoaded());
	this->vbo->SetLoader(NULL);

	// setup ibo
	Ptr<MemoryIndexBufferPool> iboLoader = MemoryIndexBufferPool::Create();
	iboLoader->Setup(IndexType::Index32, MaxNumIndices, NULL, 0, IndexBuffer::UsageDynamic, IndexBuffer::AccessWrite, IndexBuffer::SyncingCoherent);

	// create ibo
	this->ibo = IndexBuffer::Create();
	this->ibo->SetLoader(iboLoader.upcast<ResourceLoader>());
	this->ibo->SetAsyncEnabled(false);
	this->ibo->Load();
	n_assert(this->ibo->IsLoaded());
	this->ibo->SetLoader(NULL);

	// create buffer locks
	this->vboLock = BufferLock::Create();
	this->iboLock = BufferLock::Create();

	// map buffers
	this->vertexBufferPtr = (byte*)this->vbo->Map(VertexBuffer::MapWrite);
	this->indexBufferPtr = (byte*)this->ibo->Map(IndexBuffer::MapWrite);
	n_assert(0 != this->vertexBufferPtr);
	n_assert(0 != this->indexBufferPtr);

	// bind index buffer to vertex layout
	this->vertexLayout = this->vbo->GetVertexLayout();

	// setup primitive group
	this->primGroup.SetBaseIndex(0);
	this->primGroup.SetBaseVertex(0);
}

//------------------------------------------------------------------------------
/**
*/
void
VkShapeRenderer::Close()
{
	n_assert(this->IsOpen());
	n_assert(this->shapeShader.isvalid());

	this->diffuseColor = 0;
	this->model = 0;

	// unload shape meshes
	ResourceManager::Instance()->DiscardManagedResource(this->shapeMeshes[RenderShape::Box].upcast<ManagedResource>());
	ResourceManager::Instance()->DiscardManagedResource(this->shapeMeshes[RenderShape::Sphere].upcast<ManagedResource>());
	ResourceManager::Instance()->DiscardManagedResource(this->shapeMeshes[RenderShape::Cylinder].upcast<ManagedResource>());
	ResourceManager::Instance()->DiscardManagedResource(this->shapeMeshes[RenderShape::Torus].upcast<ManagedResource>());
	ResourceManager::Instance()->DiscardManagedResource(this->shapeMeshes[RenderShape::Cone].upcast<ManagedResource>());
	this->shapeMeshes.Clear();

	// discard shape shader
	this->shapeShader->Discard();
	this->shapeShader = 0;

	// unload dynamic buffers
	this->vbo->Unmap();
	this->ibo->Unmap();
	this->vbo->Unload();
	this->vbo = 0;
	this->ibo->Unload();
	this->ibo = 0;
	this->vertexBufferPtr = this->indexBufferPtr = 0;

	// call parent class
	ShapeRendererBase::Close();
}

//------------------------------------------------------------------------------
/**
*/
void
VkShapeRenderer::DrawShapes()
{
	n_assert(this->IsOpen());

	for (int depthType = 0; depthType < RenderShape::NumDepthFlags; depthType++)
	{
		this->shapeShader->SelectActiveVariation(featureBits[depthType]);
		this->shapeShader->Apply();

		IndexT i;
		for (i = 0; i < this->primitives[depthType].Size(); i++)
		{
			const RenderShape& curShape = this->primitives[depthType][i];
			if (curShape.GetShapeType() == RenderShape::Primitives)
				this->DrawPrimitives(curShape.GetModelTransform(),
				curShape.GetTopology(),
				curShape.GetNumPrimitives(),
				curShape.GetVertexData(),
				curShape.GetVertexWidth(),
				curShape.GetColor());
			else if (curShape.GetShapeType() == RenderShape::IndexedPrimitives)
				this->DrawIndexedPrimitives(curShape.GetModelTransform(),
				curShape.GetTopology(),
				curShape.GetNumPrimitives(),
				curShape.GetVertexData(),
				curShape.GetNumVertices(),
				curShape.GetVertexWidth(),
				curShape.GetIndexData(),
				curShape.GetIndexType(),
				curShape.GetColor());
			else n_error("Shape type %d is not a primitive!", curShape.GetShapeType());
		}

		// flush any buffered primitives
		this->DrawBufferedIndexedPrimitives();
		this->DrawBufferedPrimitives();

		// reset index and primitive counters
		this->numIndices = 0;
		this->numPrimitives = 0;
	}

	for (int depthType = 0; depthType < RenderShape::NumDepthFlags; depthType++)
	{
		this->shapeShader->SelectActiveVariation(featureBits[depthType + RenderShape::NumDepthFlags]);
		this->shapeShader->Apply();

		IndexT i;
		for (i = 0; i < this->shapes[depthType].Size(); i++)
		{
			const RenderShape& curShape = this->shapes[depthType][i];
			if (curShape.GetShapeType() == RenderShape::RenderMesh) this->DrawMesh(curShape.GetModelTransform(), curShape.GetMesh(), curShape.GetColor());
			else													this->DrawSimpleShape(curShape.GetModelTransform(), curShape.GetShapeType(), curShape.GetColor());
		}
	}

	// delete the shapes of my own thread id, all other shapes
	// are from other threads and will be deleted through DeleteShapesByThreadId()
	this->DeleteShapesByThreadId(Thread::GetMyThreadId());
}

//------------------------------------------------------------------------------
/**
*/
void
VkShapeRenderer::DrawSimpleShape(const Math::matrix44& modelTransform, CoreGraphics::RenderShape::Type shapeType, const Math::float4& color)
{
	n_assert(this->shapeMeshes[shapeType].isvalid());
	n_assert(shapeType < RenderShape::NumShapeTypes);

	Ptr<RenderDevice> renderDevice = RenderDevice::Instance();

	// resolve model-view-projection matrix and update shader
	this->model->SetMatrix(modelTransform);
	this->diffuseColor->SetFloat4(color);

	Ptr<Mesh> mesh = this->shapeMeshes[shapeType]->GetMesh();
	Ptr<VertexBuffer> vb = mesh->GetVertexBuffer();
	Ptr<IndexBuffer> ib = mesh->GetIndexBuffer();

	// assume all primitive groups in the mesh are identical
	mesh->ApplySharedMesh();

	IndexT i;
	for (i = 0; i < mesh->GetNumPrimitiveGroups(); i++)
	{
		// setup render device
		PrimitiveGroup group = mesh->GetPrimitiveGroupAtIndex(i);
		renderDevice->SetPrimitiveGroup(group);
		this->shapeShader->Commit();

		// draw
		renderDevice->Draw();
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VkShapeRenderer::DrawMesh(const Math::matrix44& modelTransform, const Ptr<CoreGraphics::Mesh>& mesh, const Math::float4& color)
{
	n_assert(mesh.isvalid());
	Ptr<RenderDevice> renderDevice = RenderDevice::Instance();

	// resolve model-view-projection matrix and update shader
	TransformDevice* transDev = TransformDevice::Instance();
	this->model->SetMatrix(modelTransform);
	this->diffuseColor->SetFloat4(color);

	n_assert(RenderDevice::Instance()->IsInBeginFrame());

	Ptr<CoreGraphics::VertexBuffer> vb = mesh->GetVertexBuffer();
	Ptr<CoreGraphics::IndexBuffer> ib = mesh->GetIndexBuffer();

	// assume all primitives share the same topology
	mesh->ApplySharedMesh();

	// draw primitives in shape
	IndexT i;
	for (i = 0; i < mesh->GetNumPrimitiveGroups(); i++)
	{
		// setup render device
		PrimitiveGroup group = mesh->GetPrimitiveGroupAtIndex(i);
		renderDevice->SetPrimitiveGroup(group);
		this->shapeShader->Commit();

		renderDevice->Draw();
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VkShapeRenderer::DrawPrimitives(const Math::matrix44& modelTransform, CoreGraphics::PrimitiveTopology::Code topology, SizeT numPrimitives, const void* vertices, SizeT vertexWidth, const Math::float4& color)
{
	n_assert(0 != vertices);
	n_assert(vertexWidth <= MaxVertexWidth);

	// calculate vertex count
	SizeT vertexCount = PrimitiveTopology::NumberOfVertices(topology, numPrimitives);
	vertexCount = Math::n_min(vertexCount, MaxNumVertices);

	// flush primitives to make room in our buffers
	n_assert(this->numPrimitives + vertexCount <= MaxNumVertices);

	SizeT bufferSize = MaxNumVertices;
	CoreGraphics::RenderShape::RenderShapeVertex* verts = (CoreGraphics::RenderShape::RenderShapeVertex*)this->vertexBufferPtr;

	// unlock buffer to avoid stomping data
	memcpy(verts + this->numPrimitives, vertices, vertexCount * vertexWidth);

	// append transforms
	this->unindexed.transforms.Append(modelTransform);
	this->unindexed.colors.Append(color);

	// set vertex offset in primitive group
	CoreGraphics::PrimitiveGroup group;
	group.SetBaseVertex(this->numPrimitives);
	group.SetNumVertices(vertexCount);
	group.SetNumIndices(0);
	this->unindexed.topologies.Append(topology);
	this->unindexed.primitives.Append(group);
	this->numPrimitives += vertexCount;
}

//------------------------------------------------------------------------------
/**
*/
void
VkShapeRenderer::DrawIndexedPrimitives(const Math::matrix44& modelTransform, CoreGraphics::PrimitiveTopology::Code topology, SizeT numPrimitives, const void* vertices, SizeT numVertices, SizeT vertexWidth, const void* indices, CoreGraphics::IndexType::Code indexType, const Math::float4& color)
{
	n_assert(0 != vertices);
	n_assert(0 != indices);
	n_assert(vertexWidth <= MaxVertexWidth);

	// calculate index count and size of index type
	SizeT indexCount = PrimitiveTopology::NumberOfVertices(topology, numPrimitives);
	SizeT indexSize = CoreGraphics::IndexType::SizeOf(indexType);
	SizeT vertexCount = Math::n_min(numVertices, MaxNumVertices);
	indexCount = Math::n_min(indexCount, MaxNumIndices);

	n_assert(this->numPrimitives + vertexCount <= MaxNumVertices);
	n_assert(this->numIndices + indexCount <= MaxNumIndices);

	SizeT vbBufferSize = MaxNumVertices * MaxVertexWidth;
	SizeT ibBufferSize = MaxNumIndices * MaxIndexWidth;
	CoreGraphics::RenderShape::RenderShapeVertex* verts = (CoreGraphics::RenderShape::RenderShapeVertex*)this->vertexBufferPtr;
	uint32_t* inds = (uint32_t*)this->indexBufferPtr;

	// unlock buffer and copy data
	memcpy(verts + this->numPrimitives, vertices, vertexCount * vertexWidth);
	memcpy(inds + this->numIndices, indices, indexCount * indexSize);

	// append transforms
	this->indexed.transforms.Append(modelTransform);
	this->indexed.colors.Append(color);

	// set vertex offset in primitive group
	CoreGraphics::PrimitiveGroup group;
	group.SetBaseVertex(this->numPrimitives);
	group.SetNumVertices(0);										// indices decides how many primitives we draw
	group.SetBaseIndex(this->numIndices);
	group.SetNumIndices(indexCount);
	this->indexed.topologies.Append(topology);
	this->indexed.primitives.Append(group);
	this->numPrimitives += vertexCount;
	this->numIndices += indexCount;
}

//------------------------------------------------------------------------------
/**
*/
void
VkShapeRenderer::DrawBufferedPrimitives()
{
	Ptr<RenderDevice> renderDevice = RenderDevice::Instance();
	//renderDevice->SetPrimitiveGroup(this->primGroup);
	
	IndexT i;
	for (i = 0; i < this->unindexed.primitives.Size(); i++)
	{
		const CoreGraphics::PrimitiveGroup& group = this->unindexed.primitives[i];
		const CoreGraphics::PrimitiveTopology::Code& topo = this->unindexed.topologies[i];
		const Math::matrix44& modelTransform = this->unindexed.transforms[i];
		const Math::float4& color = this->unindexed.colors[i];

		this->diffuseColor->SetFloat4(color);
		this->model->SetMatrix(modelTransform);

		renderDevice->SetPrimitiveTopology(topo);
		renderDevice->SetVertexLayout(this->vertexLayout);
		renderDevice->SetStreamVertexBuffer(0, this->vbo, 0);
		renderDevice->SetPrimitiveGroup(group);

		this->shapeShader->Commit();

		renderDevice->Draw();
	}

	this->unindexed.primitives.Clear();
	this->unindexed.transforms.Clear();
	this->unindexed.colors.Clear();
	this->unindexed.topologies.Clear();
}

//------------------------------------------------------------------------------
/**
*/
void
VkShapeRenderer::DrawBufferedIndexedPrimitives()
{
	Ptr<RenderDevice> renderDevice = RenderDevice::Instance();
	//renderDevice->SetPrimitiveGroup(this->primGroup);

	IndexT i;
	for (i = 0; i < this->indexed.primitives.Size(); i++)
	{
		const CoreGraphics::PrimitiveGroup& group = this->indexed.primitives[i];
		const CoreGraphics::PrimitiveTopology::Code& topo = this->indexed.topologies[i];
		const Math::matrix44& modelTransform = this->indexed.transforms[i];
		const Math::float4& color = this->indexed.colors[i];

		this->diffuseColor->SetFloat4(color);
		this->model->SetMatrix(modelTransform);
		
		renderDevice->SetPrimitiveTopology(topo);
		renderDevice->SetVertexLayout(this->vertexLayout);
		renderDevice->SetIndexBuffer(this->ibo);
		renderDevice->SetStreamVertexBuffer(0, this->vbo, 0);
		renderDevice->SetPrimitiveGroup(group);
		//renderDevice->BuildRenderPipeline();
		this->shapeShader->Commit();

		renderDevice->Draw();
	}

	this->indexed.primitives.Clear();
	this->indexed.transforms.Clear();
	this->indexed.colors.Clear();
	this->indexed.topologies.Clear();
}

//------------------------------------------------------------------------------
/**
*/
void
VkShapeRenderer::CreateBoxShape()
{
	Ptr<ManagedMesh> mesh = ResourceManager::Instance()->CreateManagedResource(Mesh::RTTI, "msh:system/box.nvx2", StreamMeshPool::Create()).downcast<ManagedMesh>();
	this->shapeMeshes[RenderShape::Box] = mesh;
}

//------------------------------------------------------------------------------
/**
*/
void
VkShapeRenderer::CreateSphereShape()
{
	Ptr<ManagedMesh> mesh = ResourceManager::Instance()->CreateManagedResource(Mesh::RTTI, "msh:system/sphere.nvx2", StreamMeshPool::Create()).downcast<ManagedMesh>();
	this->shapeMeshes[RenderShape::Sphere] = mesh;
}

//------------------------------------------------------------------------------
/**
*/
void
VkShapeRenderer::CreateCylinderShape()
{
	Ptr<ManagedMesh> mesh = ResourceManager::Instance()->CreateManagedResource(Mesh::RTTI, "msh:system/cylinder.nvx2", StreamMeshPool::Create()).downcast<ManagedMesh>();
	this->shapeMeshes[RenderShape::Cylinder] = mesh;
}

//------------------------------------------------------------------------------
/**
*/
void
VkShapeRenderer::CreateTorusShape()
{
	Ptr<ManagedMesh> mesh = ResourceManager::Instance()->CreateManagedResource(Mesh::RTTI, "msh:system/torus.nvx2", StreamMeshPool::Create()).downcast<ManagedMesh>();
	this->shapeMeshes[RenderShape::Torus] = mesh;
}

//------------------------------------------------------------------------------
/**
*/
void
VkShapeRenderer::CreateConeShape()
{
	Ptr<ManagedMesh> mesh = ResourceManager::Instance()->CreateManagedResource(Mesh::RTTI, "msh:system/cone.nvx2", StreamMeshPool::Create()).downcast<ManagedMesh>();
	this->shapeMeshes[RenderShape::Cone] = mesh;
}

} // namespace Vulkan