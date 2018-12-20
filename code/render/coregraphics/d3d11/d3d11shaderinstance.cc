//------------------------------------------------------------------------------
//  d3d11shaderinstance.cc
//  (C) 2012 Gustav Sterbrant
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/d3d11/d3d11shaderinstance.h"
#include "coregraphics/renderdevice.h"
#include "coregraphics/shader.h"
#include "coregraphics/shadervariable.h"
#include "coregraphics/shadervariation.h"
#include "coregraphics/shaderserver.h"
#include "resources/resourcemanager.h"
#include "util/blob.h"
#include <D3Dcompiler.h>

namespace Direct3D11
{
__ImplementClass(Direct3D11::D3D11ShaderInstance, 'D1SI', Base::ShaderInstanceBase);

using namespace Util;
using namespace CoreGraphics;

//------------------------------------------------------------------------------
/**
*/
D3D11ShaderInstance::D3D11ShaderInstance() :
	effect(0)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
D3D11ShaderInstance::~D3D11ShaderInstance()
{
	n_assert(0 == this->effect)
}

//------------------------------------------------------------------------------
/**
    This method is called by Shader::CreateInstance() to setup the 
    new shader instance.
*/
void
D3D11ShaderInstance::Setup(const Ptr<CoreGraphics::Shader>& origShader)
{
    n_assert(origShader.isvalid());
    n_assert(0 == this->effect);
    HRESULT hr;
    const Ptr<D3D11Shader>& d3d11Shader = origShader.upcast<D3D11Shader>();
    ID3D11Device* d3d11Device = D3D11RenderDevice::Instance()->GetDirect3DDevice();

    // call parent class
    ShaderInstanceBase::Setup(origShader);

	// clone incoming effect
	// hr = d3d11Shader->GetD3D11Effect()->CloneEffect(0, &this->effect);
	// n_assert(SUCCEEDED(hr));

	// copy effect pointer
	this->effect = d3d11Shader->GetD3D11Effect();

	// get description of effect
	D3DX11_EFFECT_DESC desc;
	hr = this->effect->GetDesc(&desc);
	n_assert(SUCCEEDED(hr));

	SizeT numTechs = desc.Techniques;
	IndexT techIndex;
	Array<String> usedVariables;
	for (techIndex = 0; techIndex < numTechs; techIndex++)
	{
		// get technique
		ID3DX11EffectTechnique* tech = this->effect->GetTechniqueByIndex(techIndex);

		// get pass
		ID3DX11EffectPass* pass = tech->GetPassByIndex(0);

		// create pass description
		D3DX11_PASS_SHADER_DESC passDesc;

		// get shaders
		pass->GetVertexShaderDesc(&passDesc);
		this->GetActiveVariables(passDesc, usedVariables);
		pass->GetPixelShaderDesc(&passDesc);
		this->GetActiveVariables(passDesc, usedVariables);
		pass->GetHullShaderDesc(&passDesc);
		this->GetActiveVariables(passDesc, usedVariables);
		pass->GetDomainShaderDesc(&passDesc);
		this->GetActiveVariables(passDesc, usedVariables);
		pass->GetGeometryShaderDesc(&passDesc);
		this->GetActiveVariables(passDesc, usedVariables);	
	}

	IndexT i;
	for (i = 0; i < usedVariables.Size(); i++)
	{
		// create new var
		Ptr<ShaderVariable> var = ShaderVariable::Create();

		// get shader variable
		ID3DX11EffectVariable* effectVar = this->effect->GetVariableByName(usedVariables[i].AsCharPtr());		

		// setup variable from effect
		var->Setup(effectVar);
		this->variables.Append(var);
		this->variablesByName.Add(var->GetName(), var);
		this->variablesBySemantic.Add(var->GetSemantic(), var);
	}

	for (i = 0; i < numTechs; i++)
	{
		// create new shader variation
		Ptr<ShaderVariation> var = ShaderVariation::Create();

		// get technique
		ID3DX11EffectTechnique* tech = this->effect->GetTechniqueByIndex(i);

		// setup variation from technique
		var->Setup(tech);
		this->variations.Add(var->GetFeatureMask(), var);
	}
	n_assert(this->variations.Size() > 0);

	// select first variation as default
	this->SelectActiveVariation(this->variations.KeyAtIndex(0));
}

//------------------------------------------------------------------------------
/**
*/
void 
D3D11ShaderInstance::Reload( const Ptr<CoreGraphics::Shader>& origShader )
{
	n_assert(origShader.isvalid());

	const Ptr<D3D11Shader>& d3d11Shader = origShader.upcast<D3D11Shader>();
	ID3D11Device* d3d11Device = D3D11RenderDevice::Instance()->GetDirect3DDevice();
}

//------------------------------------------------------------------------------
/**
*/
void
D3D11ShaderInstance::Cleanup()
{
	n_assert(0 != this->effect);
    ShaderInstanceBase::Cleanup();
	// this->effect->Release();
	this->effect = 0;
}

//------------------------------------------------------------------------------
/**
*/
bool
D3D11ShaderInstance::SelectActiveVariation(ShaderFeature::Mask featureMask)
{
    if (ShaderInstanceBase::SelectActiveVariation(featureMask))
	{
		ID3DX11EffectTechnique* tech = this->activeVariation->GetD3D11Technique();
		n_assert(tech->IsValid());
		return true;
	}
        
    return false;
}

//------------------------------------------------------------------------------
/**
*/
void
D3D11ShaderInstance::OnLostDevice()
{
	n_assert(0 != this->effect);
	ID3D11Device* device = D3D11RenderDevice::Instance()->GetDirect3DDevice();
    HRESULT hr = device->GetDeviceRemovedReason();
    n_assert(SUCCEEDED(hr));
}

//------------------------------------------------------------------------------
/**
*/
void
D3D11ShaderInstance::OnResetDevice()
{
	n_assert(0 != this->effect);
	ID3D11Device* device = D3D11RenderDevice::Instance()->GetDirect3DDevice();
	HRESULT hr = device->GetDeviceRemovedReason();
    n_assert(SUCCEEDED(hr));
}

//------------------------------------------------------------------------------
/**
*/
SizeT
D3D11ShaderInstance::Begin()
{
	n_assert(0 != this->effect);
    ShaderInstanceBase::Begin();
	return 1;    
}

//------------------------------------------------------------------------------
/**
*/
void
D3D11ShaderInstance::BeginPass(IndexT passIndex)
{
	n_assert(0 != this->effect);
    ShaderInstanceBase::BeginPass(passIndex);
	D3D11RenderDevice* renderDevice = D3D11RenderDevice::Instance();
	ID3D11DeviceContext* context = renderDevice->GetDirect3DDeviceContext();
	HRESULT hr = this->activeVariation->GetD3D11Technique()->GetPassByIndex(0)->Apply(0, context);
	bool tessellated = this->activeVariation->GetTessellated();
	renderDevice->SetTessellation(tessellated);
	n_assert(SUCCEEDED(hr));
}

//------------------------------------------------------------------------------
/**
*/
void
D3D11ShaderInstance::Commit()
{
	n_assert(0 != this->effect);
    ShaderInstanceBase::Commit(); 
	D3D11RenderDevice* renderDevice = D3D11RenderDevice::Instance();
	ID3D11DeviceContext* context = renderDevice->GetDirect3DDeviceContext();
	HRESULT hr = this->activeVariation->GetD3D11Technique()->GetPassByIndex(0)->Commit(context);	
	n_assert(SUCCEEDED(hr));
}

//------------------------------------------------------------------------------
/**
*/
void
D3D11ShaderInstance::EndPass()
{
	n_assert(0 != this->effect);
    ShaderInstanceBase::EndPass();
}

//------------------------------------------------------------------------------
/**
*/
void
D3D11ShaderInstance::End()
{
	n_assert(0 != this->effect);
    ShaderInstanceBase::End();
}

//------------------------------------------------------------------------------
/**
*/
void 
D3D11ShaderInstance::GetActiveVariables( const D3DX11_PASS_SHADER_DESC& passDesc, Util::Array<Util::String>& variables )
{
	// get shader variable
	ID3DX11EffectShaderVariable* effectVar = passDesc.pShaderVariable;

 	if(!effectVar->IsValid())
 	{
 		return;
 	}
	// get shader desc
	D3DX11_EFFECT_SHADER_DESC desc;
	HRESULT hr = effectVar->AsShader()->GetShaderDesc(0, &desc);

	if(desc.BytecodeLength == 0)
	{
		return;
	}

	// create reflection
	ID3D11ShaderReflection* reflection = 0;
	hr = D3DReflect(desc.pBytecode, desc.BytecodeLength, IID_ID3D11ShaderReflection, (void**)&reflection);
	if(!SUCCEEDED(hr))
	{
		n_assert(false);
		return;
	}
	//n_assert2(SUCCEEDED(hr), "Failed to reflect shader variables");	

	D3D11_SHADER_DESC shdDesc;
	reflection->GetDesc(&shdDesc);

	SizeT numBuffers = shdDesc.ConstantBuffers;
	IndexT i;

	for (i = 0; i < numBuffers; i++)
	{
		// get constant buffer
		ID3D11ShaderReflectionConstantBuffer* cb = reflection->GetConstantBufferByIndex(i);
		D3D11_SHADER_BUFFER_DESC cbDesc;
		hr = cb->GetDesc(&cbDesc);
		if (SUCCEEDED(hr))
		{
			uint i;
			for (i = 0; i < cbDesc.Variables; i++)
			{
				ID3D11ShaderReflectionVariable* var = cb->GetVariableByIndex(i);

				// get variable description
				D3D11_SHADER_VARIABLE_DESC varDesc;
				var->GetDesc(&varDesc);

				// if the variable is used, add it to list of used variables
				if (varDesc.uFlags & D3D_SVF_USED && variables.FindIndex(varDesc.Name) == InvalidIndex)
				{
					variables.Append(varDesc.Name);
				}
			}
		}
	}

	SizeT numResources = shdDesc.BoundResources;

	for (i = 0; i < numResources; i++)
	{
		// get resource
		D3D11_SHADER_INPUT_BIND_DESC bindDesc;
		reflection->GetResourceBindingDesc(i, &bindDesc);

		if (bindDesc.Type == D3D_SIT_TEXTURE && variables.FindIndex(bindDesc.Name) == InvalidIndex)
		{
			variables.Append(bindDesc.Name);
		}
	}

}

} // namespace Direct3D11

