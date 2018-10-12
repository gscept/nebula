//------------------------------------------------------------------------------
//  d3d11indexbuffer.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/d3d11/d3d11indexbuffer.h"
#include "coregraphics/d3d11/d3d11renderdevice.h"
#include "coregraphics/d3d11/d3d11types.h"

namespace Direct3D11
{
__ImplementClass(Direct3D11::D3D11IndexBuffer, 'D1IB', Base::IndexBufferBase);

using namespace CoreGraphics;

//------------------------------------------------------------------------------
/**
*/
D3D11IndexBuffer::D3D11IndexBuffer() :
    d3d11IndexBuffer(0),
	d3d11CPUIndexBuffer(0),
    mapCount(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
D3D11IndexBuffer::~D3D11IndexBuffer()
{
    n_assert(0 == this->d3d11IndexBuffer);
    n_assert(0 == this->mapCount);
}

//------------------------------------------------------------------------------
/**
*/
void
D3D11IndexBuffer::Unload()
{
    n_assert(0 == this->mapCount);
    if (0 != this->d3d11IndexBuffer)
    {
        this->d3d11IndexBuffer->Release();
        this->d3d11IndexBuffer = 0;
    }
	if (0 != this->d3d11CPUIndexBuffer)
	{
		this->d3d11CPUIndexBuffer->Release();
		this->d3d11CPUIndexBuffer = 0;
	}
    IndexBufferBase::Unload();
}
 
//------------------------------------------------------------------------------
/**
*/
void*
D3D11IndexBuffer::Map(MapType mapType)
{
    n_assert(0 != this->d3d11IndexBuffer);
    this->mapCount++;
	ID3D11DeviceContext* context = D3D11RenderDevice::Instance()->GetDirect3DDeviceContext();
	context->CopyResource(this->d3d11CPUIndexBuffer, this->d3d11IndexBuffer);
	
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
		hr = context->Map(this->d3d11CPUIndexBuffer, 0, mapFlags, 0, &subres);
		n_assert(SUCCEEDED(hr));
	}
	else
	{
		hr = context->Map(this->d3d11IndexBuffer, 0, mapFlags, 0, &subres);
		n_assert(SUCCEEDED(hr));
	}
    return subres.pData;
}

//------------------------------------------------------------------------------
/**
*/
void
D3D11IndexBuffer::Unmap()
{
    n_assert(0 != this->d3d11IndexBuffer);
    n_assert(this->mapCount > 0);
	ID3D11DeviceContext* context = D3D11RenderDevice::Instance()->GetDirect3DDeviceContext();

	if (D3D11_USAGE_STAGING == D3D11Types::AsD3D11Usage(this->usage))
		context->Unmap(this->d3d11CPUIndexBuffer, 0);
	else
		context->Unmap(this->d3d11IndexBuffer, 0);
    this->mapCount--;
}

//------------------------------------------------------------------------------
/**
*/
void
D3D11IndexBuffer::SetD3D11IndexBuffer(ID3D11Buffer* ptr)
{
    n_assert(0 != ptr);
    n_assert(0 == this->d3d11IndexBuffer);
    this->d3d11IndexBuffer = ptr;
	
	UINT d3d11usage = D3D11Types::AsD3D11Usage(this->usage);
	UINT d3d11access = D3D11Types::AsD3D11Access(this->access);
	if (D3D11_USAGE_STAGING == d3d11usage)
	{
		if (0 != this->d3d11CPUIndexBuffer)
		{
			this->d3d11CPUIndexBuffer->Release();
			this->d3d11CPUIndexBuffer = 0;
		}
		
		D3D11_BUFFER_DESC desc;
		this->d3d11IndexBuffer->GetDesc(&desc);
		desc.BindFlags = 0;
		desc.Usage = (D3D11_USAGE)d3d11usage;
		desc.CPUAccessFlags = d3d11access;
		HRESULT hr = D3D11RenderDevice::Instance()->GetDirect3DDevice()->CreateBuffer(
			&desc, NULL, &this->d3d11CPUIndexBuffer);
		if (FAILED(hr))
			n_error("Could not create cpu access buffer for index buffer in D3D11IndexBuffer::SetD3D11IndexBuffer()!");
	}
}

} // namespace CoreGraphics
