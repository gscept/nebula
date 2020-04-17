//------------------------------------------------------------------------------
// vkshaperenderer.cc
// (C) 2016-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "vkshaperenderer.h"
#include "coregraphics/vk/vktypes.h"
#include "coregraphics/graphicsdevice.h"
#include "coregraphics/transformdevice.h"
#include "coregraphics/vertexbuffer.h"
#include "coregraphics/indexbuffer.h"
#include "coregraphics/mesh.h"
#include "coregraphics/vertexsignaturepool.h"
#include "coregraphics/vertexcomponent.h"
#include "threading/thread.h"
#include "coregraphics/shaderserver.h"
#include "resources/resourceid.h"
#include "resources/resourceserver.h"
#include "coregraphics/shadersemantics.h"

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
	: indexBufferPtr(nullptr)
	, vertexBufferPtr(nullptr)
	, ibos{ IndexBufferId::Invalid() }
	, vbos{ VertexBufferId::Invalid() }
	, indexBufferActiveIndex()
	, vertexBufferActiveIndex()
	, indexBufferCapacity(0)
	, vertexBufferCapacity(0)
{
	// empty
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

	ShapeRendererBase::Open();

	// create shape shader instance
	this->shapeShader = ShaderServer::Instance()->GetShader("shd:simple.fxb"_atm);
	this->shapeMeshResources.SetSize(CoreGraphics::RenderShape::NumShapeTypes);

	// create default shapes (basically load them from the models)
	this->CreateBoxShape();
	this->CreateCylinderShape();
	this->CreateSphereShape();
	this->CreateConeShape();

	// lookup ModelViewProjection shader variable
	this->model = ShaderGetConstantBinding(this->shapeShader, "ShapeModel");
	this->diffuseColor = ShaderGetConstantBinding(this->shapeShader, "MatDiffuse");

	this->programs[RenderShape::AlwaysOnTop] = ShaderGetProgram(this->shapeShader, ShaderServer::Instance()->FeatureStringToMask("Colored"));
	this->programs[RenderShape::CheckDepth] = ShaderGetProgram(this->shapeShader, ShaderServer::Instance()->FeatureStringToMask("Colored|Alt0"));
	this->programs[RenderShape::Wireframe] = ShaderGetProgram(this->shapeShader, ShaderServer::Instance()->FeatureStringToMask("Colored|Alt1"));
	this->programs[RenderShape::AlwaysOnTop + RenderShape::NumDepthFlags] = ShaderGetProgram(this->shapeShader, ShaderServer::Instance()->FeatureStringToMask("Static"));
	this->programs[RenderShape::CheckDepth + RenderShape::NumDepthFlags] = ShaderGetProgram(this->shapeShader, ShaderServer::Instance()->FeatureStringToMask("Static|Alt0"));
	this->programs[RenderShape::Wireframe + RenderShape::NumDepthFlags] = ShaderGetProgram(this->shapeShader, ShaderServer::Instance()->FeatureStringToMask("Static|Alt1"));

	this->comps.Append(VertexComponent(VertexComponent::Position, 0, VertexComponent::Float4, 0));
	this->comps.Append(VertexComponent(VertexComponent::Color, 0, VertexComponent::Float4, 0));

	// also create an extra vertex layout, in case we get a mesh which doesn't fit with our special layout
	this->vertexLayout = CreateVertexLayout(VertexLayoutCreateInfo{ comps });

	Frame::FramePlugin::AddCallback("Debug Shapes", [this](IndexT)
		{
			CoreGraphics::BeginBatch(Frame::FrameBatchType::System);
			this->DrawShapes();
			this->numIndicesThisFrame = 0;
			this->numVerticesThisFrame = 0;
			CoreGraphics::EndBatch();
		});
}

//------------------------------------------------------------------------------
/**
*/
void
VkShapeRenderer::Close()
{
	n_assert(this->IsOpen());

	// unload shape meshes
	DiscardResource(this->shapeMeshResources[RenderShape::Box]);
	DiscardResource(this->shapeMeshResources[RenderShape::Sphere]);
	DiscardResource(this->shapeMeshResources[RenderShape::Cone]);
	this->shapeMeshResources.Clear();

	// unload dynamic buffers
	// we don't assert here as the buffer is lazy allocated
	CoreGraphics::VertexBufferId activeVBId = this->vbos[this->vertexBufferActiveIndex];
	CoreGraphics::IndexBufferId activeIBId = this->ibos[this->indexBufferActiveIndex];
	if (activeVBId != CoreGraphics::VertexBufferId::Invalid())
	{
		VertexBufferUnmap(activeVBId);
	}
	if (activeIBId != CoreGraphics::IndexBufferId::Invalid())
	{
		IndexBufferUnmap(activeIBId);
	}

	for (IndexT i = 0; i < MaxVertexIndexBuffers; i++)
	{
		if (this->vbos[i] != VertexBufferId::Invalid())
			DestroyVertexBuffer(this->vbos[i]);
		if (this->ibos[i] != IndexBufferId::Invalid())
		DestroyIndexBuffer(this->ibos[i]);
	}
	
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

	if (this->numIndicesThisFrame > this->indexBufferCapacity)
		this->GrowIndexBuffer();
	if (this->numVerticesThisFrame > this->vertexBufferCapacity)
		this->GrowVertexBuffer();



	CoreGraphics::SetVertexLayout(this->vertexLayout);
	for (int depthType = 0; depthType < RenderShape::NumDepthFlags; depthType++)
	{
		if (this->primitives[depthType].Size() > 0)
		{
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

			// apply shader
			CoreGraphics::SetShaderProgram(this->programs[depthType]);

			// flush any buffered primitives
			this->DrawBufferedIndexedPrimitives();
			this->DrawBufferedPrimitives();
		}
	}

	// reset index and primitive counters
	this->numIndices = 0;
	this->numPrimitives = 0;

	for (int depthType = 0; depthType < RenderShape::NumDepthFlags; depthType++)
	{
		if (this->shapes[depthType].Size() > 0)
		{
			CoreGraphics::SetShaderProgram(this->programs[depthType + RenderShape::NumDepthFlags]);
			CoreGraphics::SetGraphicsPipeline();

			IndexT i;
			for (i = 0; i < this->shapes[depthType].Size(); i++)
			{
				const RenderShape& curShape = this->shapes[depthType][i];
				if (curShape.GetShapeType() == RenderShape::RenderMesh)
					this->DrawMesh(curShape.GetModelTransform(), curShape.GetMesh(), curShape.GetColor());
				else
					this->DrawSimpleShape(curShape.GetModelTransform(), curShape.GetShapeType(), curShape.GetColor());
			}
		}		
	}

	// clear shapes
	this->ClearShapes();
}

//------------------------------------------------------------------------------
/**
*/
void
VkShapeRenderer::DrawSimpleShape(const Math::mat4& modelTransform, CoreGraphics::RenderShape::Type shapeType, const Math::vec4& color)
{
	n_assert(this->shapeMeshResources[shapeType] != ResourceId::Invalid());
	n_assert(shapeType < RenderShape::NumShapeTypes);
	n_assert(CoreGraphics::IsInBeginFrame());

	// resolve model-view-projection matrix and update shader
	CoreGraphics::PushConstants(CoreGraphics::GraphicsPipeline, this->model, sizeof(modelTransform), (byte*)&modelTransform);
	CoreGraphics::PushConstants(CoreGraphics::GraphicsPipeline, this->diffuseColor, sizeof(color), (byte*)&color);
	const Util::Array<CoreGraphics::PrimitiveGroup>& groups = MeshGetPrimitiveGroups(this->shapeMeshResources[shapeType]);
	IndexT i;
	for (i = 0; i < groups.Size(); i++)
	{
		// set resources
		CoreGraphics::SetIndexBuffer(MeshGetIndexBuffer(this->shapeMeshResources[shapeType]), 0);
		CoreGraphics::SetStreamVertexBuffer(0, MeshGetVertexBuffer(this->shapeMeshResources[shapeType], 0), 0);
		CoreGraphics::SetPrimitiveGroup(groups[i]);

		// draw
		CoreGraphics::Draw();
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VkShapeRenderer::DrawMesh(const Math::mat4& modelTransform, const CoreGraphics::MeshId mesh, const Math::vec4& color)
{
	n_assert(mesh != MeshId::Invalid());
	n_assert(CoreGraphics::IsInBeginFrame());

	// resolve model-view-projection matrix and update shader
	TransformDevice* transDev = TransformDevice::Instance();

	// resolve model-view-projection matrix and update shader
	CoreGraphics::PushConstants(CoreGraphics::GraphicsPipeline, this->model, sizeof(modelTransform), (byte*)&modelTransform);
	CoreGraphics::PushConstants(CoreGraphics::GraphicsPipeline, this->diffuseColor, sizeof(color), (byte*)&color);
	
	const Util::Array<CoreGraphics::PrimitiveGroup>& groups = MeshGetPrimitiveGroups(mesh);
	IndexT i;
	for (i = 0; i < groups.Size(); i++)
	{
		// set resources
		CoreGraphics::SetIndexBuffer(MeshGetIndexBuffer(mesh), 0);
		CoreGraphics::SetStreamVertexBuffer(0, MeshGetVertexBuffer(mesh, 0), 0);
		CoreGraphics::SetPrimitiveGroup(groups[i]);

		// draw
		CoreGraphics::Draw();
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VkShapeRenderer::DrawPrimitives(const Math::mat4& modelTransform, CoreGraphics::PrimitiveTopology::Code topology, SizeT numPrimitives, const void* vertices, SizeT vertexWidth, const Math::vec4& color)
{
	n_assert(0 != vertices);
	n_assert(vertexWidth <= MaxVertexWidth);

	// calculate vertex count
	SizeT vertexCount = PrimitiveTopology::NumberOfVertices(topology, numPrimitives);
	CoreGraphics::RenderShape::RenderShapeVertex* verts = reinterpret_cast<CoreGraphics::RenderShape::RenderShapeVertex*>(this->vertexBufferPtr);

	// unlock buffer to avoid stomping data
	memcpy(verts + this->numPrimitives, vertices, vertexCount * vertexWidth);

	// set vertex offset in primitive group
	CoreGraphics::PrimitiveGroup group;
	group.SetBaseVertex(this->numPrimitives);
	group.SetNumVertices(vertexCount);
	group.SetNumIndices(0);
	this->unindexed[topology].primitives.Append(group);
	this->unindexed[topology].transforms.Append(modelTransform);
	this->unindexed[topology].colors.Append(color);
	this->numPrimitives += vertexCount;
}

//------------------------------------------------------------------------------
/**
*/
void
VkShapeRenderer::DrawIndexedPrimitives(const Math::mat4& modelTransform, CoreGraphics::PrimitiveTopology::Code topology, SizeT numPrimitives, const void* vertices, SizeT numVertices, SizeT vertexWidth, const void* indices, CoreGraphics::IndexType::Code indexType, const Math::vec4& color)
{
	n_assert(0 != vertices);
	n_assert(0 != indices);
	n_assert(vertexWidth <= MaxVertexWidth);

	// calculate index count and size of index type
	SizeT indexCount = PrimitiveTopology::NumberOfVertices(topology, numPrimitives);
	SizeT indexSize = CoreGraphics::IndexType::SizeOf(indexType);

	SizeT vbBufferSize = MaxNumVertices * MaxVertexWidth;
	SizeT ibBufferSize = MaxNumIndices * MaxIndexWidth;
	CoreGraphics::RenderShape::RenderShapeVertex* verts = reinterpret_cast<CoreGraphics::RenderShape::RenderShapeVertex*>(this->vertexBufferPtr);
	uint32_t* inds = reinterpret_cast<uint32_t*>(this->indexBufferPtr);

	// unlock buffer and copy data
	memcpy(verts + this->numPrimitives, vertices, numVertices * vertexWidth);
	memcpy(inds + this->numIndices, indices, indexCount * indexSize);

	// set vertex offset in primitive group
	CoreGraphics::PrimitiveGroup group;
	group.SetBaseVertex(this->numPrimitives);
	group.SetNumVertices(0);										// indices decides how many primitives we draw
	group.SetBaseIndex(this->numIndices);
	group.SetNumIndices(indexCount);
	this->indexed[topology].primitives.Append(group);
	this->indexed[topology].transforms.Append(modelTransform);
	this->indexed[topology].colors.Append(color);
	this->numPrimitives += numVertices;
	this->numIndices += indexCount;
}

//------------------------------------------------------------------------------
/**
*/
void
VkShapeRenderer::DrawBufferedPrimitives()
{
	IndexT j;
	for (j = 0; j < CoreGraphics::PrimitiveTopology::NumTopologies; j++)
	{
		CoreGraphics::SetPrimitiveTopology(CoreGraphics::PrimitiveTopology::Code(j));
		CoreGraphics::SetGraphicsPipeline();

		IndexT i;
		for (i = 0; i < this->unindexed[j].primitives.Size(); i++)
		{
			const CoreGraphics::PrimitiveGroup& group = this->unindexed[j].primitives[i];
			const Math::mat4& modelTransform = this->unindexed[j].transforms[i];
			const Math::vec4& color = this->unindexed[j].colors[i];

			CoreGraphics::PushConstants(CoreGraphics::GraphicsPipeline, this->model, sizeof(modelTransform), (byte*)&modelTransform);
			CoreGraphics::PushConstants(CoreGraphics::GraphicsPipeline, this->diffuseColor, sizeof(color), (byte*)&color);

			CoreGraphics::SetStreamVertexBuffer(0, this->vbos[this->vertexBufferActiveIndex], 0);
			CoreGraphics::SetPrimitiveGroup(group);

			CoreGraphics::Draw();
		}
		this->unindexed[j].primitives.Clear();
		this->unindexed[j].transforms.Clear();
		this->unindexed[j].colors.Clear();
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VkShapeRenderer::DrawBufferedIndexedPrimitives()
{
	IndexT j;
	for (j = 0; j < CoreGraphics::PrimitiveTopology::NumTopologies; j++)
	{
		CoreGraphics::SetPrimitiveTopology(CoreGraphics::PrimitiveTopology::Code(j));
		CoreGraphics::SetGraphicsPipeline();

		IndexT i;
		for (i = 0; i < this->indexed[j].primitives.Size(); i++)
		{
			const CoreGraphics::PrimitiveGroup& group = this->indexed[j].primitives[i];
			const Math::mat4& modelTransform = this->indexed[j].transforms[i];
			const Math::vec4& color = this->indexed[j].colors[i];

			CoreGraphics::PushConstants(CoreGraphics::GraphicsPipeline, this->model, sizeof(modelTransform), (byte*)&modelTransform);
			CoreGraphics::PushConstants(CoreGraphics::GraphicsPipeline, this->diffuseColor, sizeof(color), (byte*)&color);

			CoreGraphics::SetIndexBuffer(this->ibos[this->indexBufferActiveIndex], 0);
			CoreGraphics::SetStreamVertexBuffer(0, this->vbos[this->vertexBufferActiveIndex], 0);
			CoreGraphics::SetPrimitiveGroup(group);

			CoreGraphics::Draw();
		}
		this->indexed[j].primitives.Clear();
		this->indexed[j].transforms.Clear();
		this->indexed[j].colors.Clear();
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
VkShapeRenderer::GrowIndexBuffer()
{
	IndexBufferCreateInfo iboInfo =
	{
		"shape_renderer_ibo",
		"render_system"_atm,
		CoreGraphics::GpuBufferTypes::AccessWrite,
		CoreGraphics::GpuBufferTypes::UsageDynamic,
		CoreGraphics::GpuBufferTypes::SyncingAutomatic,
		IndexType::Index32,
		this->numIndicesThisFrame,
		nullptr,
		0
	};

	// unmap current pointer
	if (this->indexBufferPtr)
		IndexBufferUnmap(this->ibos[this->indexBufferActiveIndex]);

	// delete the next buffer if one exists
	this->indexBufferActiveIndex = (this->indexBufferActiveIndex + 1) % MaxVertexIndexBuffers;
	if (this->ibos[this->indexBufferActiveIndex] != IndexBufferId::Invalid())
		DestroyIndexBuffer(this->ibos[this->indexBufferActiveIndex]);

	// finally allocate new buffer
	this->ibos[this->indexBufferActiveIndex] = CreateIndexBuffer(iboInfo);
	this->indexBufferPtr = (byte*)IndexBufferMap(this->ibos[this->indexBufferActiveIndex], CoreGraphics::GpuBufferTypes::MapWrite);
	n_assert(0 != this->indexBufferPtr);
}

//------------------------------------------------------------------------------
/**
*/
void 
VkShapeRenderer::GrowVertexBuffer()
{
	VertexBufferCreateInfo vboInfo =
	{
		"ShapeRenderer VBO"_atm,
		CoreGraphics::GpuBufferTypes::AccessWrite,
		CoreGraphics::GpuBufferTypes::UsageDynamic,
		CoreGraphics::GpuBufferTypes::SyncingAutomatic,
		this->numVerticesThisFrame,
		this->comps,
		nullptr,
		0
	};

	// unmap current pointer
	if (this->vertexBufferPtr)
		VertexBufferUnmap(this->vbos[this->vertexBufferActiveIndex]);

	// delete the next buffer if one exists
	this->vertexBufferActiveIndex = (this->vertexBufferActiveIndex + 1) % MaxVertexIndexBuffers;
	if (this->vbos[this->vertexBufferActiveIndex] != VertexBufferId::Invalid())
		DestroyVertexBuffer(this->vbos[this->vertexBufferActiveIndex]);

	// finally allocate new buffer
	this->vbos[this->vertexBufferActiveIndex] = CreateVertexBuffer(vboInfo);
	this->vertexBufferPtr = (byte*)VertexBufferMap(this->vbos[this->vertexBufferActiveIndex], CoreGraphics::GpuBufferTypes::MapWrite);
	n_assert(0 != this->vertexBufferPtr);
}

//------------------------------------------------------------------------------
/**
*/
void
VkShapeRenderer::CreateBoxShape()
{
	this->shapeMeshResources[RenderShape::Box] = CreateResource("msh:system/box.nvx2", "render_system", nullptr, nullptr, true);
}

//------------------------------------------------------------------------------
/**
*/
void
VkShapeRenderer::CreateSphereShape()
{
	this->shapeMeshResources[RenderShape::Sphere] = CreateResource("msh:system/sphere.nvx2", "render_system", nullptr, nullptr, true);
}

//------------------------------------------------------------------------------
/**
*/
void
VkShapeRenderer::CreateCylinderShape()
{
	this->shapeMeshResources[RenderShape::Cylinder] = CreateResource("msh:system/cylinder.nvx2", "render_system", nullptr, nullptr, true);
}

//------------------------------------------------------------------------------
/**
*/
void
VkShapeRenderer::CreateTorusShape()
{
	this->shapeMeshResources[RenderShape::Torus] = CreateResource("msh:system/torus.nvx2", "render_system", nullptr, nullptr, true);
}

//------------------------------------------------------------------------------
/**
*/
void
VkShapeRenderer::CreateConeShape()
{
	this->shapeMeshResources[RenderShape::Cone] = CreateResource("msh:system/cone.nvx2", "render_system", nullptr, nullptr, true);
}

} // namespace Vulkan