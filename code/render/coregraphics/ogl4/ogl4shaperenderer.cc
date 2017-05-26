//------------------------------------------------------------------------------
//  ogl4shaprenderer.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/ogl4/ogl4shaperenderer.h"
#include "coregraphics/ogl4/ogl4types.h"
#include "coregraphics/renderdevice.h"
#include "coregraphics/transformdevice.h"
#include "coregraphics/renderdevice.h"
#include "coregraphics/vertexbuffer.h"
#include "coregraphics/indexbuffer.h"
#include "coregraphics/streammeshloader.h"
#include "coregraphics/mesh.h"
#include "coregraphics/vertexlayoutserver.h"
#include "coregraphics/vertexcomponent.h"
#include "threading/thread.h"
#include "coregraphics/shaderserver.h"
#include "resources/resourceid.h"
#include "resources/resourcemanager.h"
#include "models/modelinstance.h"
#include "coregraphics/shadersemantics.h"
#include "coregraphics/memoryvertexbufferloader.h"
#include "coregraphics/memoryindexbufferloader.h"

namespace OpenGL4
{
__ImplementClass(OpenGL4::OGL4ShapeRenderer, 'O4SR', Base::ShapeRendererBase);

using namespace Base;
using namespace Threading;
using namespace Math;
using namespace CoreGraphics;
using namespace Threading;
using namespace Resources;
using namespace Models;

//------------------------------------------------------------------------------
/**
*/
OGL4ShapeRenderer::OGL4ShapeRenderer() :
	vertexBufferPtr(0),
    numPrimitives(0),
    numIndices(0),
	indexBufferPtr(0)
{
	this->shapeMeshes.Clear();
}

//------------------------------------------------------------------------------
/**
*/
OGL4ShapeRenderer::~OGL4ShapeRenderer()
{
    n_assert(!this->IsOpen());
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ShapeRenderer::Open()
{
    n_assert(!this->IsOpen());
    n_assert(!this->shapeShader.isvalid());

    // call parent class
    ShapeRendererBase::Open();

    // create shape shader instance
    this->shapeShader = ShaderServer::Instance()->GetShader("shd:simple");
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
	Ptr<MemoryVertexBufferLoader> vboLoader = MemoryVertexBufferLoader::Create();
	vboLoader->Setup(comps, MaxNumVertices * 3, NULL, 0, VertexBuffer::UsageDynamic, VertexBuffer::AccessWrite, VertexBuffer::SyncingCoherentPersistent);

	// create vbo
	this->vbo = VertexBuffer::Create();
	this->vbo->SetLoader(vboLoader.upcast<ResourceLoader>());
	this->vbo->SetAsyncEnabled(false);
	this->vbo->Load();
	n_assert(this->vbo->IsLoaded());
	this->vbo->SetLoader(NULL);

	// setup ibo
	Ptr<MemoryIndexBufferLoader> iboLoader = MemoryIndexBufferLoader::Create();
    iboLoader->Setup(IndexType::Index32, MaxNumIndices * 3, NULL, 0, IndexBuffer::UsageDynamic, IndexBuffer::AccessWrite, IndexBuffer::SyncingCoherentPersistent);

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
	this->primGroup.SetPrimitiveTopology(PrimitiveTopology::TriangleList);

    // set buffer index
    this->vbBufferIndex = 0;
	this->ibBufferIndex = 0;

	//glEnable(GL_LINE_SMOOTH);
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ShapeRenderer::Close()
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
OGL4ShapeRenderer::DrawShapes()
{
    n_assert(this->IsOpen());

	OGL4RenderDevice* renderDevice = OGL4RenderDevice::Instance();

	glLineWidth(1.5f);
	glPointSize(1.5f);

	glDisable(GL_LINE_SMOOTH);
	renderDevice->SetPassShader(this->shapeShader);
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
	}

	for (int depthType = 0; depthType < RenderShape::NumDepthFlags; depthType++)
    {
		this->shapeShader->SelectActiveVariation(featureBits[depthType + RenderShape::NumDepthFlags]);
        this->shapeShader->Apply();

		IndexT i;
		for (i = 0; i < this->shapes[depthType].Size(); i++)
		{
			const RenderShape& curShape = this->shapes[depthType][i];
			if (curShape.GetShapeType() == RenderShape::RenderMesh) this->DrawMesh(curShape.GetModelTransform(), curShape.GetMesh(), curShape.GetPrimitiveGroupIndex(), curShape.GetColor());
			else													this->DrawSimpleShape(curShape.GetModelTransform(), curShape.GetShapeType(), curShape.GetColor());
		}
	}
	renderDevice->SetPassShader(0);
	glEnable(GL_LINE_SMOOTH);    

	glLineWidth(1.0f);
	glPointSize(1.0f);

	// delete the shapes of my own thread id, all other shapes
	// are from other threads and will be deleted through DeleteShapesByThreadId()
	this->DeleteShapesByThreadId(Thread::GetMyThreadId());
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ShapeRenderer::DrawSimpleShape(const matrix44& modelTransform, RenderShape::Type shapeType, const float4& color)
{
    n_assert(this->shapeMeshes[shapeType].isvalid());
    n_assert(shapeType < RenderShape::NumShapeTypes);

	Ptr<RenderDevice> renderDevice = RenderDevice::Instance();

    // resolve model-view-projection matrix and update shader
    this->shapeShader->BeginUpdate();
	this->model->SetMatrix(modelTransform);
    this->diffuseColor->SetFloat4(color);
    this->shapeShader->EndUpdate();
    this->shapeShader->Commit();

	Ptr<Mesh> mesh = this->shapeMeshes[shapeType]->GetMesh();	
	Ptr<VertexBuffer> vb = mesh->GetVertexBuffer();
	Ptr<IndexBuffer> ib = mesh->GetIndexBuffer();
	PrimitiveGroup group = mesh->GetPrimitiveGroupAtIndex(0);

	// setup render device
	renderDevice->SetStreamVertexBuffer(0, vb, 0);
	renderDevice->SetVertexLayout(vb->GetVertexLayout());
	renderDevice->SetIndexBuffer(ib);
	renderDevice->SetPrimitiveGroup(group);

	// draw
	renderDevice->Draw();
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ShapeRenderer::DrawMesh(const Math::matrix44& modelTransform, const Ptr<CoreGraphics::Mesh>& mesh, const IndexT& groupIndex, const Math::float4& color)
{
	n_assert(mesh.isvalid());
	Ptr<RenderDevice> renderDevice = RenderDevice::Instance();

	// resolve model-view-projection matrix and update shader
	TransformDevice* transDev = TransformDevice::Instance();
	this->shapeShader->BeginUpdate();
	this->model->SetMatrix(modelTransform);
	this->diffuseColor->SetFloat4(color);
	this->shapeShader->EndUpdate();
	this->shapeShader->Commit();

	// draw shape
	n_assert(RenderDevice::Instance()->IsInBeginFrame());

	Ptr<CoreGraphics::VertexBuffer> vb = mesh->GetVertexBuffer();
	Ptr<CoreGraphics::IndexBuffer> ib = mesh->GetIndexBuffer();
	PrimitiveGroup group = mesh->GetPrimitiveGroupAtIndex(groupIndex);

	// setup render device
	renderDevice->SetStreamVertexBuffer(0, vb, 0);
	renderDevice->SetVertexLayout(vb->GetVertexLayout());
	renderDevice->SetIndexBuffer(ib);
	renderDevice->SetPrimitiveGroup(group);
	renderDevice->Draw();
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ShapeRenderer::DrawPrimitives(const matrix44& modelTransform, 
                                  PrimitiveTopology::Code topology,
                                  SizeT numPrimitives,
                                  const void* vertices,
                                  SizeT vertexWidth,
                                  const Math::float4& color)
{
    n_assert(0 != vertices);
	n_assert(vertexWidth <= MaxVertexWidth);

	// get device
	Ptr<OGL4RenderDevice> renderDevice = OGL4RenderDevice::Instance();

	// calculate vertex count
    SizeT vertexCount = PrimitiveTopology::NumberOfVertices(topology, numPrimitives);
	vertexCount = Math::n_min(vertexCount, MaxNumVertices);

    // flush primitives to make room in our buffers
    if (this->numPrimitives + vertexCount > MaxNumVertices)
    {
        this->DrawBufferedPrimitives();
        this->DrawBufferedIndexedPrimitives();

		// lock vbo buffer for this ring
		this->vboLock->LockBuffer(this->vbBufferIndex);
		this->vbBufferIndex = (this->vbBufferIndex + 1) % 3;
    }

    SizeT bufferSize = MaxNumVertices;
    SizeT bufferOffset = bufferSize * this->vbBufferIndex;
    CoreGraphics::RenderShape::RenderShapeVertex* verts = (CoreGraphics::RenderShape::RenderShapeVertex*)this->vertexBufferPtr;

    // unlock buffer to avoid stomping data
	//this->vboLock->WaitForRange(bufferOffset + this->numPrimitives, vertexCount * vertexWidth);
	this->vboLock->WaitForBuffer(this->vbBufferIndex);
    memcpy(verts + bufferOffset + this->numPrimitives, vertices, vertexCount * vertexWidth);

    // append transforms
    this->unindexed.transforms.Append(modelTransform);
    this->unindexed.colors.Append(color);

	// set vertex offset in primitive group
    CoreGraphics::PrimitiveGroup group;
    group.SetBaseVertex(MaxNumVertices * this->vbBufferIndex + this->numPrimitives);
	group.SetNumVertices(vertexCount);
	group.SetNumIndices(0);
	group.SetPrimitiveTopology(topology);
    this->unindexed.primitives.Append(group);
    this->numPrimitives += vertexCount;

    // place a lock and increment buffer count
    //this->vboLock->LockRange(bufferOffset + this->numPrimitives, vertexCount * vertexWidth);
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ShapeRenderer::DrawIndexedPrimitives(const matrix44& modelTransform,
                                         PrimitiveTopology::Code topology,
                                         SizeT numPrimitives,
                                         const void* vertices,
                                         SizeT numVertices,
                                         SizeT vertexWidth,
                                         const void* indices,
                                         IndexType::Code indexType,
                                         const float4& color)
{
    n_assert(0 != vertices);
    n_assert(0 != indices);
	n_assert(vertexWidth <= MaxVertexWidth);

	// get device
	Ptr<OGL4RenderDevice> renderDevice = OGL4RenderDevice::Instance();

	// calculate index count and size of index type
    SizeT indexCount = PrimitiveTopology::NumberOfVertices(topology, numPrimitives);
	SizeT indexSize = CoreGraphics::IndexType::SizeOf(indexType);
	SizeT vertexCount = Math::n_min(numVertices, MaxNumVertices);
	indexCount = Math::n_min(indexCount, MaxNumIndices);

    // flush indexed primitives
    if (this->numIndices + indexCount > MaxNumIndices || this->numPrimitives + vertexCount > MaxNumVertices)
    {
        this->DrawBufferedPrimitives();
        this->DrawBufferedIndexedPrimitives();

		// lock vbo buffer for this ring
		this->vboLock->LockBuffer(this->vbBufferIndex);
		this->vbBufferIndex = (this->vbBufferIndex + 1) % 3;

		// lock ibo buffer for this ring
		this->iboLock->LockBuffer(this->ibBufferIndex);
		this->ibBufferIndex = (this->ibBufferIndex + 1) % 3;
    }

    SizeT vbBufferSize = MaxNumVertices * MaxVertexWidth;
    SizeT vbBufferOffset = vbBufferSize * this->vbBufferIndex;
    SizeT ibBufferSize = MaxNumIndices * MaxIndexWidth;
    SizeT ibBufferOffset = ibBufferSize * this->ibBufferIndex;
	CoreGraphics::RenderShape::RenderShapeVertex* verts = (CoreGraphics::RenderShape::RenderShapeVertex*)this->vertexBufferPtr;

    // unlock buffer and copy data
	this->vboLock->WaitForBuffer(this->vbBufferIndex);
	this->iboLock->WaitForBuffer(this->ibBufferIndex);
    //this->vboLock->WaitForRange(vbBufferSize + this->numPrimitives, vertexCount * vertexWidth);
    //this->iboLock->WaitForRange(ibBufferOffset + this->numIndices, indexCount * indexSize);
	memcpy(verts + vbBufferSize + this->numPrimitives, vertices, vertexCount * vertexWidth);
    memcpy(this->indexBufferPtr + ibBufferOffset + this->numIndices, indices, indexCount * indexSize);

    // append transforms
    this->indexed.transforms.Append(modelTransform);
    this->indexed.colors.Append(color);

	// set vertex offset in primitive group
    CoreGraphics::PrimitiveGroup group;
    group.SetBaseVertex(MaxNumVertices * this->vbBufferIndex + this->numPrimitives);
	group.SetNumVertices(0);										// indices decides how many primitives we draw
    group.SetBaseIndex(MaxNumIndices * this->ibBufferIndex + this->numIndices);
	group.SetNumIndices(indexCount);
	group.SetPrimitiveTopology(topology);
    this->indexed.primitives.Append(group);
    this->numPrimitives += vertexCount;
    this->numIndices += indexCount;

    // lock buffer and increment buffer count
    //this->vboLock->LockRange(vbBufferOffset + this->numPrimitives, numVertices * vertexWidth);
    //this->iboLock->LockRange(ibBufferOffset + this->numIndices, indexCount * indexSize);
}

//------------------------------------------------------------------------------
/**
*/
void 
OGL4ShapeRenderer::CreateBoxShape()
{
	Ptr<ManagedMesh> mesh = ResourceManager::Instance()->CreateManagedResource(Mesh::RTTI, "msh:system/box.nvx2", StreamMeshLoader::Create()).downcast<ManagedMesh>();
	this->shapeMeshes[RenderShape::Box] = mesh;
}

//------------------------------------------------------------------------------
/**
*/
void 
OGL4ShapeRenderer::CreateSphereShape()
{
	Ptr<ManagedMesh> mesh = ResourceManager::Instance()->CreateManagedResource(Mesh::RTTI, "msh:system/sphere.nvx2", StreamMeshLoader::Create()).downcast<ManagedMesh>();
	this->shapeMeshes[RenderShape::Sphere] = mesh;
}

//------------------------------------------------------------------------------
/**
*/
void 
OGL4ShapeRenderer::CreateCylinderShape()
{
	Ptr<ManagedMesh> mesh = ResourceManager::Instance()->CreateManagedResource(Mesh::RTTI, "msh:system/cylinder.nvx2", StreamMeshLoader::Create()).downcast<ManagedMesh>();
	this->shapeMeshes[RenderShape::Cylinder] = mesh;
}

//------------------------------------------------------------------------------
/**
*/
void 
OGL4ShapeRenderer::CreateTorusShape()
{
	Ptr<ManagedMesh> mesh = ResourceManager::Instance()->CreateManagedResource(Mesh::RTTI, "msh:system/torus.nvx2", StreamMeshLoader::Create()).downcast<ManagedMesh>();
	this->shapeMeshes[RenderShape::Torus] = mesh;
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ShapeRenderer::CreateConeShape()
{
	Ptr<ManagedMesh> mesh = ResourceManager::Instance()->CreateManagedResource(Mesh::RTTI, "msh:system/cone.nvx2", StreamMeshLoader::Create()).downcast<ManagedMesh>();
	this->shapeMeshes[RenderShape::Cone] = mesh;
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ShapeRenderer::DrawBufferedPrimitives()
{
    Ptr<RenderDevice> renderDevice = RenderDevice::Instance();
	
    renderDevice->SetVertexLayout(this->vertexLayout);
    renderDevice->SetStreamSource(0, this->vbo, 0);

    IndexT i;
    for (i = 0; i < this->unindexed.primitives.Size(); i++)
    {
        const CoreGraphics::PrimitiveGroup& group = this->unindexed.primitives[i];
        const Math::matrix44& modelTransform = this->unindexed.transforms[i];
        const Math::float4& color = this->unindexed.colors[i];

        this->shapeShader->BeginUpdate();
        this->model->SetMatrix(modelTransform);
        this->shapeShader->EndUpdate();
        this->shapeShader->Commit();

        renderDevice->SetPrimitiveGroup(group);
        renderDevice->Draw();
    }

    this->numPrimitives = 0;
    this->unindexed.primitives.Clear();

    //this->vbBufferIndex = (this->vbBufferIndex + 1) % 3;
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ShapeRenderer::DrawBufferedIndexedPrimitives()
{
    Ptr<RenderDevice> renderDevice = RenderDevice::Instance();
    renderDevice->SetVertexLayout(this->vertexLayout);
	renderDevice->SetIndexBuffer(this->ibo);
    renderDevice->SetStreamSource(0, this->vbo, 0);

    IndexT i;
    for (i = 0; i < this->indexed.primitives.Size(); i++)
    {
        const CoreGraphics::PrimitiveGroup& group = this->indexed.primitives[i];
        const Math::matrix44& modelTransform = this->indexed.transforms[i];
        const Math::float4& color = this->indexed.colors[i];

        this->shapeShader->BeginUpdate();
        this->model->SetMatrix(modelTransform);
        this->shapeShader->EndUpdate();
        this->shapeShader->Commit();

        renderDevice->SetPrimitiveGroup(group);
        renderDevice->Draw();
    }

    this->numIndices = 0;
    this->indexed.primitives.Clear();

    //this->vbBufferIndex = (this->vbBufferIndex + 1) % 3;
    //this->ibBufferIndex = (this->ibBufferIndex + 1) % 3;
}

} // namespace OpenGL4
