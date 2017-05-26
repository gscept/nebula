//------------------------------------------------------------------------------
//  d3d11vertexbuffer.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "stdneb.h"
#include "coregraphics/d3d11/d3d11vertexbuffer.h"
#include "coregraphics/renderdevice.h"
#include "coregraphics/d3d11/d3d11types.h"

namespace Direct3D11
{
__ImplementClass(Direct3D11::D3D11VertexBuffer, 'D1VB', Base::VertexBufferBase);

using namespace CoreGraphics;

//------------------------------------------------------------------------------
/**
*/
D3D11VertexBuffer::D3D11VertexBuffer() :
    d3d11VertexBuffer(0),
	d3d11CPUVertexBuffer(0),
    mapCount(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
D3D11VertexBuffer::~D3D11VertexBuffer()
{
    n_assert(0 == this->d3d11VertexBuffer);
    n_assert(0 == this->mapCount);
}

//------------------------------------------------------------------------------
/**
*/
void
D3D11VertexBuffer::Unload()
{
    n_assert(0 == this->mapCount);
    if (0 != this->d3d11VertexBuffer)
    {
        this->d3d11VertexBuffer->Release();
        this->d3d11VertexBuffer = 0;
    }

	if (0 != this->d3d11CPUVertexBuffer)
	{
		this->d3d11CPUVertexBuffer->Release();
		this->d3d11CPUVertexBuffer = 0;
	}
    VertexBufferBase::Unload();
}

//------------------------------------------------------------------------------
/**
*/
void*
D3D11VertexBuffer::Map(MapType mapType)
{
    n_assert(0 != this->d3d11VertexBuffer);
    this->mapCount++;
	ID3D11DeviceContext* context = D3D11RenderDevice::Instance()->GetDirect3DDeviceContext();
	
	
	D3D11_MAP mapFlags;
	HRESULT hr;
	switch (mapType)
	{
	case MapRead:
		mapFlags = D3D11_MAP_READ;
		break;
	case MapWrite:
		mapFlags = D3D11_MAP_WRITE;
		break;
	case MapReadWrite:
		mapFlags = D3D11_MAP_READ_WRITE;
		break;
	case MapWriteDiscard:
		mapFlags = D3D11_MAP_WRITE_DISCARD;
		break;
	}
	if (D3D11_USAGE_STAGING == D3D11Types::AsD3D11Usage(this->usage))
	{
		context->CopyResource(this->d3d11CPUVertexBuffer, this->d3d11VertexBuffer);
		hr = context->Map(this->d3d11CPUVertexBuffer, 0, mapFlags, 0, &subres);
		n_assert(SUCCEEDED(hr));
	}
	else
	{
		hr = context->Map(this->d3d11VertexBuffer, 0, mapFlags, 0, &subres);
		n_assert(SUCCEEDED(hr));
	}
	
    return subres.pData;
}

//------------------------------------------------------------------------------
/**
*/
void
D3D11VertexBuffer::Unmap()
{
    n_assert(0 != this->d3d11VertexBuffer);
    n_assert(this->mapCount > 0);
	ID3D11DeviceContext* context = D3D11RenderDevice::Instance()->GetDirect3DDeviceContext();
	if (D3D11_USAGE_STAGING == D3D11Types::AsD3D11Usage(this->usage))
	{
		context->Unmap(this->d3d11CPUVertexBuffer, 0);
	}
	else
	{
		context->Unmap(this->d3d11VertexBuffer, 0);
	}
	//
	
    this->mapCount--;
}


//------------------------------------------------------------------------------
/**
*/
void
D3D11VertexBuffer::SetD3D11VertexBuffer(ID3D11Buffer* ptr)
{
    n_assert(0 != ptr);
    n_assert(0 == this->d3d11VertexBuffer);
    this->d3d11VertexBuffer = ptr;
	
	UINT d3d11usage = D3D11Types::AsD3D11Usage(this->usage);
	UINT d3d11access = D3D11Types::AsD3D11Access(this->access);
	if (D3D11_USAGE_STAGING == d3d11usage)
	{
		if (0 != this->d3d11CPUVertexBuffer)
		{
			this->d3d11CPUVertexBuffer->Release();
			this->d3d11CPUVertexBuffer = 0;
		}

		D3D11_BUFFER_DESC desc;
		this->d3d11VertexBuffer->GetDesc(&desc);
		desc.BindFlags = 0;
		desc.Usage = (D3D11_USAGE)d3d11usage;
		desc.CPUAccessFlags = d3d11access;
		HRESULT hr = D3D11RenderDevice::Instance()->GetDirect3DDevice()->CreateBuffer(
			&desc, NULL, &this->d3d11CPUVertexBuffer);
		if (FAILED(hr))
			n_error("Could not create cpu access buffer for vertex buffer in D3D11VertexBuffer::SetD3D11VertexBuffer()!");
	}
}

} // namespace Direct3D11
