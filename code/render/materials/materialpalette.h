#pragma once
//------------------------------------------------------------------------------
/**
    @class Material::MaterialPalette
    
    Holds a material palette which in turn holds one or several materials
    
    (C) 2011-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "resources/resource.h"
#include "util/array.h"
#include "materials/material.h"

//------------------------------------------------------------------------------
namespace Materials
{
class MaterialPalette : public Core::RefCounted
{
	__DeclareClass(MaterialPalette);
public:
	/// constructor
	MaterialPalette();
	/// destructor
	virtual ~MaterialPalette();

	/// discards the palette and its materials
	void Discard();

	/// set the name of the frame shader
	void SetName(const Resources::ResourceName& id);
	/// get the name of the frame shader
	const Resources::ResourceName& GetName() const;
	/// add material
	void AddMaterial(const Ptr<Material>& material);
	/// get material by index
	const Ptr<Material>& GetMaterialByIndex(const IndexT index) const;
	/// get material by name
	Ptr<Material> GetMaterialByName(const Resources::ResourceName& resource) const;
	/// returns true if palette contains material
	bool HasMaterial(const Resources::ResourceName& resource) const;
	/// get the list of materials
	const Util::Array<Ptr<Material> >& GetMaterials() const;
private:
	Util::Array<Ptr<Material> > materials;
	Util::Dictionary<Resources::ResourceName, Ptr<Material> > materialsByName;

	Resources::ResourceName name;
};

//------------------------------------------------------------------------------
/**
*/
inline void
MaterialPalette::SetName(const Resources::ResourceName& resId)
{
	this->name = resId;
}

//------------------------------------------------------------------------------
/**
*/
inline const Resources::ResourceName&
MaterialPalette::GetName() const
{
	return this->name;
}

//------------------------------------------------------------------------------
/**
*/
inline void
MaterialPalette::AddMaterial(const Ptr<Material>& material)
{
	this->materials.Append(material);
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<Material>&
MaterialPalette::GetMaterialByIndex(const IndexT index) const
{
	return this->materials[index];
}

//------------------------------------------------------------------------------
/**
*/
inline Ptr<Material>
MaterialPalette::GetMaterialByName(const Resources::ResourceName& resource) const
{
	for (int i = 0; i < this->materials.Size(); i++)
	{
		if (this->materials[i]->GetName() == resource)
			return this->materials[i];
	}
	n_error("There is no material with name %s", resource.Value());
	return 0;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
MaterialPalette::HasMaterial(const Resources::ResourceName& resource) const
{
	for (int i = 0; i < this->materials.Size(); i++)
	{
		if (this->materials[i]->GetName() == resource) return true;
	}
	return false;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<Ptr<Material> >& 
MaterialPalette::GetMaterials() const
{
	return this->materials;
}

}  // namespace Materials
//------------------------------------------------------------------------------