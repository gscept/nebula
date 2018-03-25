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
#include "coregraphics/mesh.h"
#include "coregraphics/vertexsignaturepool.h"
#include "coregraphics/vertexcomponent.h"
#include "threading/thread.h"
#include "coregraphics/shaderserver.h"
#include "resources/resourceid.h"
#include "resources/resourcemanager.h"
#include "coregraphics/shadersemantics.h"
#include "coregraphics/memoryvertexbufferpool.h"
#include "coregraphics/memoryindexbufferpool.h"

using namespace Base;
using namespace Threading;
using namespace Math;
using namespace CoreGraphics;
using namespace Threading;
using namespace Resources;
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
	n_assert(this->shapeShaderState == ShaderStateId::Invalid());

	ShapeRendererBase::Open();

	// create shape shader instance
	this->shapeShader = ShaderServer::Instance()->GetShader("shd:simple"_atm);
	this->shapeShaderState = ShaderCreateState(this->shapeShader, { NEBULAT_BATCH_GROUP }, false);
	this->shapeMeshResources.SetSize(CoreGraphics::RenderShape::NumShapeTypes);
	this->shapeMeshes.SetSize(CoreGraphics::RenderShape::NumShapeTypes);

	// create default shapes (basically load them from the models)
	this->CreateBoxShape();
	this->CreateCylinderShape();
	this->CreateSphereShape();
	this->CreateTorusShape();
	this->CreateConeShape();

	// lookup ModelViewProjection shader variable
	this->model = ShaderStateGetConstant(this->shapeShaderState, "ShapeModel");
	this->diffuseColor = ShaderStateGetConstant(this->shapeShaderState, "MatDiffuse");

	this->programs[RenderShape::AlwaysOnTop] = ShaderGetProgram(this->shapeShader, ShaderServer::Instance()->FeatureStringToMask("Colored"));
	this->programs[RenderShape::CheckDepth] = ShaderGetProgram(this->shapeShader, ShaderServer::Instance()->FeatureStringToMask("Colored|Alt0"));
	this->programs[RenderShape::Wireframe] = ShaderGetProgram(this->shapeShader, ShaderServer::Instance()->FeatureStringToMask("Colored|Alt1"));
	this->programs[RenderShape::AlwaysOnTop + RenderShape::NumDepthFlags] = ShaderGetProgram(this->shapeShader, ShaderServer::Instance()->FeatureStringToMask("Static"));
	this->programs[RenderShape::CheckDepth + RenderShape::NumDepthFlags] = ShaderGetProgram(this->shapeShader, ShaderServer::Instance()->FeatureStringToMask("Static|Alt0"));
	this->programs[RenderShape::Wireframe + RenderShape::NumDepthFlags] = ShaderGetProgram(this->shapeShader, ShaderServer::Instance()->FeatureStringToMask("Static|Alt1"));

	Util::Array<VertexComponent> comps;
	comps.Append(VertexComponent(VertexComponent::Position, 0, VertexComponent::Float4, 0));
	comps.Append(VertexComponent(VertexComponent::Color, 0, VertexComponent::Float4, 0));
	VertexBufferCreateInfo vboInfo = 
	{
		"shape_renderer_vbo",
		"render_system"_atm,
		CoreGraphics::GpuBufferTypes::AccessWrite,
		CoreGraphics::GpuBufferTypes::UsageDynamic,
		CoreGraphics::GpuBufferTypes::SyncingCoherent,
		MaxNumVertices,
		comps, 
		nullptr,
		0
	};
	this->vbo = CreateVertexBuffer(vboInfo);

	IndexBufferCreateInfo iboInfo =
	{
		"shape_renderer_ibo",
		"render_system"_atm,
		CoreGraphics::GpuBufferTypes::AccessWrite,
		CoreGraphics::GpuBufferTypes::UsageDynamic,
		CoreGraphics::GpuBufferTypes::SyncingCoherent,
		IndexType::Index32,
		MaxNumIndices,
		nullptr,
		0
	};
	this->ibo = CreateIndexBuffer(iboInfo);

	// map buffers
	this->vertexBufferPtr = (byte*)VertexBufferMap(this->vbo, CoreGraphics::GpuBufferTypes::MapWrite);
	this->indexBufferPtr = (byte*)IndexBufferMap(this->ibo, CoreGraphics::GpuBufferTypes::MapWrite);
	n_assert(0 != this->vertexBufferPtr);
	n_assert(0 != this->indexBufferPtr);

	// also create an extra vertex layout, in case we get a mesh which doesn't fit with our special layout
	this->vertexLayout = CreateVertexLayout(VertexLayoutCreateInfo{ comps });

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
	n_assert(this->shapeShaderState != ShaderStateId::Invalid());

	this->diffuseColor = 0;
	this->model = 0;

	// unload shape meshes
	DiscardResource(this->shapeMeshResources[RenderShape::Box]);
	DiscardResource(this->shapeMeshResources[RenderShape::Sphere]);
	DiscardResource(this->shapeMeshResources[RenderShape::Cylinder]);
	DiscardResource(this->shapeMeshResources[RenderShape::Torus]);
	DiscardResource(this->shapeMeshResources[RenderShape::Cone]);
	this->shapeMeshResources.Clear();

	// discard shape shader
	ShaderDestroyState(this->shapeShaderState);

	// unload dynamic buffers
	VertexBufferUnmap(this->vbo);
	IndexBufferUnmap(this->ibo);

	DestroyVertexBuffer(this->vbo);
	DestroyIndexBuffer(this->ibo);
	this->vertexBufferPtr = this->indexBufferPtr = nullptr;

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
		ShaderProgramBind(this->programs[depthType]);

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
		ShaderProgramBind(this->programs[depthType + RenderShape::NumDepthFlags]);

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
	n_assert(this->shapeMeshes[shapeType] != ResourceId::Invalid());
	n_assert(shapeType < RenderShape::NumShapeTypes);

	Ptr<RenderDevice> renderDevice = RenderDevice::Instance();

	// resolve model-view-projection matrix and update shader
	ShaderConstantSet(this->model, this->shapeShaderState, modelTransform);
	ShaderConstantSet(this->diffuseColor, this->shapeShaderState, color);

	const SizeT numgroups = MeshGetPrimitiveGroups(this->shapeMeshes[shapeType]).Size();
	IndexT i;
	for (i = 0; i < numgroups; i++)
	{
		MeshBind(this->shapeMeshes[shapeType], i);
		ShaderStateApply(this->shapeShaderState);

		// draw
		renderDevice->Draw();
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VkShapeRenderer::DrawMesh(const Math::matrix44& modelTransform, const CoreGraphics::MeshId mesh, const Math::float4& color)
{
	n_assert(mesh != MeshId::Invalid());
	Ptr<RenderDevice> renderDevice = RenderDevice::Instance();

	// resolve model-view-projection matrix and update shader
	TransformDevice* transDev = TransformDevice::Instance();

	// resolve model-view-projection matrix and update shader
	ShaderConstantSet(this->model, this->shapeShaderState, modelTransform);
	ShaderConstantSet(this->diffuseColor, this->shapeShaderState, color);

	n_assert(RenderDevice::Instance()->IsInBeginFrame());
	const SizeT numgroups = MeshGetPrimitiveGroups(mesh).Size();

	// draw primitives in shape
	IndexT i;
	for (i = 0; i < numgroups; i++)
	{
		MeshBind(mesh, i);
		ShaderStateApply(this->shapeShaderState);

		// draw
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

		ShaderConstantSet(this->model, this->shapeShaderState, modelTransform);
		ShaderConstantSet(this->diffuseColor, this->shapeShaderState, color);

		renderDevice->SetPrimitiveTopology(topo);
		VertexLayoutBind(this->vertexLayout);
		VertexBufferBind(this->vbo, 0, 0);
		renderDevice->SetPrimitiveGroup(group);

		ShaderStateApply(this->shapeShaderState);

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

		ShaderConstantSet(this->model, this->shapeShaderState, modelTransform);
		ShaderConstantSet(this->diffuseColor, this->shapeShaderState, color);
		
		renderDevice->SetPrimitiveTopology(topo);
		VertexLayoutBind(this->vertexLayout);
		IndexBufferBind(this->ibo, 0);
		VertexBufferBind(this->vbo, 0, 0);
		renderDevice->SetPrimitiveGroup(group);

		ShaderStateApply(this->shapeShaderState);

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
	this->shapeMeshResources[RenderShape::Box] = CreateResource("msh:system/box.nvx2", "render_system", nullptr, nullptr, true);
	this->shapeMeshes[RenderShape::Box] = MeshId(this->shapeMeshResources[RenderShape::Box].allocId);
}

//------------------------------------------------------------------------------
/**
*/
void
VkShapeRenderer::CreateSphereShape()
{
	this->shapeMeshResources[RenderShape::Sphere] = CreateResource("msh:system/sphere.nvx2", "render_system", nullptr, nullptr, true);
	this->shapeMeshes[RenderShape::Sphere] = MeshId(this->shapeMeshResources[RenderShape::Sphere].allocId);
}

//------------------------------------------------------------------------------
/**
*/
void
VkShapeRenderer::CreateCylinderShape()
{
	this->shapeMeshResources[RenderShape::Cylinder] = CreateResource("msh:system/cylinder.nvx2", "render_system", nullptr, nullptr, true);
	this->shapeMeshes[RenderShape::Cylinder] = MeshId(this->shapeMeshResources[RenderShape::Cylinder].allocId);
}

//------------------------------------------------------------------------------
/**
*/
void
VkShapeRenderer::CreateTorusShape()
{
	this->shapeMeshResources[RenderShape::Torus] = CreateResource("msh:system/torus.nvx2", "render_system", nullptr, nullptr, true);
	this->shapeMeshes[RenderShape::Torus] = MeshId(this->shapeMeshResources[RenderShape::Torus].allocId);
}

//------------------------------------------------------------------------------
/**
*/
void
VkShapeRenderer::CreateConeShape()
{
	this->shapeMeshResources[RenderShape::Cone] = CreateResource("msh:system/cone.nvx2", "render_system", nullptr, nullptr, true);
	this->shapeMeshes[RenderShape::Cone] = MeshId(this->shapeMeshResources[RenderShape::Cone].allocId);
}

} // namespace Vulkan