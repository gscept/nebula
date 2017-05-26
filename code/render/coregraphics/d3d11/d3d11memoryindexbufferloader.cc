//------------------------------------------------------------------------------
//  d3d11memoryindexbufferloader.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/d3d11/d3d11memoryindexbufferloader.h"
#include "coregraphics/indexbuffer.h"
#include "coregraphics/renderdevice.h"
#include "coregraphics/d3d11/d3d11types.h"

namespace Direct3D11
{
__ImplementClass(Direct3D11::D3D11MemoryIndexBufferLoader, 'DMIL', Base::MemoryIndexBufferLoaderBase);

using namespace Resources;
using namespace CoreGraphics;

//------------------------------------------------------------------------------
/**
    This will create a D3D11 IndexBuffer using the data provided by our
    Setup() method and set our resource object (which must be a
    D3D11IndexBuffer object).
*/
bool
D3D11MemoryIndexBufferLoader::OnLoadRequested()
{
    n_assert(this->GetState() == Resource::Initial);
    n_assert(this->resource.isvalid());
    n_assert(!this->resource->IsAsyncEnabled());
    n_assert(this->indexType != IndexType::None);
    n_assert(this->numIndices > 0);
    if (IndexBuffer::UsageImmutable == this->usage)
    {
        n_assert(this->indexDataSize == (this->numIndices * IndexType::SizeOf(this->indexType)));
        n_assert(0 != this->indexDataPtr);
        n_assert(0 < this->indexDataSize);
    }

    ID3D11Device* d3d11Device = RenderDevice::Instance()->GetDirect3DDevice();
    n_assert(0 != d3d11Device);


	// setup initial data if provided
	D3D11_SUBRESOURCE_DATA data;
	data.pSysMem = this->indexDataPtr;
	data.SysMemPitch = 0;
	data.SysMemSlicePitch = 0;

	CD3D11_BUFFER_DESC desc;
	if (D3D11_USAGE_DYNAMIC == D3D11Types::AsD3D11Usage(this->usage))
	{
		desc = CD3D11_BUFFER_DESC(
			this->numIndices * IndexType::SizeOf(this->indexType), 
			D3D11_BIND_INDEX_BUFFER,
			D3D11_USAGE_DYNAMIC,
			D3D11_CPU_ACCESS_WRITE);
	}
	else
	{
		 desc = CD3D11_BUFFER_DESC(
			this->numIndices * IndexType::SizeOf(this->indexType), 
			D3D11_BIND_INDEX_BUFFER,
			D3D11_USAGE_DEFAULT);
	}
	

    
    ID3D11Buffer* d3dIndexBuffer = 0;
	HRESULT hr;

	// create a D3D11 index buffer object
	if (0 != this->indexDataPtr)
	{
		hr = d3d11Device->CreateBuffer(&desc, &data, &d3dIndexBuffer); 
	}
	else
	{
		hr = d3d11Device->CreateBuffer(&desc, NULL, &d3dIndexBuffer); 
	}

    n_assert(SUCCEEDED(hr));
    n_assert(0 != d3dIndexBuffer);

    // setup our IndexBuffer resource
    const Ptr<IndexBuffer>& res = this->resource.downcast<IndexBuffer>();
    n_assert(!res->IsLoaded());
	res->SetUsage(this->usage);
	res->SetAccess(this->access);
    res->SetIndexType(this->indexType);
    res->SetNumIndices(this->numIndices);
    res->SetD3D11IndexBuffer(d3dIndexBuffer);

    // invalidate setup data (because we don't own our data)
    this->indexDataPtr = 0;
    this->indexDataSize = 0;

    this->SetState(Resource::Loaded);
    return true;
}

} // namespace Direct3D11
