//------------------------------------------------------------------------------
//  D3D11RenderDevice.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "stdneb.h"
#include "coregraphics/config.h"
#include "coregraphics/d3d11/D3D11RenderDevice.h"
#include "coregraphics/displaydevice.h"
#include "coregraphics/vertexbuffer.h"
#include "coregraphics/indexbuffer.h"
#include "coregraphics/d3d11/d3d11types.h"
#include "coregraphics/rendertarget.h"
#include "coregraphics/shaderserver.h"
#include <dxerr.h>
#include "math/rectangle.h"



namespace Direct3D11
{
__ImplementClass(Direct3D11::D3D11RenderDevice, 'D1RD', Base::RenderDeviceBase);
__ImplementSingleton(Direct3D11::D3D11RenderDevice);

using namespace Direct3D11;
using namespace CoreGraphics;

IDXGISwapChain* D3D11RenderDevice::d3d11SwapChain = 0;
ID3D11Device* D3D11RenderDevice::d3d11Device = 0;
ID3D11DeviceContext* D3D11RenderDevice::d3d11DeviceContext = 0;


//------------------------------------------------------------------------------
/**
*/
D3D11RenderDevice::D3D11RenderDevice() :
    adapter(0),
    displayFormat(DXGI_FORMAT_R16G16B16A16_FLOAT),
	depthStencilView(0),
    deviceBehaviourFlags(0),
    frameId(0),
	tessellate(0)
{
    __ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
D3D11RenderDevice::~D3D11RenderDevice()
{
    if (this->IsOpen())
    {
        this->Close();
    }
    this->CloseDirect3D();
    __DestructSingleton;
}



//------------------------------------------------------------------------------
/**
    Return a pointer to d3d device. Asserts that the device exists.
*/
ID3D11Device*
D3D11RenderDevice::GetDirect3DDevice() const
{
    n_assert(0 != this->d3d11Device);
    return this->d3d11Device;
}

//------------------------------------------------------------------------------
/**
    Return a pointer to d3d device context. Asserts that the device context exists.
*/
ID3D11DeviceContext*
D3D11RenderDevice::GetDirect3DDeviceContext() const
{
    n_assert(0 != this->d3d11DeviceContext);
    return this->d3d11DeviceContext;
}

//------------------------------------------------------------------------------
/**
    Return a pointer to the DXGI swap chain. Asserts if there is no swap chain
*/
IDXGISwapChain*
D3D11RenderDevice::GetDXGISwapChain() const
{
    n_assert(0 != this->d3d11SwapChain);
    return this->d3d11SwapChain;
}

//------------------------------------------------------------------------------
/**
    Return a pointer to d3d device context. Asserts that the device context exists.
*/
D3D_FEATURE_LEVEL
D3D11RenderDevice::GetDirect3DFeatureLevel() const
{
    n_assert(0 != this->usedFeatureLevel);
    return this->usedFeatureLevel;
}


//------------------------------------------------------------------------------
/**
    Close Direct3D11. This method is exclusively called by the destructor.
*/
void
D3D11RenderDevice::CloseDirect3D()
{
    if (0 != d3d11Device)
    {
		d3d11Device->Release();
        d3d11DeviceContext->Release();
    }
}

//------------------------------------------------------------------------------
/**
    Open the render device. When successful, the RenderEvent::DeviceOpen
    will be sent to all registered event handlers after the Direct3D
    device has been opened.
*/
bool
D3D11RenderDevice::Open()
{
    n_assert(!this->IsOpen());
    bool success = false;
    if (this->OpenDirect3DDevice())
    {
        // hand to parent class, this will notify event handlers
        success = RenderDeviceBase::Open();
    }
    return success;
}

//------------------------------------------------------------------------------
/**
    Close the render device. The RenderEvent::DeviceClose will be sent to
    all registered event handlers.
*/
void
D3D11RenderDevice::Close()
{
    n_assert(this->IsOpen());
    RenderDeviceBase::Close();
    this->CloseDirect3DDevice();
}

//------------------------------------------------------------------------------
/**
    This selects the adapter to use for the render device. This
    method will set the "adapter" member and the d3d11 device caps
    member.
*/
void
D3D11RenderDevice::SetupAdapter()
{

    DisplayDevice* displayDevice = DisplayDevice::Instance();
    Adapter::Code requestedAdapter = displayDevice->GetAdapter();
    n_assert(displayDevice->AdapterExists(requestedAdapter));

    #if NEBULA_DIRECT3D_USENVPERFHUD
        this->adapter = this->d3d11->GetAdapterCount() - 1;
    #else
        this->adapter = (UINT) requestedAdapter;
    #endif
    
}

//------------------------------------------------------------------------------
/**
    Select the display, back buffer and depth buffer formats and update
    the presentParams member. 
*/
void
D3D11RenderDevice::SetupBufferFormats()
{
    //HRESULT hr;
    //n_assert(0 != this->d3d11Device);
    DisplayDevice* displayDevice = DisplayDevice::Instance();

    if (displayDevice->IsFullscreen())
    {
        if (displayDevice->IsTripleBufferingEnabled())
        {
            this->d3d11SwapChainDesc.BufferCount = 3;
        }
        else
        {
            this->d3d11SwapChainDesc.BufferCount = 2;
        }
		this->d3d11SwapChainDesc.Windowed = FALSE;
    }
    else
    {
        // windowed mode
        this->d3d11SwapChainDesc.BufferCount = 2;
        this->d3d11SwapChainDesc.Windowed = TRUE;
    }

	if (displayDevice->IsDisplayModeSwitchEnabled())
	{
		this->d3d11SwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	}
	else
	{
		this->d3d11SwapChainDesc.Flags = 0;
	}

    DXGI_FORMAT backbufferPixelFormat;
	
    if (this->d3d11SwapChainDesc.Windowed)
    {
        // windowed mode: use desktop pixel format
        backbufferPixelFormat = D3D11Types::AsD3D11PixelFormat(displayDevice->GetCurrentAdapterDisplayMode((Adapter::Code)this->adapter).GetPixelFormat());
    }
    else
    {
        // fullscreen: use requested pixel format
        backbufferPixelFormat = D3D11Types::AsD3D11PixelFormat(displayDevice->GetDisplayMode().GetPixelFormat());
    }


	this->d3d11SwapChainDesc.BufferDesc.Format = backbufferPixelFormat;
	this->d3d11SwapChainDesc.BufferDesc.Width = displayDevice->GetDisplayMode().GetWidth();
	this->d3d11SwapChainDesc.BufferDesc.Height = displayDevice->GetDisplayMode().GetHeight();
}


//------------------------------------------------------------------------------
/**
    Setup the (remaining) presentation parameters. This will initialize
    the presentParams member.
*/
void
D3D11RenderDevice::SetupPresentParams()
{
    DisplayDevice* displayDevice = DisplayDevice::Instance();
    n_assert(displayDevice->IsOpen());
    
	this->d3d11SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	this->d3d11SwapChainDesc.OutputWindow = displayDevice->GetHwnd();
	DisplayMode mode = displayDevice->GetDisplayMode();
	this->d3d11SwapChainDesc.BufferDesc.RefreshRate.Numerator = mode.GetRefreshRate();
	this->d3d11SwapChainDesc.BufferDesc.RefreshRate.Denominator = 0;
}

//------------------------------------------------------------------------------
/**
    Initialize the Direct3D device with initial device state.
*/
void
D3D11RenderDevice::SetInitialDeviceState()
{
    n_assert(this->d3d11Device);

    // setup viewport
	dx11ViewPort.Width = (float)this->d3d11SwapChainDesc.BufferDesc.Width;
	dx11ViewPort.Height = (float)this->d3d11SwapChainDesc.BufferDesc.Height;
	dx11ViewPort.TopLeftX = 0;
	dx11ViewPort.TopLeftY = 0;
	dx11ViewPort.MinDepth = ViewportMinZ;
	dx11ViewPort.MaxDepth = ViewportMaxZ;	
}

//------------------------------------------------------------------------------
/**
    Open the Direct3D device. This will completely setup the device
    into a usable state.
*/
bool
D3D11RenderDevice::OpenDirect3DDevice()
{
    n_assert(0 == this->d3d11DeviceContext);
    n_assert(0 == this->d3d11Device);
    HRESULT hr;
    DisplayDevice* displayDevice = DisplayDevice::Instance();
    n_assert(displayDevice->IsOpen());

    // setup device creation parameters
    //Memory::Clear(&this->d3d11SwapChain, sizeof(this->d3d11SwapChain));
    this->SetupAdapter();
    this->SetupBufferFormats();
    this->SetupPresentParams();
    
    // create device
	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3
	};
	
	// setup the rest of the swap chain settings
	d3d11SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	d3d11SwapChainDesc.BufferDesc.RefreshRate.Numerator = 59;
	d3d11SwapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	d3d11SwapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE;
	d3d11SwapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_STRETCHED;
	d3d11SwapChainDesc.OutputWindow = displayDevice->GetHwnd();
	d3d11SwapChainDesc.SampleDesc.Count = D3D11Types::AsD3D11MultiSampleType(displayDevice->GetAntiAliasQuality());
	d3d11SwapChainDesc.SampleDesc.Quality = 0;

	int extraFlags = D3D11_CREATE_DEVICE_SINGLETHREADED;
#if _DEBUG_DX11
	extraFlags = D3D11_CREATE_DEVICE_DEBUG;
#endif
	hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, extraFlags, featureLevels, 4, D3D11_SDK_VERSION, &d3d11SwapChainDesc, &d3d11SwapChain, &d3d11Device, &usedFeatureLevel, &d3d11DeviceContext);

    if (!SUCCEEDED(hr))
    {
        n_error("Failed to create Direct3D device object: %s!\n", DXGetErrorString(hr));
        return false;
    }

	UINT* formatSupport = new UINT[sizeof(D3D11_FORMAT_SUPPORT )];
	hr = d3d11Device->CheckFormatSupport(DXGI_FORMAT_D24_UNORM_S8_UINT, formatSupport);

	if (!SUCCEEDED(hr))
	{
		n_error("Rendering device doesn't support D24S8 depth buffer!\n");
	}

	// initialize scissor rect
	this->d3d11ScissorRects.SetSize(MaxNumRenderTargets);

	// create wireframe rasterizer state
	CD3D11_DEFAULT def;
	CD3D11_RASTERIZER_DESC desc(def);
	desc.CullMode = D3D11_CULL_FRONT;
	desc.FillMode = D3D11_FILL_WIREFRAME;
	this->d3d11Device->CreateRasterizerState(&desc, &this->wireframeRasterizer);

    // set initial device state
    this->SetInitialDeviceState();

    // create double buffer query to avoid gpu to render more than 1 frame ahead
    IndexT i;
    for (i = 0; i < numSyncQueries; ++i)
    {
		D3D11_QUERY_DESC desc;
		desc.Query = D3D11_QUERY_EVENT;
		desc.MiscFlags = 0;
		hr = this->d3d11Device->CreateQuery(&desc, &this->gpuSyncQuery[i]);
		if (FAILED(hr))
		{
			n_error("Failed to create query: %s!\n", DXGetErrorString(hr));
			return false;
		}
    }    

    return true;
}

//------------------------------------------------------------------------------
/**
    Close the Direct3D device.
*/
void
D3D11RenderDevice::CloseDirect3DDevice()
{
    n_assert(0 != this->d3d11DeviceContext);
    n_assert(0 != this->d3d11Device);

    this->UnbindD3D11Resources();   
    IndexT i;

    // release queries
    for (i = 0; i < numSyncQueries; ++i)
    {
        this->gpuSyncQuery[i]->Release();  	
    }

	// release wireframe rasterizer
	this->wireframeRasterizer->Release();
	this->wireframeRasterizer = 0;

	// release swap chain
	this->d3d11SwapChain->Release();
	this->d3d11SwapChain = 0;

	// release context
	this->d3d11DeviceContext->Release();
	this->d3d11DeviceContext = 0;

    // release the Direct3D device
    this->d3d11Device->Release();
    this->d3d11Device = 0;	
}

//------------------------------------------------------------------------------
/**
    This catches the lost device state, and tries to restore a lost device.
    The method will send out the events DeviceLost and DeviceRestored.
    Resources should react to these events accordingly. As long as
    the device is in an invalid state, the method will return false.
    This method is called by BeginFrame().
*/
bool
D3D11RenderDevice::TestResetDevice()
{
    n_assert(0 != this->d3d11Device);

    HRESULT hr = this->d3d11Device->GetDeviceRemovedReason();
    if (hr == S_OK)
    {
        // everything is ok
        return true;
    }
    else if (DXGI_ERROR_DEVICE_RESET == hr)
    {
        // notify event handlers that the device was lost
        this->NotifyEventHandlers(RenderEvent(RenderEvent::DeviceLost));

        // if we are in windowed mode, the cause for the lost
        // device may be a desktop display mode switch, in this
        // case we need to find new buffer formats
        if (this->d3d11SwapChainDesc.Windowed)
        {
            this->SetupBufferFormats();
        }

        // set initial device state
        this->SetInitialDeviceState();

        // send the DeviceRestored event
        this->NotifyEventHandlers(RenderEvent(RenderEvent::DeviceRestored));
    }
    // the device cannot be restored at this time
    return false;
}

//------------------------------------------------------------------------------
/**
*/
void 
D3D11RenderDevice::DisplayResized()
{
	if (this->d3d11SwapChain)
	{
		// clear state
		this->UnbindD3D11Resources();

		// discard default render target
		this->defaultRenderTarget->Discard();

		// get display device
		DisplayDevice* display = DisplayDevice::Instance();

		UINT numBuffers = 2;
		if (display->IsTripleBufferingEnabled())
		{
			numBuffers = 3;
		}

		// resize swap chain
		HRESULT hr = this->d3d11SwapChain->ResizeBuffers(numBuffers, 
			0, 
			0, 
			DXGI_FORMAT_UNKNOWN,
			d3d11SwapChainDesc.Flags);
		n_assert(SUCCEEDED(hr));
		
		// setup default render target again
		this->defaultRenderTarget->Setup();
	}

	// call base class
	RenderDeviceBase::DisplayResized();
}

//------------------------------------------------------------------------------
/**
    Begin a complete frame. Call this once per frame before any rendering 
    happens. If rendering is not possible for some reason (e.g. a lost
    device) the method will return false. This method may also send
    the DeviceLost and DeviceRestored RenderEvents to attached event
    handlers.
*/
bool
D3D11RenderDevice::BeginFrame()
{
    n_assert(this->d3d11Device);
	this->d3d11DeviceContext->RSSetViewports(1, &dx11ViewPort);
    if (RenderDeviceBase::BeginFrame())
    {
        // check for and handle lost device
        if (!this->TestResetDevice())
        {
            this->inBeginFrame = false;
            return false;
        }
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
void 
D3D11RenderDevice::BeginPass( const Ptr<CoreGraphics::MultipleRenderTarget>& mrt, const Ptr<CoreGraphics::ShaderInstance>& passShader )
{
	n_assert(mrt.isvalid());
	RenderDeviceBase::BeginPass(mrt, passShader);

	// add all viewports of the mrt as viewports
	SizeT numTargets = mrt->GetNumRendertargets();
	IndexT i;
	for (i = 0; i < numTargets; i++)
	{
		this->AddViewport(mrt->GetRenderTarget(i)->GetResolveRect());
	}

	// apply render targets, viewports and scissor rectangles
	this->SetScissorRects();
	this->SetRenderTargets();
	this->SetViewports();
}

//------------------------------------------------------------------------------
/**
*/
void 
D3D11RenderDevice::BeginPass( const Ptr<CoreGraphics::RenderTarget>& rt, const Ptr<CoreGraphics::ShaderInstance>& passShader )
{
	n_assert(rt.isvalid());
	RenderDeviceBase::BeginPass(rt, passShader);
	
	// if we have a resolve rect array, use that one first, otherwise fall back to the standard rect
	if (rt->IsResolveRectArrayValid())
	{
		const Util::Array<Math::rectangle<int> >& resolveRects = rt->GetResolveRectArray();
		IndexT i;
		for (i = 0; i < resolveRects.Size(); i++)
		{
			this->AddViewport(resolveRects[i]);
		}
	}
	else
	{
		this->AddViewport(rt->GetResolveRect());
	}

	// apply render targets, viewports and scissor rectangles
	this->SetScissorRects();
	this->SetRenderTargets();
	this->SetViewports();
}

//------------------------------------------------------------------------------
/**
    End a complete frame. Call this once per frame after rendering
    has happened and before Present(), and only if BeginFrame() returns true.
*/
void
D3D11RenderDevice::EndFrame()
{
    RenderDeviceBase::EndFrame();
}

//------------------------------------------------------------------------------
/**
    End the current rendering pass. This will flush all texture stages
    in order to keep the d3d11 resource reference counts consistent without too
    much hassle.
*/
void
D3D11RenderDevice::EndPass()
{
    RenderDeviceBase::EndPass();
	this->UnbindD3D11Resources();
	this->tessellate = false;
}

//------------------------------------------------------------------------------
/**
    NOTE: Present() should be called as late as possible after EndFrame()
    to improve parallelism between the GPU and the CPU.
*/
void
D3D11RenderDevice::Present()
{
    RenderDeviceBase::Present();
	DisplayDevice* display = DisplayDevice::Instance();
          
    // present backbuffer...
    n_assert(0 != this->d3d11Device);
    if (0 != D3D11DisplayDevice::Instance()->GetHwnd())
    {
		HRESULT hr;
#if NEBULA_RENDER_THREAD
		if (display->IsEmbedded())
		{
			hr = this->d3d11SwapChain->Present(1, 0);
		}
		else
		{
			bool b = display->IsVerticalSyncEnabled();
			hr = this->d3d11SwapChain->Present(b, 0);
		}
#else
		bool b = display->IsVerticalSyncEnabled();
		hr = this->d3d11SwapChain->Present(b, 0);
#endif			
        n_assert(SUCCEEDED(hr));
    }

    // sync cpu thread with gpu
    this->SyncGPU();
}

//------------------------------------------------------------------------------
/**
    Sets the vertex buffer to use for the next Draw().
*/
void
D3D11RenderDevice::SetStreamSource(IndexT streamIndex, const Ptr<VertexBuffer>& vb, IndexT offsetVertexIndex)
{
    n_assert((streamIndex >= 0) && (streamIndex < MaxNumVertexStreams));
    n_assert(this->inBeginPass);
    n_assert(0 != this->d3d11Device);    
    n_assert(vb.isvalid());

	Ptr<CoreGraphics::VertexLayout> layout = vb->GetVertexLayout();
    if ((this->streamVertexBuffers[streamIndex] != vb) || (this->streamVertexOffsets[streamIndex] != offsetVertexIndex))
    {
		vertexByteSize[streamIndex] = vb->GetVertexLayout()->GetVertexByteSize();
		vertexByteOffset[streamIndex] = offsetVertexIndex * vb->GetVertexLayout()->GetVertexByteSize();
		vertexBuffers[streamIndex] = vb->GetD3D11VertexBuffer();
		this->d3d11DeviceContext->IASetVertexBuffers(0, 1 + streamIndex, vertexBuffers, vertexByteSize, vertexByteOffset);	
    }
    RenderDeviceBase::SetStreamSource(streamIndex, vb, offsetVertexIndex);
}

//------------------------------------------------------------------------------
/**
    Sets the vertex layout for the next Draw()
*/
void
D3D11RenderDevice::SetVertexLayout(const Ptr<VertexLayout>& vl)
{
    n_assert(this->inBeginPass);
    n_assert(0 != this->d3d11Device);    
    n_assert(vl.isvalid());

	if (this->vertexLayout != vl)
	{
		RenderDeviceBase::SetVertexLayout(vl);
		this->d3d11DeviceContext->IASetInputLayout(vl->GetD3D11VertexDeclaration());
	}
}

//------------------------------------------------------------------------------
/**
    Sets the index buffer to use for the next Draw().
*/
void
D3D11RenderDevice::SetIndexBuffer(const Ptr<IndexBuffer>& ib)
{
    n_assert(this->inBeginPass);
    n_assert(0 != this->d3d11Device);
    n_assert(ib.isvalid());
	
	if (this->indexBuffer != ib)
	{
		RenderDeviceBase::SetIndexBuffer(ib);
		this->d3d11DeviceContext->IASetIndexBuffer(ib->GetD3D11IndexBuffer(), D3D11Types::IndexTypeAsD3D11Format(ib->GetIndexType()), 0);		
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
D3D11RenderDevice::SetPrimitiveGroup( const CoreGraphics::PrimitiveGroup& pg )
{
	if (this->primitiveGroup.GetPrimitiveTopology() != pg.GetPrimitiveTopology())
	{
		D3D11_PRIMITIVE_TOPOLOGY d3dPrimType = D3D11Types::AsD3D11PrimitiveType(pg.GetPrimitiveTopology());
		this->d3d11DeviceContext->IASetPrimitiveTopology(d3dPrimType);
	}
	RenderDeviceBase::SetPrimitiveGroup(pg);
}

//------------------------------------------------------------------------------
/**
*/
void 
D3D11RenderDevice::SetTessellation( bool state )
{
	if (state && this->primitiveGroup.GetPrimitiveTopology() != PrimitiveTopology::InvalidPrimitiveTopology)
	{
		D3D11_PRIMITIVE_TOPOLOGY d3dPrimType = D3D11Types::AsD3D11PrimitiveType(this->primitiveGroup.GetPrimitiveTopology());
		d3dPrimType = D3D11Types::AsD3D11TessellateableType(d3dPrimType);
		this->d3d11DeviceContext->IASetPrimitiveTopology(d3dPrimType);
	}
	this->tessellate = state;
}

//------------------------------------------------------------------------------
/**
    Draw the current primitive group. Requires a vertex buffer, an
    optional index buffer and a primitive group to be set through the
    respective methods. To use non-indexed rendering, set the number
    of indices in the primitive group to 0.
*/
void
D3D11RenderDevice::Draw()
{
    n_assert(this->inBeginPass);
    n_assert(0 != this->d3d11Device);

    if (this->primitiveGroup.GetNumIndices() > 0)
    {
        // use indexed rendering
        
        this->d3d11DeviceContext->DrawIndexed(
			this->primitiveGroup.GetNumIndices(), 
			this->primitiveGroup.GetBaseIndex(),
			0);

    }
    else
    {
        // use non-indexed rendering
		this->d3d11DeviceContext->Draw(
			this->primitiveGroup.GetNumVertices(), 
			this->primitiveGroup.GetBaseVertex());

    }

    // update debug stats
    _incr_counter(RenderDeviceNumPrimitives, this->primitiveGroup.GetNumPrimitives());
    _incr_counter(RenderDeviceNumDrawCalls, 1);
}

//------------------------------------------------------------------------------
/**
    Draw N instances of the current primitive group. Requires the following
    setup:
        
        - vertex stream 0: vertex buffer with instancing data, one vertex per instance
        - vertex stream 1: vertex buffer with instance geometry data
        - index buffer: index buffer for geometry data
        - primitive group: the primitive group which describes one instance
        - vertex declaration: describes a combined vertex from stream 0 and stream 1
*/
void
D3D11RenderDevice::DrawIndexedInstanced(SizeT numInstances)
{
    n_assert(this->inBeginPass);
    n_assert(numInstances > 0);
    n_assert(0 != this->d3d11Device);
    
    n_assert(this->primitiveGroup.GetNumIndices() > 0);


    this->d3d11DeviceContext->DrawIndexedInstanced(
		this->primitiveGroup.GetNumIndices(),
		numInstances,
		0,
		this->primitiveGroup.GetBaseVertex(),
		0);


    // update debug stats
    _incr_counter(RenderDeviceNumPrimitives, this->primitiveGroup.GetNumPrimitives() * numInstances);
    _incr_counter(RenderDeviceNumDrawCalls, 1);
}

//------------------------------------------------------------------------------
/**
    Save the backbuffer to the provided stream.
*/
ImageFileFormat::Code
D3D11RenderDevice::SaveScreenshot(ImageFileFormat::Code fmt, const Ptr<IO::Stream>& outStream)
{
    n_assert(!this->inBeginFrame);
    n_assert(0 != this->d3d11Device);
    HRESULT hr;

    // create a plain offscreen surface to capture data to
	ID3D11Texture2D* backBuffer;
	D3D11_TEXTURE2D_DESC desc;
	hr = this->d3d11SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBuffer);
	n_assert(SUCCEEDED(hr));
	backBuffer->GetDesc(&desc);

	desc.BindFlags = 0;
	desc.Usage = D3D11_USAGE_STAGING;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

	ID3D11Texture2D* CPUBackBuffer;

	hr = this->d3d11Device->CreateTexture2D(&desc, 0, &CPUBackBuffer);
	n_assert(SUCCEEDED(hr));

	this->d3d11DeviceContext->CopyResource(CPUBackBuffer, backBuffer);

	ID3D10Blob* textureBlob;
	hr = D3DX11SaveTextureToMemory(D3D11RenderDevice::Instance()->GetDirect3DDeviceContext(), CPUBackBuffer, D3D11Types::AsD3DXImageFileFormat(fmt), &textureBlob, 0);
	n_assert(SUCCEEDED(hr));
    // write result to stream

    void* dataPtr  = textureBlob->GetBufferPointer();
    SIZE_T dataSize = textureBlob->GetBufferSize();
    outStream->SetAccessMode(IO::Stream::WriteAccess);
    if (outStream->Open())
    {
        outStream->Write(dataPtr, (IO::Stream::Size)dataSize);
        outStream->Close();
        outStream->SetMediaType(ImageFileFormat::ToMediaType(fmt));
    }
	backBuffer->Release();

	CPUBackBuffer->Release();
    
    return fmt;
}

//------------------------------------------------------------------------------
/**
    Unbind all d3d11 resources from the device, this is necessary to keep the
    resource reference counts consistent. Should be called at the
    end of each rendering pass.
*/
void
D3D11RenderDevice::UnbindD3D11Resources()
{
	this->depthStencilView = 0;
	this->vertexLayout = 0;
	this->indexBuffer = 0;
	IndexT i;
	for (i = 0; i < MaxNumVertexStreams; i++)
	{
		this->streamVertexBuffers[i] = 0;
	}
	this->primitiveGroup.SetPrimitiveTopology(PrimitiveTopology::InvalidPrimitiveTopology);
	this->renderTargets.Clear();
	this->d3d11DeviceContext->ClearState();
	this->d3d11Viewports.Clear();
}

//------------------------------------------------------------------------------
/**
*/
void 
D3D11RenderDevice::SyncGPU()
{	
	bool queryData;
	this->frameId++;
	while ( S_OK == this->d3d11DeviceContext->GetData(this->gpuSyncQuery[0], &queryData, sizeof(int), 0))
	{
		if (queryData)
		{
			// gpu is done
			return;
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void D3D11RenderDevice::AddRenderTarget( ID3D11RenderTargetView* renderTargetView )
{
	n_assert( 0 != renderTargetView );
	renderTargets.Append(renderTargetView);
}

//------------------------------------------------------------------------------
/**
*/
void 
D3D11RenderDevice::SetDepthStencilTarget( ID3D11DepthStencilView* depthStencilView )
{
	n_assert(0 != depthStencilView);
	this->depthStencilView = depthStencilView;
}

//------------------------------------------------------------------------------
/**
*/
void D3D11RenderDevice::SetRenderTargets()
{
	n_assert( 0 != renderTargets.Size()	);
	d3d11DeviceContext->OMSetRenderTargets(renderTargets.Size(), &renderTargets.Front(), this->depthStencilView);
}

//------------------------------------------------------------------------------
/**
*/
void 
D3D11RenderDevice::SetScissorRects()
{
	if (this->d3d11ScissorRects.Size() > 0)
	{
		d3d11DeviceContext->RSSetScissorRects(this->d3d11ScissorRects.Size(), &this->d3d11ScissorRects[0]);	
	}	
}

//------------------------------------------------------------------------------
/**
*/
void 
D3D11RenderDevice::AddViewport( const Math::rectangle<int>& rect )
{
	D3D11_VIEWPORT viewport;
	viewport.TopLeftX = (float)rect.left;
	viewport.TopLeftY = (float)rect.top;
	viewport.Width = (float)rect.width();
	viewport.Height = (float)rect.height();
	viewport.MinDepth = ViewportMinZ;
	viewport.MaxDepth = ViewportMaxZ;
	
	this->d3d11Viewports.Append(viewport);
}

//------------------------------------------------------------------------------
/**
*/
void 
D3D11RenderDevice::SetScissorRect( const Math::rectangle<int>& rect, int index )
{
	D3D11_RECT rectangle;
	rectangle.left = rect.left;
	rectangle.top = rect.top;
	rectangle.right = rect.right;
	rectangle.bottom = rect.bottom;

	this->d3d11ScissorRects[index] = rectangle;
}

//------------------------------------------------------------------------------
/**
*/
void 
D3D11RenderDevice::SetViewports()
{
	n_assert( 0 != this->d3d11Viewports.Size());
	d3d11DeviceContext->RSSetViewports(this->d3d11Viewports.Size(), &this->d3d11Viewports[0]);
}

//------------------------------------------------------------------------------
/**
*/
void 
D3D11RenderDevice::ApplyWireframe()
{
	this->d3d11DeviceContext->RSSetState(this->wireframeRasterizer);
}

//------------------------------------------------------------------------------
/**
*/
void 
D3D11RenderDevice::ApplyPassSettings()
{
	n_assert(this->inBeginPass);
	this->SetViewports();
	this->SetRenderTargets();
	this->SetScissorRects();
}

} // namespace CoreGraphics

