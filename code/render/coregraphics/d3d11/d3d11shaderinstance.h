#pragma once
//------------------------------------------------------------------------------
/**
    @class Direct3D11::D3D11ShaderInstance
    
    D3D11 implementation of CoreGraphics::ShaderInstance.
    
    @todo lost/reset device handling

    (C) 2007 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "coregraphics/base/shaderstatebase.h"
#include "coregraphics/shaderfeature.h"


namespace Direct3D11
{
	
class D3D11ShaderInstance : public Base::ShaderInstanceBase
{
    __DeclareClass(D3D11ShaderInstance);
public:
    /// constructor
    D3D11ShaderInstance();
    /// destructor
    virtual ~D3D11ShaderInstance();    
    /// select active variation by feature mask
    bool SelectActiveVariation(CoreGraphics::ShaderFeature::Mask featureMask);
    /// begin rendering through the currently selected variation, returns no. passes
    SizeT Begin();
    /// begin pass
    void BeginPass(IndexT passIndex);
    /// commit changes before rendering
    void Commit();
    /// end pass
    void EndPass();
    /// end rendering through variation
    void End();
	/// setup static texture bindings
	void SetupSharedTextures();

	/// returns true if shader instance needs to refresh constant buffers
	bool IsDirty() const;
	/// set the dirty flag for this shader instance
	void SetDirty(bool b);

protected:
    friend class Base::ShaderBase;
    friend class D3D11Shader;

    /// setup the shader instance from its original shader object
    virtual void Setup(const Ptr<CoreGraphics::Shader>& origShader);
	/// reload the shader instance from original shader object
	virtual void Reload(const Ptr<CoreGraphics::Shader>& origShader);

    /// cleanup the shader instance
    virtual void Cleanup();

    /// called by d3d11 shader server when d3d11 device is lost
    void OnLostDevice();
    /// called by d3d11 shader server when d3d11 device is reset
    void OnResetDevice();

	/// gets all used variables from a shader
	void GetActiveVariables(const D3DX11_PASS_SHADER_DESC& passDesc, Util::Array<Util::String>& variables);


	ID3DX11Effect* effect;
};

} // namespace Direct3D11
//------------------------------------------------------------------------------

    