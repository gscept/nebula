#pragma once
//------------------------------------------------------------------------------
/**
    @class Direct3D11::D3D11RenderDevice
  
    Implements a RenderDevice on top of Direct3D11.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/    
//------------------------------------------------------------------------------
#include "coregraphics/base/renderdevicebase.h"
#include "coregraphics/pixelformat.h"
#include "coregraphics/imagefileformat.h"
#include "coregraphics/shaderstate.h"

namespace Direct3D11
{

#define NEBULA_USE_FLOAT_DEPTHBUFFER 0
#if NEBULA_USE_FLOAT_DEPTHBUFFER
    #define DepthFormat DXGI_FORMAT_D24_UNORM_S8_UINT
    #define ViewportMinZ 1
    #define ViewportMaxZ 0
    #define COREGRAPHICS_STANDARDCLEARDEPTHVALUE 0.0f
#else
    #define DepthFormat DXGI_FORMAT_D24_UNORM_S8_UINT
    #define ViewportMinZ 0
    #define ViewportMaxZ 1
    #define COREGRAPHICS_STANDARDCLEARDEPTHVALUE 1.0f
#endif

class D3D11RenderDevice : public Base::RenderDeviceBase
{
    __DeclareClass(D3D11RenderDevice);
    __DeclareSingleton(D3D11RenderDevice);
public:
    /// constructor
    D3D11RenderDevice();
    /// destructor
    virtual ~D3D11RenderDevice();

    /// get pointer to the d3d device
    ID3D11Device* GetDirect3DDevice() const;
	/// get pointer to the d3d11 device context
	ID3D11DeviceContext* GetDirect3DDeviceContext() const;
	/// get the directx 11 swap chain
	IDXGISwapChain* GetDXGISwapChain() const;
	/// get the feature level used by the directX device
	D3D_FEATURE_LEVEL GetDirect3DFeatureLevel() const;

    /// open the device
    bool Open();
    /// close the device
    void Close();

    /// begin complete frame
    bool BeginFrame();
    /// set the current vertex stream source
    void SetStreamSource(IndexT streamIndex, const Ptr<CoreGraphics::VertexBuffer>& vb, IndexT offsetVertexIndex);
    /// set current vertex layout
    void SetVertexLayout(const Ptr<CoreGraphics::VertexLayout>& vl);
    /// set current index buffer
    void SetIndexBuffer(const Ptr<CoreGraphics::IndexBuffer>& ib);
	/// add multiple render target
	void AddRenderTarget(ID3D11RenderTargetView* renderTargetView);
	/// sets depth-stencil target
	void SetDepthStencilTarget(ID3D11DepthStencilView* depthStencilView);
    /// draw current primitives
    void Draw();
    /// draw indexed, instanced primitives (see method header for details)
    void DrawIndexedInstanced(SizeT numInstances);
	/// begin a pass with an MRT
	void BeginPass(const Ptr<CoreGraphics::MultipleRenderTarget>& mrt, const Ptr<CoreGraphics::ShaderInstance>& passShader);
	/// begin a pass with a single RT
	void BeginPass(const Ptr<CoreGraphics::RenderTarget>& rt, const Ptr<CoreGraphics::ShaderInstance>& passShader);
    /// end current pass
    void EndPass();
    /// end complete frame
    void EndFrame();
    /// present the rendered scene
    void Present();
	/// adds a render viewport
	void AddViewport(const Math::rectangle<int>& rect);
	/// sets the render viewports
	void SetViewports();
	/// adds a scissor rect
	void SetScissorRect(const Math::rectangle<int>& rect, int index);

    /// save a screenshot to the provided stream
    CoreGraphics::ImageFileFormat::Code SaveScreenshot(CoreGraphics::ImageFileFormat::Code fmt, const Ptr<IO::Stream>& outStream);
    /// get the present parameters
    D3DPRESENT_PARAMETERS GetPresentParams() const;
	/// gets the shader for this pass
	Ptr<CoreGraphics::ShaderInstance> GetPassShader() const;
	/// sets the shader for this pass
	void SetPassShader(const Ptr<CoreGraphics::ShaderInstance>& passShader);
	/// sets the primitive group
	void SetPrimitiveGroup(const CoreGraphics::PrimitiveGroup& pg);
	/// sets whether or not the render device should tessellate
	void SetTessellation(bool state);
	/// gets whether or not the render device should tessellate
	bool GetTessellation();
	/// applies pre-render variables (performance issue if called to frequently)
	void ApplyPassSettings();

	/// applies wireframe state
	void ApplyWireframe();

	/// resizes swap chain
	void DisplayResized();

	static const short MaxNumRenderTargets = 8;

private:
	// give shape renderer, and text renderer exclusive access to set viewports and render targets
	friend class D3D11ShapeRenderer;
	friend class D3D11TextRenderer;

    /// close Direct3D
    void CloseDirect3D();
    /// open the Direct3D device
    bool OpenDirect3DDevice();
    /// close the Direct3D device
    void CloseDirect3DDevice();
    /// setup the requested adapter for the Direct3D device
    void SetupAdapter();
    /// select the requested buffer formats for the Direct3D device
    void SetupBufferFormats();
    /// setup the remaining presentation parameters
    void SetupPresentParams();
    /// set the initial Direct3D device state
    void SetInitialDeviceState();
    /// test for and handle lost device 
    bool TestResetDevice();
	
	/// sets scissor rects
	void SetScissorRects();
	/// sets the active render targets
	void SetRenderTargets();

    /// unbind D3D resources in the device
    void UnbindD3D11Resources();
    /// sync with gpu
    void SyncGPU();

	Util::Array<ID3D11RenderTargetView*> renderTargets;
	Util::Array<D3D11_VIEWPORT> d3d11Viewports;
	Util::FixedArray<D3D11_RECT> d3d11ScissorRects;
	ID3D11DepthStencilView* depthStencilView;

	static ID3D11Device* d3d11Device;
	static ID3D11DeviceContext* d3d11DeviceContext;
	static IDXGISwapChain* d3d11SwapChain;

	ID3D11RasterizerState* wireframeRasterizer;

	/// you need to have setup a backbuffer view for your backbuffer in order to set it as a render target
	static ID3D11RenderTargetView* backBufferView;

	D3D11_VIEWPORT dx11ViewPort;
	D3D_FEATURE_LEVEL usedFeatureLevel;
	DXGI_SWAP_CHAIN_DESC d3d11SwapChainDesc;

	UINT vertexByteSize[MaxNumVertexStreams];
	UINT vertexByteOffset[MaxNumVertexStreams];
	ID3D11Buffer* vertexBuffers[MaxNumVertexStreams];

    UINT adapter;
    DXGI_FORMAT displayFormat;
    DWORD deviceBehaviourFlags;
    static const int numSyncQueries = 1;
    uint frameId;
	bool tessellate;
	ID3D11Query* gpuSyncQuery[numSyncQueries];       
};



//------------------------------------------------------------------------------
/**
*/
inline Ptr<CoreGraphics::ShaderInstance>
D3D11RenderDevice::GetPassShader() const
{
	return this->passShader;
}

//------------------------------------------------------------------------------
/**
*/
inline void
D3D11RenderDevice::SetPassShader(const Ptr<CoreGraphics::ShaderInstance>& passShader)
{
	this->passShader = passShader;
}

//------------------------------------------------------------------------------
/**
*/
inline bool 
D3D11RenderDevice::GetTessellation()
{
	return this->tessellate;
}
} // namespace Direct3D11
//------------------------------------------------------------------------------
