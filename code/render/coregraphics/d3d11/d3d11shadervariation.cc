//------------------------------------------------------------------------------
//  d3d11shadervariation.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/d3d11/d3d11shadervariation.h"
#include "coregraphics/shaderserver.h"
#include "coregraphics/renderdevice.h"

namespace Direct3D11
{
__ImplementClass(Direct3D11::D3D11ShaderVariation, 'D1SV', Base::ShaderVariationBase);

using namespace CoreGraphics;

//------------------------------------------------------------------------------
/**
*/
D3D11ShaderVariation::D3D11ShaderVariation() :
	technique(0),
	tessellated(false)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
D3D11ShaderVariation::~D3D11ShaderVariation()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void 
D3D11ShaderVariation::Setup( ID3DX11EffectTechnique* tech )
{
	n_assert(0 != tech);
	this->technique = tech;

	// get description of technique
	D3DX11_TECHNIQUE_DESC desc;
	tech->GetDesc(&desc);
	this->SetName(desc.Name);

	ID3DX11EffectVariable* annotationVar = tech->GetAnnotationByName("Mask");
	ID3DX11EffectStringVariable* annotationString = annotationVar->AsString();
	if (annotationString->IsValid())
	{
		LPCTSTR string;
		annotationString->GetString(&string);
		this->SetFeatureMask(ShaderServer::Instance()->FeatureStringToMask(string));
		this->tessellated = (this->featureMask & ShaderServer::Instance()->FeatureStringToMask("Tessellated")) != 0;
	}
	else
	{
		this->SetFeatureMask(ShaderServer::Instance()->FeatureStringToMask("Default"));
	}

	// get pass
	ID3DX11EffectPass* pass = tech->GetPassByIndex(0);
	n_assert(0 != pass);

	this->SetNumPasses(1);
}
} // namespace Direct3D11

