//------------------------------------------------------------------------------
//  d3d11vertexlayout.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/d3d11/d3d11vertexlayout.h"
#include "coregraphics/d3d11/d3d11types.h"
#include "coregraphics/renderdevice.h"
#include "coregraphics/shaderserver.h"
#include "resources/resourceid.h"

namespace Direct3D11
{
__ImplementClass(Direct3D11::D3D11VertexLayout, 'D1VL', Base::VertexLayoutBase);

using namespace CoreGraphics;
using namespace Util;

//------------------------------------------------------------------------------
/**
*/
D3D11VertexLayout::D3D11VertexLayout()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
D3D11VertexLayout::~D3D11VertexLayout()
{
    n_assert(this->inputLayouts.IsEmpty());
}

//------------------------------------------------------------------------------
/**
*/
void
D3D11VertexLayout::Setup(const Array<VertexComponent>& c)
{
    n_assert(this->inputLayouts.IsEmpty());

    // call parent class
    Base::VertexLayoutBase::Setup(c);

	ID3D11Device* device = D3D11RenderDevice::Instance()->GetDirect3DDevice();
	ID3D11DeviceContext* context = D3D11RenderDevice::Instance()->GetDirect3DDeviceContext();

    // create a D3D11 vertex declaration object
    n_assert(this->components.Size() < maxElements);
    IndexT curOffset[RenderDevice::MaxNumVertexStreams] = { 0 };
    	
    for (compIndex = 0; compIndex < this->components.Size(); compIndex++)
    {
		const VertexComponent& component = this->components[compIndex];
		
		Util::String signature = component.GetSignature();
		semanticName[compIndex] = D3D11Types::SemanticNameAsD3D11Format(component.GetSemanticName(), component.GetSemanticIndex());
		decl[compIndex].SemanticName = semanticName[compIndex].AsCharPtr();
		decl[compIndex].SemanticIndex = component.GetSemanticIndex(); 
		
		decl[compIndex].AlignedByteOffset = curOffset[component.GetStreamIndex()];
		
		// first stream must be mesh, the others are arbitrary per-instance streams
		if (component.GetStreamIndex() <= 0)
		{
			decl[compIndex].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
			decl[compIndex].InstanceDataStepRate = 0;
		}
		else
		{
			decl[compIndex].InputSlotClass = D3D11_INPUT_PER_INSTANCE_DATA;
			decl[compIndex].InstanceDataStepRate = 1;
		}
		
		decl[compIndex].Format = D3D11Types::AsD3D11VertexDeclarationType(component.GetFormat());
		decl[compIndex].InputSlot = component.GetStreamIndex();
		curOffset[component.GetStreamIndex()] += component.GetByteSize();
    }

}

//------------------------------------------------------------------------------
/**
*/
void
D3D11VertexLayout::Discard()
{
	if (this->inputLayouts.IsEmpty())
	{
		n_warning("Removed empty vertex layout! This was probably just used as a reference for a particle\n");
	}
	else
	{
		IndexT i;
		for (i = 0; i < this->inputLayouts.Size(); i++)
		{
			this->inputLayouts.ValueAtIndex(i)->Release();
		}
		this->inputLayouts.Clear();
	}	

    VertexLayoutBase::Discard();
}

//------------------------------------------------------------------------------
/**
*/
ID3D11InputLayout*
D3D11VertexLayout::GetD3D11VertexDeclaration()
{
	// ugly, but hey, we need it!
	Ptr<CoreGraphics::ShaderInstance> instance = ShaderServer::Instance()->GetActiveShaderInstance();
	if (this->inputLayouts.Contains(instance))
	{
		return this->inputLayouts[instance];
	}
	else
	{
		this->CreateVertexLayout(instance);
		n_assert(this->inputLayouts.Contains(instance));
		return this->inputLayouts[instance];
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
D3D11VertexLayout::CreateVertexLayout(const Ptr<CoreGraphics::ShaderInstance>& inst)
{
	n_assert(!this->inputLayouts.Contains(inst));

	ID3D11Device* device = D3D11RenderDevice::Instance()->GetDirect3DDevice();
	HRESULT hr;

	// create pointer to layout
	ID3D11InputLayout* layout;

	// gets the active shader, will only succeed if there is no pre-effect (that is, will only work for post-effects)
	Ptr<CoreGraphics::ShaderVariation> variation = inst->GetActiveVariation();
	ID3DX11EffectTechnique* technique = variation->GetD3D11Technique();
	ID3DX11EffectPass* pass = technique->GetPassByIndex(0);

	D3DX11_PASS_DESC desc;
	pass->GetDesc(&desc);

	// this should succeed!
	hr = device->CreateInputLayout(decl, compIndex, desc.pIAInputSignature, desc.IAInputSignatureSize, &layout);

	// ugly fallback, if we fail to create a layout, we simply use the static shader and create a layout from that shader instead
	if (FAILED(hr))
	{
		Ptr<CoreGraphics::ShaderInstance> instance = ShaderServer::Instance()->GetShader("shd:static")->GetAllShaderInstances()[0];
		Ptr<CoreGraphics::ShaderVariation> variation = instance->GetVariationByIndex(0);
		ID3DX11EffectTechnique* technique = variation->GetD3D11Technique();
		ID3DX11EffectPass* pass = technique->GetPassByIndex(0);

		D3DX11_PASS_DESC desc;
		pass->GetDesc(&desc);

		// this must succeed!
		hr = device->CreateInputLayout(decl, compIndex, desc.pIAInputSignature, desc.IAInputSignatureSize, &layout);
		n_assert(SUCCEEDED(hr));
		this->inputLayouts.Add(inst, layout);
	}
	else
	{
		this->inputLayouts.Add(inst, layout);
	}

}
} // namespace Direct3D11
