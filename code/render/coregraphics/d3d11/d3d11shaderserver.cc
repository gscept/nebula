//------------------------------------------------------------------------------
//  d3d11shaderserver.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "stdneb.h"
#include "coregraphics/streamshaderloader.h"
#include "coregraphics/d3d11/d3d11shaderserver.h"
#include "materials/materialserver.h"

namespace Direct3D11
{
__ImplementClass(Direct3D11::D3D11ShaderServer, 'D1SS', Base::ShaderServerBase);
__ImplementSingleton(Direct3D11::D3D11ShaderServer);

using namespace Resources;
using namespace CoreGraphics;

//------------------------------------------------------------------------------
/**
*/
D3D11ShaderServer::D3D11ShaderServer()
{
    __ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
D3D11ShaderServer::~D3D11ShaderServer()
{
    if (this->IsOpen())
    {
        this->Close();
    }
    __DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
bool
D3D11ShaderServer::Open()
{
    n_assert(!this->IsOpen());

    ShaderServerBase::Open();

	if (this->HasShader(ResourceId("shd:static")))
	{
		this->sharedVariableShaderInst = this->CreateShaderInstance(ResourceId("shd:static"));
		this->SetActiveShaderInstance(this->sharedVariableShaderInst);
		n_assert(this->sharedVariableShaderInst.isvalid());
	}

    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
D3D11ShaderServer::Close()
{
    n_assert(this->IsOpen());
    ShaderServerBase::Close();
}

//------------------------------------------------------------------------------
/**
*/
Ptr<CoreGraphics::ShaderInstance> 
D3D11ShaderServer::GetSharedShader()
{
	return this->sharedVariableShaderInst;	
}

//------------------------------------------------------------------------------
/**
	Must be called from within Shader
*/
void 
D3D11ShaderServer::ReloadShader( Ptr<CoreGraphics::Shader> shader )
{
	n_assert(0 != shader);
	shader->SetLoader(StreamShaderLoader::Create());
	shader->SetAsyncEnabled(false);
	shader->Load();
	if (shader->IsLoaded())
	{
		shader->SetLoader(0);
	}
	else
	{
		n_error("Failed to load shader '%s'!", shader->GetResourceId());
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
D3D11ShaderServer::LoadShader( const Resources::ResourceId& shdName )
{
	n_assert(shdName.IsValid());
	Ptr<Shader> shader = Shader::Create();
	shader->SetResourceId(shdName);
	shader->SetLoader(StreamShaderLoader::Create());
	shader->SetAsyncEnabled(false);
	shader->Load();
	if (shader->IsLoaded())
	{
		shader->SetLoader(0);
		this->shaders.Add(shdName, shader);
	}
	else
	{
		n_warning("Failed to explicitly load shader '%s'!", shdName);
		shader->Unload();
	}
}
} // namespace Direct3D11

