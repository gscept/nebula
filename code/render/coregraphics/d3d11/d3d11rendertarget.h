#pragma once
//------------------------------------------------------------------------------
/**
    @class Direct3D11::D3D11RenderDevice
  
    D3D11 implementation of RenderTarget.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/    
//------------------------------------------------------------------------------
#include "coregraphics/base/rendertargetbase.h"
#include "coregraphics/shadervariable.h"


namespace Direct3D11
{
class D3D11RenderTarget : public Base::RenderTargetBase
{
    __DeclareClass(D3D11RenderTarget);
public:
    /// constructor
    D3D11RenderTarget();
    /// destructor
    virtual ~D3D11RenderTarget();
    
    /// setup the render target object
    void Setup();
    /// discard the render target object
    void Discard();
    /// begin a render pass
    void BeginPass();
    /// end current render pass
    void EndPass();
    /// generate mipmap levels
    void GenerateMipLevels();
	
	/// called when the display changes size
	void OnDisplayResized();

	/// clears render target by force
	void Clear();

	/// returns render target view
	ID3D11RenderTargetView* GetD3D11RenderTargetView() const;

protected:
    friend class D3D11RenderDevice;

    /// setup compatible multisample type
    void SetupMultiSampleType();
                                      
	Ptr<CoreGraphics::ShaderVariable> textureRatio;
    Ptr<CoreGraphics::ShaderVariable> sharedPixelSize; 
    Ptr<CoreGraphics::ShaderVariable> sharedHalfPixelSize;

	/// the actual render target texture
	ID3D11Texture2D* d3d11ResolveTexture;
	ID3D11Texture2D* d3d11CPUResolveTexture;

	ID3D11RenderTargetView* d3d11ResolveTextureRenderView;
	ID3D11RenderTargetView* d3d11CPUResolveTextureRenderView;
	ID3D11RenderTargetView* d3d11BackBufferRenderView;

    DXGI_SAMPLE_DESC d3d11MultiSampleDescription;
    DXGI_FORMAT d3d11ColorBufferFormat;
    bool needsResolve;
};



//------------------------------------------------------------------------------
/**
*/
inline ID3D11RenderTargetView* 
D3D11RenderTarget::GetD3D11RenderTargetView() const
{
	return this->d3d11ResolveTextureRenderView;
}


} // namespace Direct3D11
//------------------------------------------------------------------------------
