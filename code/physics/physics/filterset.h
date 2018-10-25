#pragma once
//------------------------------------------------------------------------------
/**
    @class PhysicsFeature::GameFilterSet

    A filter set object allows to define conditions which define a set of
    physics entities. It is used to include or exclude physics entities
    from various tests (like stabbing checks).
    
	(C) 2012-2018 Individual contributors, see AUTHORS file
*/
#include "util/array.h"
#include "model/templates.h"

//------------------------------------------------------------------------------
namespace Physics
{
class PhysicsObject;

class FilterSet
{
public:
    /// constructor
    FilterSet();
    /// destructor
    virtual ~FilterSet();
    /// clear the filter set
    virtual void Clear();    
    /// add a rigid body unique id to the set
    void AddRigidBodyId(uint id);
    /// access to rigid body id's
    const Util::Array<uint>& GetRigidBodyIds() const;
    /// add a material type to the set
    void AddMaterialType(uint t);
    /// access to material id's
    const Util::Array<uint>& GetMaterialTypes() const;
    /// check a object for exlusion
    virtual bool CheckObject(const PhysicsObject * object) const;	
    /// set collide category bit mask (combination of CollideCategory bits)
    void SetCollideBits(uint bitMask);
    /// get collide category bit mask
    uint GetCollideBits() const;
    
protected:
	/// return true if rigid body id is in set
	bool CheckRigidBodyId(uint id) const;
	/// return true if material type is in set
	bool CheckMaterialType(uint t) const;

private:
    Util::Array<uint> rigidBodyIds;
    Util::Array<uint> materialTypes;
    uint collideBits;
};

//------------------------------------------------------------------------------
/**
*/
inline
FilterSet::FilterSet() :
    collideBits((uint)None)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
FilterSet::~FilterSet()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline void
FilterSet::Clear()
{    
    this->rigidBodyIds.Clear();
    this->materialTypes.Clear();
    this->collideBits = (uint)None;
}

//------------------------------------------------------------------------------
/**
*/
inline void
FilterSet::AddRigidBodyId(uint id)
{
    this->rigidBodyIds.Append(id);
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<uint>&
FilterSet::GetRigidBodyIds() const
{
    return this->rigidBodyIds;
}

//------------------------------------------------------------------------------
/**
*/
inline void
FilterSet::AddMaterialType(uint t)
{
    this->materialTypes.Append(t);
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<uint>&
FilterSet::GetMaterialTypes() const
{
    return this->materialTypes;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
FilterSet::CheckRigidBodyId(uint id) const
{
    return (this->rigidBodyIds.Find(id) != 0);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
FilterSet::CheckMaterialType(uint id) const
{
    return (this->materialTypes.Find(id) != 0);
}

//------------------------------------------------------------------------------
/**
*/
inline void
FilterSet::SetCollideBits(uint bitMask)
{
    this->collideBits = bitMask;
}

//------------------------------------------------------------------------------
/**
*/
inline uint
FilterSet::GetCollideBits() const
{
    return this->collideBits;
}

} // namespace Physics
//------------------------------------------------------------------------------