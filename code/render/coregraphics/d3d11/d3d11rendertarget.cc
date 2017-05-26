//------------------------------------------------------------------------------
//  D3D11RenderDevice.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------


#include "stdneb.h"
#include "coregraphics/config.h"
#include "coregraphics/rendertarget.h"
#include "coregraphics/d3d11/d3d11renderdevice.h"
#include "coregraphics/d3d11/d3d11types.h"
#include "coregraphics/displaydevice.h"
#include "resources/resourcemanager.h"
#include "coregraphics/shadersemantics.h"
#include "coregraphics/shaderserver.h"
#include "coregraphics/renderdevice.h"

namespace Direct3D11
{
__ImplementClass(Direct3D11::D3D11RenderTarget, 'D1RT', Base::RenderTargetBase);

using namespace Direct3D11;
using namespace CoreGraphics;
using namespace Resources;

//------------------------------------------------------------------------------
/**
*/
D3D11RenderTarget::D3D11RenderTarget() :
    d3d11ResolveTexture(0),
    d3d11CPUResolveTexture(0),
	d3d11BackBufferRenderView(0),
	d3d11CPUResolveTextureRenderView(0),
	d3d11ResolveTextureRenderView(0),
    d3d11ColorBufferFormat(DXGI_FORMAT_UNKNOWN),
    needsResolve(false)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
D3D11RenderTarget::~D3D11RenderTarget()
{
    n_assert(!this->isValid);
    n_assert(0 == this->d3d11ResolveTexture);
}

//------------------------------------------------------------------------------
/**
*/
void
D3D11RenderTarget::Setup()
{
    n_assert(0 == this->d3d11ResolveTexture);
    HRESULT hr;

    ID3D11Device* d3d11Dev = D3D11RenderDevice::Instance()->GetDirect3DDevice();
    
    // call parent class
    RenderTargetBase::Setup();

    // if we're the default render target, query display device
    // for setup parameters
    if (this->isDefaultRenderTarget)
    {
        // NOTE: the default render target will never be anti-aliased!
        // this assumes a render pipeline where the actual rendering goes
        // into an offscreen render target and is then resolved to the back buffer
		DisplayDevice* displayDevice = DisplayDevice::Instance();
        this->SetWidth(displayDevice->GetDisplayMode().GetWidth());
        this->SetHeight(displayDevice->GetDisplayMode().GetHeight());
        this->SetColorBufferFormat(displayDevice->GetDisplayMode().GetPixelFormat());
		this->SetAntiAliasQuality(displayDevice->GetAntiAliasQuality());

		this->resolveRect.right = displayDevice->GetDisplayMode().GetWidth();
		this->resolveRect.bottom = displayDevice->GetDisplayMode().GetHeight();
		this->resolveRect.left = 0;
		this->resolveRect.top = 0;		

		hr = D3D11RenderDevice::Instance()->GetDXGISwapChain()->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&this->d3d11ResolveTexture);
		n_assert(SUCCEEDED(hr));
		D3D11_TEXTURE2D_DESC desc;
		this->d3d11ResolveTexture->GetDesc(&desc);
		hr = d3d11Dev->CreateRenderTargetView(this->d3d11ResolveTexture, NULL, &this->d3d11ResolveTextureRenderView);
		n_assert(SUCCEEDED(hr));
    }
	else if (this->relativeSizeValid)
	{
		DisplayDevice* displayDevice = DisplayDevice::Instance();
		this->SetWidth(SizeT(displayDevice->GetDisplayMode().GetWidth() * this->relWidth));
		this->SetHeight(SizeT(displayDevice->GetDisplayMode().GetHeight() * this->relHeight));
	}

    // setup our pixel format and multisample parameters (order important!)
    this->d3d11ColorBufferFormat = D3D11Types::AsD3D11PixelFormat(this->colorBufferFormat);
    this->SetupMultiSampleType();

    // check if a resolve texture must be allocated
    if (this->mipMapsEnabled ||
        (1 != this->d3d11MultiSampleDescription.Count) ||
        (this->resolveTextureDimensionsValid &&
         ((this->resolveTextureWidth != this->width) ||
          (this->resolveTextureHeight != this->height))))
    {
        this->needsResolve = true;
    }
    else
    {
        this->needsResolve = false;
    }

    // create the render target either as a texture, or as
    // a surface, or don't create it if rendering goes
    // into backbuffer
    if (!this->needsResolve)
    {
        if (!this->isDefaultRenderTarget)
        {
			D3D11_USAGE usage = D3D11_USAGE_DEFAULT;

			this->resolveRect.left = 0;
			this->resolveRect.top = 0;
			this->resolveRect.right = this->width;
			this->resolveRect.bottom = this->height;	
			
			CD3D11_TEXTURE2D_DESC desc = CD3D11_TEXTURE2D_DESC(
				this->d3d11ColorBufferFormat,
				this->width,
				this->height,
				1,
				1,
				D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
				usage);	


			desc.SampleDesc = this->d3d11MultiSampleDescription;

			hr = d3d11Dev->CreateTexture2D(&desc, NULL, &this->d3d11ResolveTexture);
			// check if texture was created
			n_assert(SUCCEEDED(hr));
			n_assert(0 != this->d3d11ResolveTexture);

			hr = d3d11Dev->CreateRenderTargetView(this->d3d11ResolveTexture, NULL, &this->d3d11ResolveTextureRenderView);
			n_assert(SUCCEEDED(hr));
			n_assert(0 != this->d3d11ResolveTextureRenderView);
						
			// create extra cpu texture for cpu look up (read-only)
			if (this->resolveCpuAccess)
			{
				desc = CD3D11_TEXTURE2D_DESC(
					this->d3d11ColorBufferFormat,
					this->width,
					this->height,
					1,
					1,
					0,
					D3D11_USAGE_STAGING,
					D3D11_CPU_ACCESS_READ);	
				hr = d3d11Dev->CreateTexture2D(&desc, NULL, &this->d3d11CPUResolveTexture);

				n_assert(SUCCEEDED(hr));
				n_assert(0 != this->d3d11CPUResolveTexture);
			}

        }
        else
        {
            // NOTE: if we are the default render target and not antialiased, 
            // rendering will go directly to the backbuffer, so there's no
            // need to allocate a render target
        }
    }
	else
	{
		D3D11_USAGE usage = D3D11_USAGE_DEFAULT;

		SizeT resolveWidth = this->resolveTextureDimensionsValid ? this->resolveTextureWidth : this->width;
		SizeT resolveHeight = this->resolveTextureDimensionsValid ? this->resolveTextureHeight : this->height;

		this->resolveRect.left = 0;
		this->resolveRect.top = 0;
		this->resolveRect.right = resolveWidth;
		this->resolveRect.bottom = resolveHeight;

		CD3D11_TEXTURE2D_DESC desc;
		if (this->mipMapsEnabled)
		{
			 desc = CD3D11_TEXTURE2D_DESC(
				this->d3d11ColorBufferFormat,
				resolveWidth,
				resolveHeight,
				1,
				this->mipLevels,
				D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
				usage);
			 desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
		}
		else
		{
			desc = CD3D11_TEXTURE2D_DESC(
				this->d3d11ColorBufferFormat,
				resolveWidth,
				resolveHeight,
				1,
				1,
				D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
				usage);	
		}

		desc.SampleDesc = this->d3d11MultiSampleDescription;

		hr = d3d11Dev->CreateTexture2D(&desc, NULL, &this->d3d11ResolveTexture);
		n_assert(SUCCEEDED(hr));
		n_assert(0 != this->d3d11ResolveTexture);

		hr = d3d11Dev->CreateRenderTargetView(this->d3d11ResolveTexture, NULL, &this->d3d11ResolveTextureRenderView);
		n_assert(SUCCEEDED(hr));
		n_assert(0 != this->d3d11ResolveTextureRenderView);

		// create extra cpu texture for cpu look up (read-only)
		if (this->resolveCpuAccess)
		{
			desc = CD3D11_TEXTURE2D_DESC(
				this->d3d11ColorBufferFormat,
				this->width,
				this->height,
				1,
				1,
				0,
				D3D11_USAGE_STAGING,
				D3D11_CPU_ACCESS_READ);	

			hr = d3d11Dev->CreateTexture2D(&desc, NULL, &this->d3d11CPUResolveTexture);

			n_assert(SUCCEEDED(hr));
			n_assert(0 != this->d3d11CPUResolveTexture);
		}
	}

    // if a resolve texture exists, create a shared texture resource, so that
    // the texture is publicly visible
    if (this->d3d11ResolveTexture && this->resolveTextureResId.IsValid())
    {
		this->resolveTexture = ResourceManager::Instance()->CreateUnmanagedResource(this->resolveTextureResId, Texture::RTTI).downcast<Texture>();
		this->resolveTexture->SetupFromD3D11Texture(this->d3d11ResolveTexture, true);
    }
    // if a cpu resolve texture exists, create a shared texture resource, so that
    // the texture is usable for a cpu lockup
    if (this->d3d11CPUResolveTexture && this->resolveTextureResId.IsValid())
    {
        Resources::ResourceId resolveCPUTextureResId = Util::String(this->resolveTextureResId.AsString() + "_CPU_ReadOnly");
        this->resolveCPUTexture = ResourceManager::Instance()->CreateUnmanagedResource(resolveCPUTextureResId, Texture::RTTI).downcast<Texture>();  
        this->resolveCPUTexture->SetUsage(Texture::UsageDynamic);
        this->resolveCPUTexture->SetAccess(Texture::AccessRead);               
        this->resolveCPUTexture->SetupFromD3D11Texture(this->d3d11CPUResolveTexture);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
D3D11RenderTarget::Discard()
{
    RenderTargetBase::Discard();

    if (0 != this->d3d11ResolveTexture)
    {
        this->d3d11ResolveTexture->Release();
        this->d3d11ResolveTexture = 0;
    }

	this->d3d11ResolveTextureRenderView->Release();
	this->d3d11ResolveTextureRenderView = 0;

	if (this->d3d11CPUResolveTextureRenderView)
	{
		this->d3d11CPUResolveTextureRenderView->Release();
		this->d3d11CPUResolveTextureRenderView = 0;
	}
		
	this->d3d11CPUResolveTexture = 0;
    this->sharedPixelSize = 0;
    this->sharedHalfPixelSize = 0;
}

//------------------------------------------------------------------------------
/**
    Select the antialias parameters that most closely resembly 
    the preferred settings in the DisplayDevice object.
*/
void
D3D11RenderTarget::SetupMultiSampleType()
{
    n_assert(D3DFMT_UNKNOWN != this->d3d11ColorBufferFormat);
    D3D11RenderDevice* renderDevice = D3D11RenderDevice::Instance();
    ID3D11Device* d3d11 = renderDevice->GetDirect3DDevice();

    #if NEBULA3_DIRECT3D_DEBUG
        this->d3d11MultiSampleDescription.Count = 0;
        this->d3d11MultiSampleDescription.Quality = 0;
    #else
        // convert Nebula3 antialias quality into D3D type
        this->d3d11MultiSampleDescription.Count = D3D11Types::AsD3D11MultiSampleType(this->antiAliasQuality);
		this->d3d11MultiSampleDescription.Quality = 0;

		if (this->d3d11MultiSampleDescription.Count > 1)
		{
			// check if the multisample type is compatible with the selected display mode
			UINT availableQualityLevels = 0;
			UINT depthBufferQualityLevels = 0;
			HRESULT renderTargetResult = d3d11->CheckMultisampleQualityLevels(this->d3d11ColorBufferFormat, d3d11MultiSampleDescription.Count, &availableQualityLevels);
			HRESULT depthBufferResult = d3d11->CheckMultisampleQualityLevels(DXGI_FORMAT_D24_UNORM_S8_UINT, d3d11MultiSampleDescription.Count, &depthBufferQualityLevels);

			if ((S_OK != renderTargetResult) || (S_OK != depthBufferResult))
			{
				// reset to no multisampling
				this->d3d11MultiSampleDescription.Count = 0;
				this->d3d11MultiSampleDescription.Quality = 0;

			}
			else
			{
				n_assert(SUCCEEDED(renderTargetResult) && SUCCEEDED(depthBufferResult));
			}

			// clamp multisample quality to the available quality levels
			if (availableQualityLevels > 0)
			{
				this->d3d11MultiSampleDescription.Quality = availableQualityLevels - 1;
			}
			else
			{
				this->d3d11MultiSampleDescription.Quality = 0;
			}
		}
        

    #endif
}  

//------------------------------------------------------------------------------
/**
*/
void
D3D11RenderTarget::BeginPass()
{
    ID3D11Device* d3d11Dev = D3D11RenderDevice::Instance()->GetDirect3DDevice();
	IDXGISwapChain* swapChain = D3D11RenderDevice::Instance()->GetDXGISwapChain();
	ID3D11DeviceContext* context = D3D11RenderDevice::Instance()->GetDirect3DDeviceContext();
	D3D11RenderDevice* renderDevice = RenderDevice::Instance();

    // apply the render target (may be the back buffer)
    if (0 != this->d3d11ResolveTextureRenderView)
    {
		renderDevice->AddRenderTarget(this->d3d11ResolveTextureRenderView);
		//renderDevice->AddViewport(this->resolveRect);
    }

	// set depth-stencil target
	if (this->depthStencilTarget.isvalid())
	{
		this->depthStencilTarget->SetClearDepth(this->clearDepth);
		this->depthStencilTarget->SetClearStencil(this->clearStencil);
		this->depthStencilTarget->SetClearFlags(this->clearFlags);
		this->depthStencilTarget->BeginPass();
		
		// set depth-stencil target in device
		renderDevice->SetDepthStencilTarget(this->depthStencilTarget->GetD3D11DepthStencilView());		
	}
	
	// call base class
    RenderTargetBase::BeginPass();

	// clear render target
	this->Clear();

	const Ptr<CoreGraphics::ShaderInstance>& instance = D3D11RenderDevice::Instance()->GetPassShader();
	if (instance.isvalid() && instance->HasVariableBySemantic("TextureRatio"))
	{
		Ptr<CoreGraphics::ShaderVariable> var = instance->GetVariableBySemantic("TextureRatio");
		uint xRatio = DisplayDevice::Instance()->GetDisplayMode().GetHeight() / this->GetHeight();
		uint yRatio = DisplayDevice::Instance()->GetDisplayMode().GetHeight() / this->GetHeight();
		var->SetFloat4(Math::float4((float)xRatio, (float)yRatio, 0, 0));
	}    
}

//------------------------------------------------------------------------------
/**
*/
void 
D3D11RenderTarget::Clear()
{
	ID3D11DeviceContext* context = D3D11RenderDevice::Instance()->GetDirect3DDeviceContext();
	if (0 != (this->clearFlags & ClearColor))
	{
		const FLOAT RGBA[4] = { clearColor.x(), clearColor.y(),clearColor.z(),clearColor.w() };
		context->ClearRenderTargetView(d3d11ResolveTextureRenderView, RGBA);
		this->clearColor = clearColor;
	}
}

//------------------------------------------------------------------------------
/**
*/
void
D3D11RenderTarget::EndPass()
{

    ID3D11Device* d3d11Dev = D3D11RenderDevice::Instance()->GetDirect3DDevice();
	ID3D11DeviceContext* d3d11Context = D3D11RenderDevice::Instance()->GetDirect3DDeviceContext();

    // if necessary need to resolve the render target, either
    // into our resolve texture, or into the back buffer
    if (this->needsResolve)
    {        
        RECT destRect;
        CONST RECT* pDestRect = NULL;
        if (this->resolveRectValid)
        {
            destRect.left   = this->resolveRect.left;
            destRect.right  = this->resolveRect.right;
            destRect.top    = this->resolveRect.top;
            destRect.bottom = this->resolveRect.bottom;
            pDestRect = &destRect;
        }

        // need cpu access, copy from gpu mem to sys mem
        if (this->resolveCpuAccess)
        {
			d3d11Context->CopyResource(this->d3d11CPUResolveTexture, this->d3d11ResolveTexture);
        }
    }
    else if (this->resolveCpuAccess)
    {
        // copy data
		d3d11Context->CopyResource(this->d3d11CPUResolveTexture, this->d3d11ResolveTexture);
    }

	// end pass for depth-stencil target
	if (this->depthStencilTarget.isvalid())
	{
		this->depthStencilTarget->EndPass();
	}

    RenderTargetBase::EndPass();
}

//------------------------------------------------------------------------------
/**
*/
void
D3D11RenderTarget::GenerateMipLevels()
{
    n_assert(0 != this->d3d11ResolveTexture);
    n_assert(this->mipMapsEnabled);

	this->resolveTexture->GenerateMipmaps();
}

//------------------------------------------------------------------------------
/**
*/
void 
D3D11RenderTarget::OnDisplayResized()
{
	// get render device
	ID3D11Device* d3d11Dev = D3D11RenderDevice::Instance()->GetDirect3DDevice();
	DisplayDevice* displayDevice = DisplayDevice::Instance();

	// save depth stencil
	if (this->d3d11ResolveTexture && this->relativeSizeValid)
	{
		// get description
		D3D11_TEXTURE2D_DESC desc;
		this->d3d11ResolveTexture->GetDesc(&desc);

		// recalculate dimensions
		if (this->relativeSizeValid)
		{
			this->SetWidth(SizeT(displayDevice->GetDisplayMode().GetWidth() * this->relWidth));
			this->SetHeight(SizeT(displayDevice->GetDisplayMode().GetHeight() * this->relHeight));
		}

		this->resolveRect.right = this->width;
		this->resolveRect.bottom = this->height;

		// release render target view
		this->d3d11ResolveTextureRenderView->Release();
		this->d3d11ResolveTextureRenderView = 0;

		// release texture
		this->d3d11ResolveTexture->Release();
		this->d3d11ResolveTexture = 0;
	
		// set new size
		desc.Width = this->width;
		desc.Height = this->height;

		// create new textures
		HRESULT hr;
		hr = d3d11Dev->CreateTexture2D(&desc, NULL, &this->d3d11ResolveTexture);
		n_assert(SUCCEEDED(hr));

		hr = d3d11Dev->CreateRenderTargetView(this->d3d11ResolveTexture, NULL, &this->d3d11ResolveTextureRenderView);
		n_assert(SUCCEEDED(hr));

		// unload resolve texture
		this->resolveTexture->Unload();
		this->resolveTexture->SetupFromD3D11Texture(this->d3d11ResolveTexture, true);
	}
	else if (this->isDefaultRenderTarget)
	{
		// update dimensions
		this->SetWidth(SizeT(displayDevice->GetDisplayMode().GetWidth()));
		this->SetHeight(SizeT(displayDevice->GetDisplayMode().GetHeight()));

		this->resolveRect.right = this->width;
		this->resolveRect.bottom = this->height;

		// release render target view
		this->d3d11ResolveTextureRenderView->Release();
		this->d3d11ResolveTextureRenderView = 0;

		// release texture
		this->d3d11ResolveTexture->Release();
		this->d3d11ResolveTexture = 0;

		HRESULT hr;
		hr = D3D11RenderDevice::Instance()->GetDXGISwapChain()->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&this->d3d11ResolveTexture);
		n_assert(SUCCEEDED(hr));
		D3D11_TEXTURE2D_DESC desc;
		this->d3d11ResolveTexture->GetDesc(&desc);
		hr = d3d11Dev->CreateRenderTargetView(this->d3d11ResolveTexture, NULL, &this->d3d11ResolveTextureRenderView);
		n_assert(SUCCEEDED(hr));
	}

	this->Clear();
}
} // namespace Direct3D11

