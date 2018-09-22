#ifndef PHYSICS_MATERIALTABLE_H
#define PHYSICS_MATERIALTABLE_H
//------------------------------------------------------------------------------
/**
    @class Physics::MaterialTable

    Contains material properties and friction coefficients.
    
	(C) 2003 RadonLabs GmbH
	(C) 2012-2016 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "util/string.h"
#include "util/fixedarray.h"
#include "util/fixedtable.h"
#include "util/stringatom.h"
#include "util/hashtable.h"
#include "io/uri.h"

//------------------------------------------------------------------------------
namespace Physics
{
typedef int MaterialType;
const MaterialType InvalidMaterial = -1;

class MaterialTable
{
public:

    struct Material
    {
        Material() : name(), friction(0.0f), restitution(0.0f) {}
        Util::StringAtom name;
        float friction;
        float restitution;
    };
    struct Interaction
    {
        Interaction() {}
        Util::StringAtom collEvent;
    };

    /// initialize material table
    static void Setup();
	
    /// translate material type to string
	static const Util::StringAtom&  MaterialTypeToString(MaterialType t);
    /// translate string to material type
	static MaterialType StringToMaterialType(const Util::StringAtom& str);
   

    /// get friction coefficient for a material
    static float GetFriction(MaterialType t0);
    /// get restitution for a material
	static float GetRestitution(MaterialType t0);
    /// get collision event for two materials
	static const Util::StringAtom& GetCollisionEvent(MaterialType t0, MaterialType t1);

    /// set friction of a material type
    static void SetFriction(MaterialType t0, float friction);
    /// set restitution of a material type
    static void SetRestitution(MaterialType t0, float bouncyness);	

    /// toolkit helpers

    /// get list of all available materials (slow)
    static Util::Array<Util::StringAtom> GetMaterials();
	/// save to file
	static void SaveMaterialTable(const IO::URI & file);
    /// get copy material array (slow)
    static Util::Array<Material> GetMaterialTable();
    /// get copy of interaction table (slow)
    static Util::Dictionary<Util::String, Util::Dictionary<Util::String, Util::String>> GetInteractionTable();


private:
    /// constructor
    MaterialTable();
    /// destructor
    ~MaterialTable();
    
    /// add material properties. t must be within size constraints
    static void AddMaterial(MaterialType t, const Util::StringAtom& name, float friction, float restitution);

    /// add interaction
    static void AddInteraction(MaterialType t0, MaterialType t1, const Util::StringAtom& event, bool symmetric = true);
   

	static Util::HashTable<Util::StringAtom, MaterialType> materialsHash;
	static Util::StringAtom invalidTypeString;
    static Util::FixedArray<Material> materials;
	static Util::FixedTable<Interaction> interactions;
};


//------------------------------------------------------------------------------
/**
*/
inline float
MaterialTable::GetFriction(MaterialType t0)
{
	n_assert(t0 >= 0 && t0 < materials.Size());
	return materials[t0].friction;
}

//------------------------------------------------------------------------------
/**
*/
inline float
MaterialTable::GetRestitution(MaterialType t0)
{
	n_assert(t0 >= 0 && t0 < materials.Size());
	return materials[t0].restitution;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::StringAtom&
MaterialTable::GetCollisionEvent(MaterialType t0, MaterialType t1)
{
	n_assert(t0 >= 0 && t0 < materials.Size());
	n_assert(t1 >= 0 && t1 < materials.Size());
	return interactions.At(t0, t1).collEvent;
}

}; // namespace Physics
//------------------------------------------------------------------------------
#endif
    
