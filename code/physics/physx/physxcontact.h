#pragma once
//------------------------------------------------------------------------------
/**
    @class PhysX::Contact

    Contact points with position and Face-Up vector.

    (C) 2016 Individual contributors, see AUTHORS file
*/
#include "math/vector.h"
#include "physics/materialtable.h"
#include "util/array.h"
#include "core/refcounted.h"

//------------------------------------------------------------------------------
namespace Physics
{
class PhysicsObject;

class BaseContact : public Core::RefCounted
{
	__DeclareClass(BaseContact);
public:
	
    enum Type
    {
        StaticType,             // Contact between static and dynamic objects
        DynamicType,            // Contact between dynamic and dynamic objects

        RayCheck,               // Contact caused through a raycheck

        NumTypes,
        InvalidType
    };

    /// Construct.
    BaseContact();

	~BaseContact();

    /// clear content
    void Clear();

	/// convenience functions - returns the first element in the array
	void SetNormalVector(const Math::vector & p);
	const Math::vector & GetNormalVector() const;
	void SetPoint(const Math::point & p);
	const Math::point & GetPoint() const;

    /// set positions 
	void SetPoints(const Util::Array<Math::point> & v);
    /// get positions
    const Util::Array<Math::point> & GetPoints() const;

    /// set up vector to `v'
    void SetUpVector(const Math::vector& v);
    /// get up vector
    const Math::vector& GetUpVector() const;

	/// set normal vector to `v'
	void SetNormalVectors(const Util::Array<Math::vector>& v);
	/// get normal vector
	const Util::Array<Math::vector> & GetNormalVectors() const;

    /// get maximum penetration depth
    float GetDepth() const;
    /// set penetration depth
    void SetDepth(float d);
    
	/// set the exact object of ours that collided with the other
	void SetOwnerObject(const Ptr<PhysicsObject> & b);
	/// set the exact object of ours that collided with the other
	const Ptr<Physics::PhysicsObject>& GetOwnerObject() const;

    /// set the object we collided with
    void SetCollisionObject(const Ptr<Physics::PhysicsObject> & b);    
    /// get the object we collided with
    const Ptr<Physics::PhysicsObject> & GetCollisionObject() const;

    /// Set material of contact
    void SetMaterial(MaterialType material);
    /// get material of contact
    MaterialType GetMaterial() const;   

    /// set the type of the contact point
    void SetType(Type type);
    /// get the type
    const Type& GetType() const;

protected:
    Math::vector upVector;
    float depth;    
	Ptr<Physics::PhysicsObject> object, ownerObject;
    MaterialType material;
    Type type;
	Util::Array<Math::point> positions;
	Util::Array<Math::vector> normals;
};

//------------------------------------------------------------------------------
/**
*/
inline 
void 
BaseContact::SetPoints(const Util::Array<Math::point> &v)
{
    positions = v;
}

//------------------------------------------------------------------------------
/**
*/
inline 
void 
BaseContact::SetPoint(const Math::point &v)
{
	n_assert2(positions.Size()==0,"can only set one point");
	positions.Append(v);
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::point & 
BaseContact::GetPoint() const
{
	return positions[0];
}

//------------------------------------------------------------------------------
/**
*/
inline 
void 
BaseContact::SetNormalVector(const Math::vector &v)
{
	n_assert2(normals.Size()==0,"can only set one point");
	normals.Append(v);
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::vector & 
BaseContact::GetNormalVector() const
{
	return normals[0];
}

//------------------------------------------------------------------------------
/**
*/
inline 
const Util::Array<Math::point> & 
BaseContact::GetPoints() const
{
    return positions;
}


//------------------------------------------------------------------------------
/**
*/
inline 
void 
BaseContact::SetNormalVectors(const Util::Array<Math::vector> & v)
{
	normals = v;
}

//------------------------------------------------------------------------------
/**
*/
inline 
const Util::Array<Math::vector> &
BaseContact::GetNormalVectors() const
{
	return normals;
}
//------------------------------------------------------------------------------
/**
*/
inline 
void 
BaseContact::SetType(Type type)
{
    this->type = type;
}

//------------------------------------------------------------------------------
/**
*/
inline 
const BaseContact::Type& 
BaseContact::GetType() const
{
    return this->type;
}

//------------------------------------------------------------------------------
/**
*/
inline 
void 
BaseContact::SetUpVector(const Math::vector& v)
{
    upVector = v;
}

//------------------------------------------------------------------------------
/**
*/
inline 
const Math::vector& 
BaseContact::GetUpVector() const
{
    return upVector;
}

//------------------------------------------------------------------------------
/**
*/
inline
float
BaseContact::GetDepth() const
{
    return this->depth;    
}

//------------------------------------------------------------------------------
/**
*/
inline
void
BaseContact::SetDepth(float d)
{
    this->depth = d;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
BaseContact::SetMaterial(MaterialType mat)
{
    this->material = mat;
}

//------------------------------------------------------------------------------
/**
*/
inline
MaterialType
BaseContact::GetMaterial() const
{
    return this->material;
}

} // namespace Physics
//------------------------------------------------------------------------------
