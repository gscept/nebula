#pragma once
//------------------------------------------------------------------------------
/**
    @class Materials::Material
    
    Describes a material, which is a collection of shaders. 
	A material has a set of variables, which can be instanced, and then applied, 
	which in turn applies it to all the shaders having that variable.

	As such, we can effectively instantiate a material, apply a variable, 
	and the variable will be activated for that entity when the material is applied.
    
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------

#include "core/refcounted.h"
#include "coregraphics/shader.h"
#include "util/array.h"
#include "util/keyvaluepair.h"
#include "util/variant.h"
#include "util/stringatom.h"
#include "materials/materialtype.h"
#include "graphics/batchgroup.h"
#include "materialfeature.h"

namespace Materials
{
class Surface;
class MaterialInstance;
class Material : public Core::RefCounted
{
	__DeclareClass(Material);
public:

	struct MaterialParameter
	{
		enum EditType
		{
			EditRaw,
			EditColor,

			NumEditTypes
		};

		static EditType EditTypeFromString(const Util::String& str)
		{
			if (str == "raw") return EditRaw;
			else if (str == "color") return EditColor;
			else return EditRaw;
		}

		Util::String name;
        Util::String desc;
		Util::Variant defaultVal;
		Util::Variant min;
		Util::Variant max;        
		EditType editType;
		bool system;
	};

	struct MaterialPass
	{
		Graphics::BatchGroup::Code code;
		Ptr<CoreGraphics::Shader> shader;
		CoreGraphics::ShaderFeature::Mask featureMask;
		IndexT index;

		const bool operator==(const MaterialPass& rhs) const
		{
			return this->code == rhs.code && this->shader == rhs.shader && this->featureMask == rhs.featureMask && this->index == rhs.index;
		}
	};

	/// constructor-
	Material();
	/// destructor
	virtual ~Material();

	/// sets the name for the material
	void SetName(const Resources::ResourceName& name);
	/// gets the name for the material
	const Resources::ResourceName& GetName() const;
	/// set the group this material belongs to
	void SetGroup(const Util::StringAtom& group);
	/// get the group this material belongs to
	const Util::StringAtom& GetGroup() const;
	/// set description
	void SetDescription(const Util::String& description);
	/// get description
	const Util::String& GetDescription() const;
	/// sets the type of the material
	void SetFeatures(const MaterialFeature::Mask& type);
	/// gets the type of the material
	const MaterialFeature::Mask& GetFeatures() const;
	/// sets the batch type
	void SetCode(const Materials::MaterialType::Code& code);
	/// get the batch type
    const Materials::MaterialType::Code& GetCode() const;
	/// set the material to be virtual
	void SetVirtual(bool b);
	/// get if the material is virtual
	const bool GetVirtual() const;

	/// setup material from list of list of shaders with variations
	void Setup();
	/// cleans up the material
	void Unload();	
	/// discards the material and all its instances
	void Discard();
	/// reloads entire material
	void Reload();
	/// reloads material passes related to shader
	void Reload(const Ptr<CoreGraphics::Shader>& shader);

	/// add a shader to the material
	void AddPass(const Graphics::BatchGroup::Code& code, const Ptr<CoreGraphics::Shader>& shader, const CoreGraphics::ShaderFeature::Mask& mask);
	/// get pass by index
	const MaterialPass& GetPassByIndex(const IndexT index) const;
	/// get pass list by code
	const Util::Array<MaterialPass>& GetPassesByCode(const Graphics::BatchGroup::Code& code);
	/// get the amount of passes used by this material
	SizeT GetNumPasses();

    /// add a surface to this material
    void AddSurface(const Ptr<Surface>& sur);
    /// remove a surface material from this material
    void RemoveSurface(const Ptr<Surface>& sur);
    /// get surfaces
    const Util::Array<Ptr<Surface>>& GetSurfaces() const;
	/// returns true if surface is already registered

    /// adds parameter
    void AddParam(const Util::String& name, const Material::MaterialParameter& param);

	/// gets the number of parameters
	const SizeT GetNumParameters() const;
	/// gets the dictionary of parameters and their default values
	const Util::Dictionary<Util::StringAtom, MaterialParameter>& GetParameters() const;

private:
	friend class MaterialLoader;

    /// load material inherited from another
    void LoadInherited(const Ptr<Material>& material);

	bool isVirtual;
	Util::Array<Ptr<Material>> inheritedMaterials;
	Resources::ResourceName name;
	Util::StringAtom group;
	Util::String description;
	MaterialFeature::Mask type;
    Materials::MaterialType::Code code;
	Util::Dictionary<Util::StringAtom, MaterialParameter> parametersByName;
	Util::Array<MaterialPass> passesByIndex;
	Util::Dictionary<Graphics::BatchGroup::Code, Util::Array<MaterialPass>> passesByBatchGroup;
    Util::Array<Ptr<Surface>> surfaces;
	Util::Dictionary<Util::StringAtom, Ptr<Surface>> surfacesByName;
}; 

//------------------------------------------------------------------------------
/**
*/
inline void
Material::SetName(const Resources::ResourceName& name)
{
	this->name = name;
}

//------------------------------------------------------------------------------
/**
*/
inline const Resources::ResourceName&
Material::GetName() const
{
	return this->name;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Material::SetGroup(const Util::StringAtom& group)
{
	this->group = group;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::StringAtom&
Material::GetGroup() const
{
	return this->group;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Material::SetDescription(const Util::String& description)
{
	this->description = description;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String& 
Material::GetDescription() const
{
	return this->description;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Material::SetFeatures(const Materials::MaterialFeature::Mask& type)
{
	this->type = type;
}

//------------------------------------------------------------------------------
/**
*/
inline const Materials::MaterialFeature::Mask& 
Material::GetFeatures() const
{
	return this->type;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Material::SetCode(const Materials::MaterialType::Code& code)
{
	this->code = code;
}

//------------------------------------------------------------------------------
/**
*/
inline const Materials::MaterialType::Code&
Material::GetCode() const
{
	return this->code;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Material::SetVirtual(bool b)
{
	this->isVirtual = b;
}

//------------------------------------------------------------------------------
/**
*/
inline const bool
Material::GetVirtual() const
{
	return this->isVirtual;
}

//------------------------------------------------------------------------------
/**
*/
inline const SizeT 
Material::GetNumParameters() const
{
	return this->parametersByName.Size();
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Dictionary<Util::StringAtom, Material::MaterialParameter>& 
Material::GetParameters() const
{
	return this->parametersByName;
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT 
Material::GetNumPasses()
{
	return this->passesByIndex.Size();
}

} // namespace Materials
//------------------------------------------------------------------------------