#pragma once
//------------------------------------------------------------------------------
/**
	@class Material::MaterialServer
    
	Server object for the material subsystem. Factory for Materials.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "core/singleton.h"
#include "resources/resourceid.h"
#include "materials/materialpalette.h"
#include "materials/material.h"
#include "materials/surfacename.h"
#include "util/dictionary.h"
#include "util/array.h"

//------------------------------------------------------------------------------
namespace Materials
{

class MaterialServer : public Core::RefCounted
{
	__DeclareClass(MaterialServer);
	__DeclareSingleton(MaterialServer);
public:

	/// constructor
	MaterialServer();
	/// destructor
	virtual ~MaterialServer();
	/// open the material server (loads all materials)
	bool Open();
	/// close the material server
	void Close();
	/// return if server is open
	bool IsOpen() const;

    /// returns true if material exists
    bool HasMaterial(const Resources::ResourceName& name);
	/// get material by name
	const Ptr<Material>& GetMaterialByName(const Resources::ResourceName& name);
    /// get all materials, creates new array with materials
    Util::Array<Ptr<Material>> GetMaterials() const;
	/// get material codes by type
	const Util::Array<Ptr<Material> >& GetMaterialsByBatchGroup(const Graphics::BatchGroup::Code& type);
	/// returns true if we have any materials by type
    const bool HasMaterialsByBatchGroup(const Graphics::BatchGroup::Code& type);
	/// add a material to the server
	void AddMaterial(const Ptr<Material>& material);

	/// convert a shader feature string into a feature bit mask
	Materials::MaterialFeature::Mask FeatureStringToMask(const Util::String& str);
	/// convert shader feature bit mask into string
	Util::String FeatureMaskToString(Materials::MaterialFeature::Mask mask);

	/// gain access to a material palette by name, will be loaded if it isn't already
	const Ptr<MaterialPalette>& LookupMaterialPalette(const Resources::ResourceName& name);
	
private:
    friend class MaterialType;
    friend class SurfaceName;

	/// load material palette
	void LoadMaterialPalette(const Resources::ResourceName& name);

    MaterialType materialTypeRegistry;
    SurfaceName surfaceNameRegistry;
	MaterialFeature materialFeature;
	Util::Dictionary<Resources::ResourceName, Ptr<Material>> materials;
	Util::Dictionary<Graphics::BatchGroup::Code, Util::Array<Ptr<Material>>> materialsByBatchGroup;
	Util::Dictionary<Resources::ResourceName, Ptr<MaterialPalette>> materialPalettes;
	bool isOpen;
};

//------------------------------------------------------------------------------
/**
*/
inline bool 
MaterialServer::HasMaterial( const Resources::ResourceName& name )
{
    return this->materials.Contains(name);
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<Material>&
MaterialServer::GetMaterialByName(const Resources::ResourceName& name)
{
    n_assert(this->materials.Contains(name));
    return this->materials[name];
}

//------------------------------------------------------------------------------
/**
*/
inline Util::Array<Ptr<Material>>
MaterialServer::GetMaterials() const
{
    return this->materials.ValuesAsArray();
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<Ptr<Material> >& 
MaterialServer::GetMaterialsByBatchGroup(const Graphics::BatchGroup::Code& type)
{
	n_assert(this->materialsByBatchGroup.Contains(type));
	return this->materialsByBatchGroup[type];
}

//------------------------------------------------------------------------------
/**
*/
inline const bool 
MaterialServer::HasMaterialsByBatchGroup(const Graphics::BatchGroup::Code& type)
{
	return this->materialsByBatchGroup.Contains(type);
}

}

