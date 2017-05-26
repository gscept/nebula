//------------------------------------------------------------------------------
//  materialinstance.cc
//  (C) 2011-2013 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "util/stringatom.h"
#include "util/array.h"
#include "materials/materialinstance.h"
#include "materials/material.h"
#include "frame/framepassbase.h"
#include "coregraphics/texture.h"
#include "resources/resourcemanager.h"
#include "resources/managedtexture.h"

using namespace Resources;
using namespace Util;
using namespace CoreGraphics;

namespace Materials
{
__ImplementClass(Materials::MaterialInstance, 'MATI', Core::RefCounted);


//------------------------------------------------------------------------------
/**
*/
MaterialInstance::MaterialInstance()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
MaterialInstance::~MaterialInstance()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void 
MaterialInstance::Setup( const Ptr<Material>& origMaterial )
{
	n_assert(!this->IsValid());
	this->originalMaterial = origMaterial;

	// get parameters from material
	const Util::Dictionary<Util::StringAtom, Material::MaterialParameter>& parameters = originalMaterial->GetParameters();

	// find shader variables based on name from several shaders
	Dictionary<StringAtom, Array<Ptr<ShaderVariable>>> shaderVariableMap;
	SizeT numPasses = this->originalMaterial->GetNumPasses();
	for (int i = 0; i < numPasses; i++)
	{
		// get indexed data from material
		const Models::BatchType::Code& code = this->originalMaterial->GetBatchGroup(i);
		const Ptr<Shader>& shader = this->originalMaterial->GetShaderInstance(code);

		// create an instance of the shader for the material
		const Ptr<ShaderState>& shdInst = shader->CreateState();

		// go through our material parameters
		IndexT j;
		for (j = 0; j < parameters.Size(); j++)
		{
			// get parameter name
			StringAtom paramName = parameters.KeyAtIndex(j);

			// get variable with equal name from shader
			if (shdInst->HasVariableBySemantic(paramName))
			{
				// get variable
				const Ptr<ShaderVariable>& var = shdInst->GetVariableBySemantic(paramName);

				if (!shaderVariableMap.Contains(paramName))
				{
					shaderVariableMap.Add(paramName, Array<Ptr<ShaderVariable>>());
				}
				shaderVariableMap[paramName].Append(var);
			}
		}

		this->shaderInstancesByCode.Add(code, shdInst);
		this->shaderInstances.Append(shdInst);
	}

	// create material variable from shader variables
	for (int i = 0; i < shaderVariableMap.Size(); i++)
	{
		Ptr<MaterialVariable> var = MaterialVariable::Create();
		StringAtom varName = shaderVariableMap.KeyAtIndex(i);
		var->Setup(shaderVariableMap.ValueAtIndex(i), parameters[varName].defaultVal);
		this->materialVariables.Append(var);
		this->materialVariablesByName.Add(shaderVariableMap.KeyAtIndex(i), var);
	}

}


//------------------------------------------------------------------------------
/**
	Go through and apply all variables
*/
void 
MaterialInstance::Apply()
{
	IndexT i;
	for (i = 0; i < this->materialVariables.Size(); i++)
	{
		this->materialVariables[i]->Apply();
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
MaterialInstance::Cleanup()
{
	n_assert(this->IsValid());
	for (int i = 0; i < this->shaderInstances.Size(); i++)
	{
		this->shaderInstances[i]->Discard();
	}
	this->shaderInstances.Clear();
	this->shaderInstancesByCode.Clear();
	for (int i = 0; i < this->materialVariables.Size(); i++)
	{
		this->materialVariables[i]->Cleanup();
	}
	this->materialVariables.Clear();
	this->materialVariablesByName.Clear();
	this->originalMaterial = 0;
}

//------------------------------------------------------------------------------
/**
*/
void 
MaterialInstance::Discard()
{
	n_assert(this->IsValid());
	this->originalMaterial->DiscardMaterialInstance(this);
}

//------------------------------------------------------------------------------
/**
*/
bool 
MaterialInstance::IsValid() const
{
	return this->originalMaterial.isvalid();
}

//------------------------------------------------------------------------------
/**
*/
const Ptr<MaterialVariable>&
MaterialInstance::GetVariableByName(const MaterialVariable::Name& n) const
{
#if NEBULA3_DEBUG
	if (!this->HasVariableByName(n))
	{
		n_error("Invalid shader variable name '%s' in shader '%s'",
			n.Value(), this->originalMaterial->GetName().Value());
	}
#endif
	return this->materialVariablesByName[n];
}

//------------------------------------------------------------------------------
/**
*/
const Ptr<CoreGraphics::ShaderState>&
MaterialInstance::GetShaderStateByIndex(IndexT i) const
{
	n_assert(this->shaderInstances.Size() > i);
	return this->shaderInstances[i];
}

//------------------------------------------------------------------------------
/**
*/
const Ptr<CoreGraphics::ShaderState>&
MaterialInstance::GetShaderStateByCode(const Models::BatchType::Code& code)
{
	n_assert(this->shaderInstancesByCode.Contains(code));
	return this->shaderInstancesByCode[code];
}

//------------------------------------------------------------------------------
/**
*/
const SizeT 
MaterialInstance::GetNumShaderInstances() const
{
	return this->shaderInstances.Size();
}

} // namespace Materials