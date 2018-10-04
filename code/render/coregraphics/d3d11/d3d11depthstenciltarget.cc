//------------------------------------------------------------------------------
//  d3d11depthstenciltarget.cc
//  (C) 2013 Gustav Sterbrant
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "d3d11depthstenciltarget.h"
#include "d3d11renderdevice.h"
#include "coregraphics/displaydevice.h"

namespace Direct3D11
{
__ImplementClass(Direct3D11::D3D11DepthStencilTarget, 'D1DT', Base::DepthStencilTargetBase);

using namespace CoreGraphics;
//------------------------------------------------------------------------------
/**
*/
D3D11DepthStencilTarget::D3D11DepthStencilTarget() :
	d3d11DepthStencil(0),
	d3d11DepthStencilView(0)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
D3D11DepthStencilTarget::~D3D11DepthStencilTarget()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void 
D3D11DepthStencilTarget::Setup()
{
	n_assert(0 == this->d3d11DepthStencil);

	// setup base class
	DepthStencilTargetBase::Setup();

	// if we have a relative size on the dept-stencil target, calculate actual size
	if (this->useRelativeSize)
	{
		DisplayDevice* displayDevice = DisplayDevice::Instance();
		this->SetWidth(SizeT(displayDevice->GetDisplayMode().GetWidth() * this->relWidth));
		this->SetHeight(SizeT(displayDevice->GetDisplayMode().GetHeight() * this->relHeight));
	}


	ID3D11Device* d3d11Dev = D3D11RenderDevice::Instance()->GetDirect3DDevice();
	CD3D11_TEXTURE2D_DESC desc = CD3D11_TEXTURE2D_DESC(
		DXGI_FORMAT_D24_UNORM_S8_UINT,
		this->width,
		this->height,
		1,
		1,
		D3D11_BIND_DEPTH_STENCIL,
		D3D11_USAGE_DEFAULT);

	HRESULT hr;
	hr = d3d11Dev->CreateTexture2D(&desc, NULL, &this->d3d11DepthStencil);
	n_assert(SUCCEEDED(hr));
	n_assert(0 != this->d3d11DepthStencil);
	hr = d3d11Dev->CreateDepthStencilView(this->d3d11DepthStencil, NULL, &this->d3d11DepthStencilView);
	n_assert(SUCCEEDED(hr));
	n_assert(0 != this->d3d11DepthStencilView);
}

//------------------------------------------------------------------------------
/**
*/
void 
D3D11DepthStencilTarget::Discard()
{
	DepthStencilTargetBase::Discard();

	this->d3d11DepthStencil->Release();
	this->d3d11DepthStencil = 0;
	
	this->d3d11DepthStencilView->Release();
	this->d3d11DepthStencilView = 0;
}

//------------------------------------------------------------------------------
/**
*/
void 
D3D11DepthStencilTarget::BeginPass()
{
	DepthStencilTargetBase::BeginPass();
	this->Clear();
}

//------------------------------------------------------------------------------
/**
*/
void 
D3D11DepthStencilTarget::EndPass()
{
	DepthStencilTargetBase::EndPass();
}

//------------------------------------------------------------------------------
/**
*/
void 
D3D11DepthStencilTarget::Clear()
{
	ID3D11DeviceContext* context = D3D11RenderDevice::Instance()->GetDirect3DDeviceContext();
	uint d3d11ClearFlags = 0;
	if (0 != (this->clearFlags & ClearDepth))
	{
		d3d11ClearFlags |= D3D11_CLEAR_DEPTH;
	}
	if (0 != (this->clearFlags & ClearStencil))
	{
		d3d11ClearFlags |= D3D11_CLEAR_STENCIL;
	}

	if (0 != d3d11ClearFlags)
	{
		context->ClearDepthStencilView(d3d11DepthStencilView, d3d11ClearFlags, this->clearDepth, this->clearStencil);
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
D3D11DepthStencilTarget::OnDisplayResized()
{
	// if we have a relative size on the dept-stencil target, calculate actual size
	if (this->useRelativeSize)
	{
		DisplayDevice* displayDevice = DisplayDevice::Instance();
		this->SetWidth(SizeT(displayDevice->GetDisplayMode().GetWidth() * this->relWidth));
		this->SetHeight(SizeT(displayDevice->GetDisplayMode().GetHeight() * this->relHeight));
	}

	// release reference to depth stencil stuff
	this->d3d11DepthStencil->Release();
	this->d3d11DepthStencil = 0;

	this->d3d11DepthStencilView->Release();
	this->d3d11DepthStencilView = 0;

	ID3D11Device* d3d11Dev = D3D11RenderDevice::Instance()->GetDirect3DDevice();
	CD3D11_TEXTURE2D_DESC desc = CD3D11_TEXTURE2D_DESC(
		DXGI_FORMAT_D24_UNORM_S8_UINT,
		this->width,
		this->height,
		1,
		1,
		D3D11_BIND_DEPTH_STENCIL,
		D3D11_USAGE_DEFAULT);

	HRESULT hr;
	hr = d3d11Dev->CreateTexture2D(&desc, NULL, &this->d3d11DepthStencil);
	n_assert(SUCCEEDED(hr));
	n_assert(0 != this->d3d11DepthStencil);
	hr = d3d11Dev->CreateDepthStencilView(this->d3d11DepthStencil, NULL, &this->d3d11DepthStencilView);
	n_assert(SUCCEEDED(hr));
	n_assert(0 != this->d3d11DepthStencilView);
}

} // namespace Direct3D11