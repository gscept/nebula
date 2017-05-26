//------------------------------------------------------------------------------
//  surfaceinstance.cc
//  (C) 2015-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "surfaceinstance.h"
#include "material.h"
#include "surface.h"
#include "resources/resourcemanager.h"
#include "coregraphics/shaderstate.h"
#include "coregraphics/config.h"
#include "surfaceconstant.h"

using namespace CoreGraphics;
namespace Materials
{
__ImplementClass(Materials::SurfaceInstance, 'SUIN', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
SurfaceInstance::SurfaceInstance() :
    originalSurface(0)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
SurfaceInstance::~SurfaceInstance()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
SurfaceInstance::Setup(const Ptr<Surface>& surface)
{
	n_assert(this->originalSurface == 0);

    // set original surface
    this->originalSurface = surface;

    // create temporary dictionary mapping between shaders and their variables
    Util::Dictionary<Util::StringAtom, Util::Array<Ptr<CoreGraphics::ShaderState>>> variableToShaderMap;
    Util::Dictionary<Util::StringAtom, Util::Array<Graphics::BatchGroup::Code>> variableToCodeMap;
	Util::Dictionary<Util::StringAtom, Util::Array<Material::MaterialPass>> variableToPassMap;
    const Ptr<Material>& materialTemplate = this->originalSurface->materialTemplate;

    // get parameters from material
    const Util::Dictionary<Util::StringAtom, Material::MaterialParameter>& parameters = materialTemplate->GetParameters();
    this->code = this->originalSurface->code;

    SizeT numPasses = materialTemplate->GetNumPasses();
    IndexT passIndex;
    for (passIndex = 0; passIndex < numPasses; passIndex++)
    {
        // get indexed data from material
		const Material::MaterialPass& pass = materialTemplate->GetPassByIndex(passIndex);
        const Graphics::BatchGroup::Code& code = pass.code;
        const Ptr<Shader>& shader = pass.shader;

		// only add instance once for a shader
		Ptr<ShaderState> shdInst;
		IndexT index = this->shaderInstancesByShader.FindIndex(shader);
		//if (index == InvalidIndex)
		{
			// if no shader instance exists, create state and add to dictionary
			shdInst = shader->CreateState({ NEBULAT_DEFAULT_GROUP });
			this->shaderInstancesByShader.Add(shader, shdInst);
		}
		/*
		else
		{
			// fetch state
			shdInst = this->shaderInstancesByShader.ValueAtIndex(index);
		}
		*/

		// go through our materials parameter list and set them up
		IndexT paramIndex;
		for (paramIndex = 0; paramIndex < parameters.Size(); paramIndex++)
		{
			// get parameter name
			const Util::StringAtom& paramName = parameters.KeyAtIndex(paramIndex);
			const Material::MaterialParameter& param = parameters.ValueAtIndex(paramIndex);

			// add to dictionary of parameter
			if (!variableToShaderMap.Contains(paramName)) variableToShaderMap.Add(paramName, Util::Array<Ptr<CoreGraphics::ShaderState>>());
			variableToShaderMap[paramName].Append(shdInst);

			if (!variableToCodeMap.Contains(paramName)) variableToCodeMap.Add(paramName, Util::Array<Graphics::BatchGroup::Code>());
			variableToCodeMap[paramName].Append(code);

			if (!variableToPassMap.Contains(paramName)) variableToPassMap.Add(paramName, Util::Array<Material::MaterialPass>());
			variableToPassMap[paramName].Append(pass);
		}

		// add to dictionary
		this->shaderInstances.Append(shdInst);
    }

    // go through mappings
    IndexT mappingIndex;
	for (mappingIndex = 0; mappingIndex < variableToPassMap.Size(); mappingIndex++)
    {
		const Util::StringAtom& paramName = variableToPassMap.KeyAtIndex(mappingIndex);
		const Util::Array<Material::MaterialPass>& passes = variableToPassMap.ValueAtIndex(mappingIndex);
        const Util::Array<Ptr<CoreGraphics::ShaderState>>& shaders = variableToShaderMap.ValueAtIndex(mappingIndex);
        //const Util::Array<Graphics::BatchGroup::Code>& codes = variableToCodeMap.ValueAtIndex(mappingIndex);

        // create a new multi-shader variable container (or surface constant)
        Ptr<SurfaceConstant> constant = SurfaceConstant::Create();

        // get parameter by name (which is used for its default value)
        const Material::MaterialParameter& param = parameters[paramName];

        // get the value defined in the surface resource
        Surface::SurfaceValueBinding& val = this->originalSurface->staticValues[paramName];
        
        // specially handle default values which are strings
        if (val.value.GetType() == Util::Variant::String)
        {
            Ptr<Resources::ManagedTexture> tex = Resources::ResourceManager::Instance()->CreateManagedResource(CoreGraphics::Texture::RTTI, val.value.GetString() + NEBULA3_TEXTURE_EXTENSION, NULL, true).downcast<Resources::ManagedTexture>();
            //this->SetTexture(paramName, tex);
            val.value.SetType(Util::Variant::Object);
            val.value.SetObject(tex->GetTexture());
			this->allocatedTextures.Append(tex);
        }

        // setup constant
        constant->SetValue(val.value);
		constant->Setup(paramName, passes, shaders);
        constant->system = param.system;

        // add constants to variable lists
        this->constants.Append(constant);
        this->constantsByName.Add(paramName, constant);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
SurfaceInstance::Discard()
{
	IndexT i;
	for (i = 0; i < this->allocatedTextures.Size(); i++)
	{
		Resources::ResourceManager::Instance()->DiscardManagedResource(this->allocatedTextures[i].upcast<Resources::ManagedResource>());
	}
	this->allocatedTextures.Clear();
    this->originalSurface->DiscardInstance(this);
    this->originalSurface = 0;
}

//------------------------------------------------------------------------------
/**
*/
void
SurfaceInstance::Cleanup()
{
    IndexT i;
	for (i = 0; i < this->constants.Size(); i++)
	{
		this->constants[i]->Discard();
	}

    for (i = 0; i < this->shaderInstances.Size(); i++)
    {
        this->shaderInstances[i]->Discard();
    }
    this->shaderInstances.Clear();
    this->shaderInstancesByShader.Clear();

    this->constants.Clear();
    this->constantsByName.Clear();
	this->managedTextures.Clear();
	this->originalSurface = 0;

	this->code = Materials::SurfaceName::InvalidSurfaceName;
}

//------------------------------------------------------------------------------
/**
*/
const Ptr<CoreGraphics::ShaderState>&
SurfaceInstance::GetShaderState(const IndexT passIndex)
{
    return this->shaderInstances[passIndex];
}

//------------------------------------------------------------------------------
/**
*/
void
SurfaceInstance::SetValue(const Util::StringAtom& param, const Util::Variant& value)
{
    n_assert(param.IsValid());
    n_assert(this->constantsByName.Contains(param));
    this->constantsByName[param]->SetValue(value);
}

//------------------------------------------------------------------------------
/**
*/
void
SurfaceInstance::SetTexture(const Util::StringAtom& param, const Ptr<CoreGraphics::Texture>& tex)
{
    n_assert(param.IsValid());
    n_assert(this->constantsByName.Contains(param));
    this->constantsByName[param]->SetTexture(tex);
}

//------------------------------------------------------------------------------
/**
*/
void
SurfaceInstance::SetTexture(const Util::StringAtom& param, const Ptr<Resources::ManagedTexture>& tex)
{
    n_assert(param.IsValid());
    DeferredTextureBinding obj;
    obj.tex = tex;
    obj.var = param;
    this->managedTextures.Append(obj);
}

//------------------------------------------------------------------------------
/**
*/
void
SurfaceInstance::Apply(const IndexT passIndex)
{
    IndexT i;

    // 'touch' textures which may not have been loaded when the material was first set up
    for (i = 0; i < this->managedTextures.Size(); i++)
    {
        const DeferredTextureBinding& bind = this->managedTextures[i];
        if (!bind.tex->IsPlaceholder())
        {
            this->constantsByName[bind.var]->SetTexture(bind.tex->GetTexture());
            this->managedTextures.EraseIndex(i);
            i--;
        }
    }

	const Ptr<CoreGraphics::ShaderState>& shdInst = this->shaderInstances[passIndex];
    for (i = 0; i < this->constants.Size(); i++)
    {
		this->constants[i]->Apply(passIndex);
    }

    // get shader instance by code
    //this->shaderInstancesByCode[group]->Apply();
	shdInst->Commit();
}

} // namespace Materials