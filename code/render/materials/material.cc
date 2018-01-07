//------------------------------------------------------------------------------
//  material.cc
//  (C) 2011-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "materials/material.h"
#include "materials/materialinstance.h"
#include "surface.h"
#include "coregraphics/shaderpool.h"

namespace Materials
{
__ImplementClass(Materials::Material, 'MATR', Resources::Resource);

//------------------------------------------------------------------------------
/**
*/
Material::Material() :
	isVirtual(false)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
Material::~Material()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
Material::Setup()
{
	n_assert(this->passesByIndex.IsEmpty());
	n_assert(this->passesByBatchGroup.IsEmpty());
}

//------------------------------------------------------------------------------
/**
	Load inherited by walking through all shaders, parameters and features, add if non existent, or replace if inherited material defines it again
*/
void
Material::LoadInherited(const Ptr<Material>& material)
{
	IndexT i;

	SizeT numPasses = this->GetNumPasses();
	// add/replace shaders
	for (i = 0; i < numPasses; i++)
	{
		const MaterialPass& pass = material->passesByIndex[i];
		MaterialPass newPass{ pass.code, pass.shader, pass.featureMask };
		const Graphics::BatchGroup::Code& key = pass.code;
		const CoreGraphics::ShaderId value = pass.shader;
		IndexT i = this->passesByIndex.FindIndex(newPass);
		if (i == InvalidIndex)
		{
			this->passesByBatchGroup.Add(newPass.code, Util::Array<MaterialPass>());
		}
		this->passesByBatchGroup[newPass.code].Append(newPass);
		this->passesByIndex.Append(newPass);
	}

	// add/replace parameters
	for (i = 0; i < material->parametersByName.Size(); i++)
	{
		const Util::StringAtom& key = material->parametersByName.KeyAtIndex(i);
		const MaterialParameter& value = material->parametersByName.ValueAtIndex(i);
		if (this->parametersByName.Contains(key))
		{
			this->parametersByName[key] = value;
		}
		else
		{
			this->parametersByName.Add(key, value);
		}
	}

	this->inheritedMaterials.Append(material);
}

//------------------------------------------------------------------------------
/**
*/
void
Material::Unload()
{
	this->passesByIndex.Clear();
	this->passesByBatchGroup.Clear();
	this->parametersByName.Clear();
	this->inheritedMaterials.Clear();
}

//------------------------------------------------------------------------------
/**
*/
void 
Material::Discard()
{
	this->inheritedMaterials.Clear();
	this->Unload();
}

//------------------------------------------------------------------------------
/**
*/
void 
Material::AddPass(const Graphics::BatchGroup::Code& code, const CoreGraphics::ShaderId shader, const CoreGraphics::ShaderFeature::Mask& mask)
{
	n_assert(shader != Ids::InvalidId64);

	// create pass
	MaterialPass pass{ code, shader, mask, this->passesByIndex.Size() };

    // if this pass is already defined and this material inherits another, remove the previous definition and override
	if (this->inheritedMaterials.Size() > 0)
	{
		if (this->passesByBatchGroup.Contains(code))
		{
			// yes, copy array since we want to maintain the order
			Util::Array<MaterialPass> passes = this->passesByBatchGroup[code];
			IndexT i;
			for (i = 0; i < passes.Size(); i++)
			{
				if (passes[i] == pass)
				{
					this->passesByBatchGroup[code].EraseIndex(this->passesByBatchGroup[code].FindIndex(passes[i]));
					this->passesByIndex.EraseIndex(this->passesByIndex.FindIndex(passes[i]));
				}				
			}
		}
	}

	// add pass
	this->passesByIndex.Append(pass);
	if (!this->passesByBatchGroup.Contains(code))
	{
		this->passesByBatchGroup.Add(code, Util::Array<MaterialPass>());
	}
	this->passesByBatchGroup[code].Append(pass);
}

//------------------------------------------------------------------------------
/**
*/
void
Material::AddParam(const Util::String& name, const Material::MaterialParameter& param)
{
    n_assert(!name.IsEmpty());

    // if this parameter is already defined and this material inherits another, remove the previous definition and override
	if (this->inheritedMaterials.Size() > 0)
    {
        if (this->parametersByName.Contains(name))
        {
            this->parametersByName.Erase(name);
        }
    }

    // add parameter
    n_assert(!this->parametersByName.Contains(name));
    this->parametersByName.Add(name, param);
}

//------------------------------------------------------------------------------
/**
*/
void
Material::AddSurface(const Ptr<Surface>& sur)
{
    n_assert(this->surfaces.FindIndex(sur) == InvalidIndex);
    this->surfaces.Append(sur);
}

//------------------------------------------------------------------------------
/**
*/
void
Material::RemoveSurface(const Ptr<Surface>& sur)
{
    IndexT index = this->surfaces.FindIndex(sur);
    n_assert(index != InvalidIndex);
    this->surfaces.EraseIndex(index);
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<Ptr<Surface>>&
Material::GetSurfaces() const
{
    return this->surfaces;
}

//------------------------------------------------------------------------------
/**
*/
const Material::MaterialPass&
Material::GetPassByIndex(const IndexT index) const
{
	return this->passesByIndex[index];
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<Material::MaterialPass>&
Material::GetPassesByCode(const Graphics::BatchGroup::Code& code)
{
	return this->passesByBatchGroup[code];
}

//------------------------------------------------------------------------------
/**
*/
void
Material::Reload()
{
	// IMPLEMENT ME, should reload all passes and variables, then do the same for the surfaces.
}

//------------------------------------------------------------------------------
/**
*/
void
Material::Reload(const CoreGraphics::ShaderId shader)
{
	IndexT i;
	for (i = 0; i < this->passesByIndex.Size(); i++)
	{
		MaterialPass& pass = this->passesByIndex[i];
		const Util::StringAtom& lhs = CoreGraphics::shaderPool->GetName(pass.shader.id24);
		const Util::StringAtom& rhs = CoreGraphics::shaderPool->GetName(shader.id24);
		if (lhs == rhs)
		{
			pass.shader = shader;
		}
	}

	// go through and reload all surfaces
	for (i = 0; i < this->surfaces.Size(); i++)
	{
		this->surfaces[i]->Reload();
	}
}



} // namespace Materials