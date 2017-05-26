//------------------------------------------------------------------------------
//  D3D11Shader.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/d3d11/d3d11shader.h"
#include "coregraphics/shaderinstance.h"
#include "coregraphics/shader.h"
#include "coregraphics/renderdevice.h"
#include "coregraphics/shaderserver.h"
#include "coregraphics/vertexcomponent.h"
#include <D3Dcompiler.h>

namespace Direct3D11
{
__ImplementClass(Direct3D11::D3D11Shader, 'D1SD', Base::ShaderBase);

using namespace CoreGraphics;

//------------------------------------------------------------------------------
/**
*/
D3D11Shader::D3D11Shader()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
D3D11Shader::~D3D11Shader()
{
    if (this->IsLoaded())
    {
        this->Unload();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
D3D11Shader::Unload()
{
	n_assert(0 == this->shaderInstances.Size());
	this->Cleanup();
    ShaderBase::Unload();
}

//------------------------------------------------------------------------------
/**
*/
void 
D3D11Shader::Cleanup()
{
	// remove instances
	this->shaderInstances.Clear();
	this->effect->Release();
	this->effect = 0;
}


//------------------------------------------------------------------------------
/**
*/
void
D3D11Shader::OnLostDevice()
{
	ID3D11Device* device = RenderDevice::Instance()->GetDirect3DDevice();
	HRESULT hr = device->GetDeviceRemovedReason();
    n_assert(SUCCEEDED(hr));

    // notify our instances
    IndexT i;
    for (i = 0; i < this->shaderInstances.Size(); i++)
    {
        this->shaderInstances[i].downcast<D3D11ShaderInstance>()->OnLostDevice();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
D3D11Shader::OnResetDevice()
{

	ID3D11Device* device = RenderDevice::Instance()->GetDirect3DDevice();
	HRESULT hr = device->GetDeviceRemovedReason();
    n_assert(SUCCEEDED(hr));

    // notify our instances
    IndexT i;
    for (i = 0; i < this->shaderInstances.Size(); i++)
    {
        this->shaderInstances[i].downcast<D3D11ShaderInstance>()->OnResetDevice();
    }

}

//------------------------------------------------------------------------------
/**
*/
void 
D3D11Shader::Reload()
{
	this->Cleanup();
	Ptr<ShaderBase> thisPtr(this);
	ShaderBase::Unload();
	ShaderServer::Instance()->ReloadShader(thisPtr.downcast<Shader>());
	for (int i = 0; i < this->shaderInstances.Size(); i++)
	{
		this->shaderInstances[i]->Cleanup();
		this->shaderInstances[i]->Reload(thisPtr.downcast<Shader>());
	}
}


} // namespace Direct3D11

