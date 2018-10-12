//------------------------------------------------------------------------------
//  d3d11memoryvertexbufferloader.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/vertexlayoutserver.h"
#include "coregraphics/d3d11/d3d11memoryvertexbufferloader.h"
#include "coregraphics/d3d11/d3d11types.h"
#include "coregraphics/renderdevice.h"
#include "coregraphics/vertexbuffer.h"

namespace Direct3D11
{
__ImplementClass(Direct3D11::D3D11MemoryVertexBufferLoader, 'DMVL', Base::MemoryVertexBufferLoaderBase);

using namespace Resources;
using namespace CoreGraphics;

//------------------------------------------------------------------------------
/**
    This will create a D3D11 vertex buffer and vertex declaration object
    from the data provided in the Setup() method and setup our resource
    object (which must be a D3D11VertexBuffer object).
*/
bool
D3D11MemoryVertexBufferLoader::OnLoadRequested()
{
    n_assert(this->GetState() == Resource::Initial);
    n_assert(this->resource.isvalid());
    n_assert(!this->resource->IsAsyncEnabled());
    n_assert(this->numVertices > 0);
    if (VertexBuffer::UsageImmutable == this->usage)
    {
        n_assert(0 != this->vertexDataPtr);
        n_assert(0 < this->vertexDataSize);
    }

	ID3D11Buffer* d3dVertexBuffer = 0;
	D3D11_SUBRESOURCE_DATA data;
    ID3D11Device* d3d11Device = RenderDevice::Instance()->GetDirect3DDevice();
	
    n_assert(0 != d3d11Device);	

    // first setup the vertex layout (contains the D3D11 vertex declaration)
    Ptr<VertexLayout> vertexLayout = VertexLayoutServer::Instance()->CreateSharedVertexLayout(this->vertexComponents);
    if (0 != this->vertexDataPtr)
    {
        n_assert((this->numVertices * vertexLayout->GetVertexByteSize()) == this->vertexDataSize);
    }

	// create a D3D11 vertex buffer object
	D3D11_BUFFER_DESC desc;
	if (D3D11_USAGE_DYNAMIC == D3D11Types::AsD3D11Usage(this->usage))
	{
		desc = CD3D11_BUFFER_DESC(
			this->numVertices * vertexLayout->GetVertexByteSize(), 
			D3D11_BIND_VERTEX_BUFFER,
			D3D11_USAGE_DYNAMIC,
			D3D11_CPU_ACCESS_WRITE);
	}
	else
	{
		desc = CD3D11_BUFFER_DESC(
			this->numVertices * vertexLayout->GetVertexByteSize(), 
			D3D11_BIND_VERTEX_BUFFER,
			D3D11_USAGE_DEFAULT);
	}
	

	desc.StructureByteStride = vertexLayout->GetVertexByteSize();
	
	HRESULT hr;
	
	data.pSysMem = this->vertexDataPtr;
	data.SysMemPitch = 0;
	data.SysMemSlicePitch = 0;

	if (0 != this->vertexDataPtr)
	{
		hr = d3d11Device->CreateBuffer(&desc, &data, &d3dVertexBuffer);
	}
	else
	{
		hr = d3d11Device->CreateBuffer(&desc, NULL, &d3dVertexBuffer);
	}
	

	n_assert(SUCCEEDED(hr));
    n_assert(0 != d3dVertexBuffer);

    // setup our resource object
    const Ptr<VertexBuffer>& res = this->resource.downcast<VertexBuffer>();
    n_assert(!res->IsLoaded());
	res->SetUsage(this->usage);
	res->SetAccess(this->access);
    res->SetVertexLayout(vertexLayout);
    res->SetNumVertices(this->numVertices);
    res->SetD3D11VertexBuffer(d3dVertexBuffer);


    // invalidate setup data (because we don't own our data)
    this->vertexDataPtr = 0;
    this->vertexDataSize = 0;

    this->SetState(Resource::Loaded);
    return true;
}

} // namespace Direct3D11
