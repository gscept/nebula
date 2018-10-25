#pragma once
//------------------------------------------------------------------------------
/**
    @class Direct3D11::D3D11ShaderVariation

    Under Direct3D11, a shader variation is represented by an d3dx effect 
    technique which must be annotated by a FeatureMask string.

    (C) 2011-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "coregraphics/base/shadervariationbase.h"

namespace Direct3D11
{
class D3D11ShaderVariation : public Base::ShaderVariationBase
{
    __DeclareClass(D3D11ShaderVariation);
public:
    /// constructor
    D3D11ShaderVariation();
    /// destructor
    virtual ~D3D11ShaderVariation();
	/// get the d3d11 blend state
	ID3DX11EffectTechnique* GetD3D11Technique() const;

	/// get tessellation bool
	const bool GetTessellated() const;

private:
	friend class D3D11ShaderInstance;

	/// sets up variation from technique
	void Setup(ID3DX11EffectTechnique* tech);

	ID3DX11EffectTechnique* technique;
	bool tessellated;	
};

//------------------------------------------------------------------------------
/**
*/
inline ID3DX11EffectTechnique* 
D3D11ShaderVariation::GetD3D11Technique() const
{
	return this->technique;
}

//------------------------------------------------------------------------------
/**
*/
inline const bool 
D3D11ShaderVariation::GetTessellated() const
{
	return this->tessellated;
}

} // namespace Direct3D11
//------------------------------------------------------------------------------
