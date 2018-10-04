//------------------------------------------------------------------------------
//  d3d11shaprenderer.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "stdneb.h"
#include "coregraphics/d3d11/d3d11shaperenderer.h"
#include "coregraphics/d3d11/d3d11types.h"
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

namespace Direct3D11
{
__ImplementClass(Direct3D11::D3D11ShapeRenderer, 'D1SR', Base::ShapeRendererBase);

using namespace Threading;
using namespace Math;
using namespace CoreGraphics;
using namespace Threading;
using namespace Resources;
using namespace Models;

//------------------------------------------------------------------------------
/**
*/
D3D11ShapeRenderer::D3D11ShapeRenderer() :
	vertexBuffer(0),
	indexBuffer(0)
{
	this->shapeMeshes.Clear();
}

//------------------------------------------------------------------------------
/**
*/
D3D11ShapeRenderer::~D3D11ShapeRenderer()
{
    n_assert(!this->IsOpen());
}

//------------------------------------------------------------------------------
/**
*/
void
D3D11ShapeRenderer::Open()
{
    n_assert(!this->IsOpen());
    n_assert(!this->shapeShader.isvalid());

    // call parent class
    ShapeRendererBase::Open();

    // create shape shader instance
	
    this->shapeShader = ShaderServer::Instance()->CreateShaderInstance(ResourceId("shd:simple"));

	this->shapeMeshes.SetSize(CoreGraphics::RenderShape::NumShapeTypes);

	Util::Array<VertexComponent> vertexComponents;
	vertexComponents.Append(VertexComponent(VertexComponent::Position, 0, VertexComponent::Float3, 0));
	this->vertexLayout = VertexLayoutServer::Instance()->CreateSharedVertexLayout(vertexComponents);

	this->CreateBoxShape();
	this->CreateCylinderShape();
	this->CreateSphereShape();
	this->CreateTorusShape();
	this->CreateConeShape();

    // lookup ModelViewProjection shader variable
	this->model = this->shapeShader->GetVariableBySemantic(ShaderVariable::Name(NEBULA_SEMANTIC_MODEL));
	this->viewProjection = this->shapeShader->GetVariableBySemantic(ShaderVariable::Name(NEBULA_SEMANTIC_VIEWPROJECTION));
	this->diffuseColor  = this->shapeShader->GetVariableBySemantic(ShaderVariable::Name("MatDiffuse"));

	this->depthFeatureBits[RenderShape::AlwaysOnTop] = ShaderServer::Instance()->FeatureStringToMask("Static");    
	this->depthFeatureBits[RenderShape::CheckDepth] = ShaderServer::Instance()->FeatureStringToMask("Static|Depth");
	this->depthFeatureBits[RenderShape::Wireframe] = ShaderServer::Instance()->FeatureStringToMask("Static|Wireframe");

	HRESULT hr;
	int vertexWidth = 40;
	D3D11_BUFFER_DESC desc;
	desc.ByteWidth = vertexWidth * sizeof(float) * MaxNumVertices;
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.MiscFlags = 0;
	desc.StructureByteStride = 0;

	hr = RenderDevice::Instance()->GetDirect3DDevice()->CreateBuffer(&desc, NULL, &this->vertexBuffer);
	n_assert(SUCCEEDED(hr));

	desc.ByteWidth = sizeof(int) * MaxNumVertices;
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.MiscFlags = 0;
	desc.StructureByteStride = 0;

	hr = RenderDevice::Instance()->GetDirect3DDevice()->CreateBuffer(&desc, NULL, &this->indexBuffer);
	n_assert(SUCCEEDED(hr));
}

//------------------------------------------------------------------------------
/**
*/
void
D3D11ShapeRenderer::Close()
{
    n_assert(this->IsOpen());
    n_assert(this->shapeShader.isvalid());

	IndexT i;
	for (i = 0; i < 5; i++)
	{
		// remove from resource manager
		ResourceManager::Instance()->DiscardManagedResource(shapeMeshes[i].upcast<Resources::ManagedResource>());
	}
	this->shapeMeshes.Clear();

    this->diffuseColor = 0;
    this->model = 0;	

    // discard shape shader
    this->shapeShader->Discard();
    this->shapeShader = 0;

	this->vertexLayout = 0;
	this->model = 0;
	this->diffuseColor = 0;

    // call parent class
    ShapeRendererBase::Close();
}

//------------------------------------------------------------------------------
/**
*/
void
D3D11ShapeRenderer::DrawShapes()
{
    n_assert(this->IsOpen());

	RenderDevice* renderDevice = RenderDevice::Instance();
	ShaderServer* shaderServer = ShaderServer::Instance();

	shaderServer->SetActiveShaderInstance(this->shapeShader);
	renderDevice->SetVertexLayout(this->vertexLayout);
	renderDevice->SetRenderTargets();
	renderDevice->SetViewports();

	// render shapes
    for (int depthType = 0; depthType<RenderShape::NumDepthFlags; depthType++)
    {
	    if (this->shapes[depthType].Size() > 0)
	    {	
			this->shapeShader->SelectActiveVariation(depthFeatureBits[depthType]);	
	        this->shapeShader->Begin();
	        this->shapeShader->BeginPass(0);	
	
	        // render individual shapes
	        IndexT i;
	        for (i = 0; i < this->shapes[depthType].Size(); i++)
	        {
	            const RenderShape& curShape = this->shapes[depthType][i];
	            n_assert(InvalidThreadId != curShape.GetThreadId());
	            switch (curShape.GetShapeType())
	            {
	                case RenderShape::Primitives:
	                    this->DrawPrimitives(curShape.GetModelTransform(),
	                                         curShape.GetTopology(),
	                                         curShape.GetNumPrimitives(),
	                                         curShape.GetVertexData(),
	                                         curShape.GetVertexWidth(),
	                                         curShape.GetColor());
	                    break;
	
	                case RenderShape::IndexedPrimitives:
	                    this->DrawIndexedPrimitives(curShape.GetModelTransform(),
	                                                curShape.GetTopology(),
	                                                curShape.GetNumPrimitives(),
	                                                curShape.GetVertexData(),
	                                                curShape.GetNumVertices(),
	                                                curShape.GetVertexWidth(),
	                                                curShape.GetIndexData(),
	                                                curShape.GetIndexType(),
	                                                curShape.GetColor());
	                    break;
					case RenderShape::RenderMesh:
						this->DrawMesh(curShape.GetModelTransform(),
									   curShape.GetMesh(),
									   curShape.GetColor());
						break;
	                default:
	                    this->DrawSimpleShape(curShape.GetModelTransform(), curShape.GetShapeType(), curShape.GetColor());
	                    break;
	            }
	        }

			this->shapeShader->EndPass();
			this->shapeShader->End();
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
D3D11ShapeRenderer::DrawSimpleShape(const matrix44& modelTransform, RenderShape::Type shapeType, const float4& color)
{
    n_assert(this->shapeMeshes[shapeType].isvalid());
    n_assert(shapeType < RenderShape::NumShapeTypes);

	Ptr<RenderDevice> renderDevice = RenderDevice::Instance();
	ID3D11DeviceContext* deviceContext = D3D11RenderDevice::Instance()->GetDirect3DDeviceContext();

    // set model transform
	this->viewProjection->SetMatrix(TransformDevice::Instance()->GetViewProjTransform());
    this->model->SetMatrix(modelTransform);
    this->diffuseColor->SetFloat4(color);
    this->shapeShader->Commit();

    // draw shape
    n_assert(RenderDevice::Instance()->IsInBeginFrame());
	
	Ptr<Mesh> mesh = this->shapeMeshes[shapeType]->GetMesh();
	renderDevice->SetPrimitiveGroup(mesh->GetPrimitiveGroupAtIndex(0));
	
	Ptr<CoreGraphics::VertexBuffer> vb = mesh->GetVertexBuffer();
	Ptr<CoreGraphics::IndexBuffer> ib = mesh->GetIndexBuffer();
	deviceContext->IASetIndexBuffer(ib->GetD3D11IndexBuffer(), D3D11Types::IndexTypeAsD3D11Format(ib->GetIndexType()), 0);
	ID3D11Buffer* vertexBuffer = vb->GetD3D11VertexBuffer();
	UINT vertexByteSize = vb->GetVertexLayout()->GetVertexByteSize();
	UINT offset = 0;
	deviceContext->IASetPrimitiveTopology(D3D11Types::AsD3D11PrimitiveType(mesh->GetPrimitiveGroupAtIndex(0).GetPrimitiveTopology()));
	deviceContext->IASetVertexBuffers(0, 1, &vertexBuffer, &vertexByteSize, &offset);
	deviceContext->IASetInputLayout(vb->GetVertexLayout()->GetD3D11VertexDeclaration());
	deviceContext->DrawIndexed(mesh->GetPrimitiveGroupAtIndex(0).GetNumIndices(), 0, 0);	
}

//------------------------------------------------------------------------------
/**
*/
void
D3D11ShapeRenderer::DrawPrimitives(const matrix44& modelTransform, 
                                  PrimitiveTopology::Code topology,
                                  SizeT numPrimitives,
                                  const void* vertices,
                                  SizeT vertexWidth,
                                  const Math::float4& color)
{
    n_assert(0 != vertices);

	// set model transform
	this->viewProjection->SetMatrix(TransformDevice::Instance()->GetViewProjTransform());
	this->model->SetMatrix(modelTransform);
    this->diffuseColor->SetFloat4(color);
    this->shapeShader->Commit();

    // draw primitives
    D3D11_PRIMITIVE_TOPOLOGY d3d11PrimType = D3D11Types::AsD3D11PrimitiveType(topology);

	ID3D11DeviceContext* context = RenderDevice::Instance()->GetDirect3DDeviceContext();

	// we need to create a temporary buffer seeing as we have no idea what primitive is being fed to the shape renderer...
	SizeT vertexCount = PrimitiveTopology::NumberOfVertices(topology, numPrimitives);

	int numVertices = min(MaxNumVertices, vertexCount);
	D3D11_MAPPED_SUBRESOURCE subres;
	HRESULT hr = RenderDevice::Instance()->GetDirect3DDeviceContext()->Map(this->vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &subres);
	n_assert(SUCCEEDED(hr));

	memcpy(subres.pData, vertices, numVertices * vertexWidth * sizeof(float));

	RenderDevice::Instance()->GetDirect3DDeviceContext()->Unmap(this->vertexBuffer, 0);

	context->IASetPrimitiveTopology(d3d11PrimType);
	UINT stride = vertexWidth * sizeof(float);
	UINT offset = 0;
	context->IASetVertexBuffers(0, 1, &this->vertexBuffer, &stride, &offset);	
	context->Draw(vertexCount, 0);

}

//------------------------------------------------------------------------------
/**
*/
void
D3D11ShapeRenderer::DrawIndexedPrimitives(const matrix44& modelTransform,
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

	// set model transform
	this->viewProjection->SetMatrix(TransformDevice::Instance()->GetViewProjTransform());
	this->model->SetMatrix(modelTransform);
    this->diffuseColor->SetFloat4(color);
    this->shapeShader->Commit();

    // draw indexed primitives
    D3D11_PRIMITIVE_TOPOLOGY d3d11PrimType = D3D11Types::AsD3D11PrimitiveType(topology);
    DXGI_FORMAT d3d11IndexType = (IndexType::Index16 == indexType) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
	ID3D11DeviceContext* context = RenderDevice::Instance()->GetDirect3DDeviceContext();

	SizeT indexCount = PrimitiveTopology::NumberOfVertices(topology, numPrimitives);

	numVertices = min(MaxNumVertices, numVertices);
	D3D11_MAPPED_SUBRESOURCE subres;
	HRESULT hr = RenderDevice::Instance()->GetDirect3DDeviceContext()->Map(this->vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &subres);
	n_assert(SUCCEEDED(hr));
	memcpy(subres.pData, vertices, numVertices * vertexWidth * sizeof(float));
	RenderDevice::Instance()->GetDirect3DDeviceContext()->Unmap(this->vertexBuffer, 0);

	int numIndices = min(MaxNumIndices, indexCount);
	hr = RenderDevice::Instance()->GetDirect3DDeviceContext()->Map(this->indexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &subres);
	n_assert(SUCCEEDED(hr));
	memcpy(subres.pData, indices, numIndices * CoreGraphics::IndexType::SizeOf(indexType));
	RenderDevice::Instance()->GetDirect3DDeviceContext()->Unmap(this->indexBuffer, 0);

	context->IASetPrimitiveTopology(d3d11PrimType);
	UINT stride = vertexWidth * sizeof(float);
	UINT offset = 0;
	context->IASetVertexBuffers(0, 1, &this->vertexBuffer, &stride, &offset);
	context->IASetIndexBuffer(this->indexBuffer, d3d11IndexType, 0);

	context->DrawIndexed(indexCount, 0, 0);
}

//------------------------------------------------------------------------------
/**
*/
void 
D3D11ShapeRenderer::DrawMesh( const Math::matrix44& modelTransform, const Ptr<CoreGraphics::Mesh>& mesh, const Math::float4& color )
{
	n_assert(mesh.isvalid());

	// set model transform
	this->viewProjection->SetMatrix(TransformDevice::Instance()->GetViewProjTransform());
	this->model->SetMatrix(modelTransform);
	this->diffuseColor->SetFloat4(color);
	this->shapeShader->Commit();

	const Ptr<D3D11RenderDevice>& device = D3D11RenderDevice::Instance();

	device->SetIndexBuffer(mesh->GetIndexBuffer());
	device->SetVertexLayout(this->vertexLayout);
	device->SetStreamSource(0, mesh->GetVertexBuffer(), 0);

	// get amount of primitive groups
	int numPrimGroups = mesh->GetNumPrimitiveGroups();
	IndexT prim;
	for (prim = 0; prim < numPrimGroups; prim++)
	{
		// get prim group
		const CoreGraphics::PrimitiveGroup& group = mesh->GetPrimitiveGroupAtIndex(prim);
		device->SetPrimitiveGroup(group);

		// finally draw primitive group
		device->Draw();
	}

}

//------------------------------------------------------------------------------
/**
*/
void 
D3D11ShapeRenderer::CreateBoxShape()
{
	Ptr<ManagedMesh> mesh = ResourceManager::Instance()->CreateManagedResource(Mesh::RTTI, "msh:system/box.nvx2", StreamMeshLoader::Create()).downcast<ManagedMesh>();
	this->shapeMeshes[RenderShape::Box] = mesh;
}

//------------------------------------------------------------------------------
/**
*/
void 
D3D11ShapeRenderer::CreateSphereShape()
{
	Ptr<ManagedMesh> mesh = ResourceManager::Instance()->CreateManagedResource(Mesh::RTTI, "msh:system/sphere.nvx2", StreamMeshLoader::Create()).downcast<ManagedMesh>();
	this->shapeMeshes[RenderShape::Sphere] = mesh;
}

//------------------------------------------------------------------------------
/**
*/
void 
D3D11ShapeRenderer::CreateCylinderShape()
{
	Ptr<ManagedMesh> mesh = ResourceManager::Instance()->CreateManagedResource(Mesh::RTTI, "msh:system/cylinder.nvx2", StreamMeshLoader::Create()).downcast<ManagedMesh>();
	this->shapeMeshes[RenderShape::Cylinder] = mesh;
}

//------------------------------------------------------------------------------
/**
*/
void 
D3D11ShapeRenderer::CreateTorusShape()
{
	Ptr<ManagedMesh> mesh = ResourceManager::Instance()->CreateManagedResource(Mesh::RTTI, "msh:system/torus.nvx2", StreamMeshLoader::Create()).downcast<ManagedMesh>();
	this->shapeMeshes[RenderShape::Torus] = mesh;
}

//------------------------------------------------------------------------------
/**
*/
void
D3D11ShapeRenderer::CreateConeShape()
{
	Ptr<ManagedMesh> mesh = ResourceManager::Instance()->CreateManagedResource(Mesh::RTTI, "msh:system/cone.nvx2", StreamMeshLoader::Create()).downcast<ManagedMesh>();
	this->shapeMeshes[RenderShape::Cone] = mesh;
}


} // namespace Direct3D11