#pragma once
//------------------------------------------------------------------------------
/**
    @class Physics::BaseJoint

    A joint (also known as constraint) connects two Actors    

    (C) 2012-2016 Johannes Hirche, LTU Skelleftea
*/
#include "core/refcounted.h"
#include "core/ptr.h"
#include "physics/physicsbody.h"

//------------------------------------------------------------------------------
namespace Physics
{

class BaseJoint : public Core::RefCounted
{
    __DeclareAbstractClass(BaseJoint);
public:
    

    /// default constructor
    BaseJoint();
       
    /// get the joint type
    JointType GetType() const;
    /// update position and orientation
    virtual void UpdateTransform(const Math::matrix44& m) = 0;
    /// render debug visualization
    virtual void RenderDebug();

	/// attach to scene
	virtual void Attach(Physics::BaseScene * world) = 0;
	/// detach from scene
	virtual void Detach() = 0;
	/// is attached
    virtual bool IsAttached() const;
	/// set enable
	virtual void SetEnabled(bool b) = 0;
	/// is enabled
	bool IsEnabled() const;
    /// get the first body
    const Ptr<PhysicsBody> & GetBody1() const;
	/// get the second body (can be null)
    const Ptr<PhysicsBody> & GetBody2() const;
    /// set link name (for linking to a character joint)
    void SetLinkName(const Util::String& n);
    /// get link name
    const Util::String& GetLinkName();
    /// return true if a link name has been set
    bool IsLinkValid() const;
    /// set link index
    void SetLinkIndex(int i);
    /// get link index
    int GetLinkIndex() const;
	void SetType(Physics::JointType  t);	
	/// set the 2 bodies connected by the joint (0 pointers are valid)
	void SetBodies(const Ptr<PhysicsBody> & body1, const Ptr<PhysicsBody> & body2);	

	virtual void SetBreakThreshold(float threshold) = 0;
	virtual float GetBreakThreshold() = 0;

	virtual void SetERP(float ERP, int axis = 0 ) = 0;
	virtual void SetCFM(float CFM, int axis = 0) = 0;

	virtual void SetStoppingERP(float ERP, int axis = 0) = 0;
	virtual void SetStoppingCFM(float CFM, int axis = 0) = 0;

	virtual float GetERP(int axis = 0) = 0;
	virtual float GetCFM(int axis = 0) = 0;

	virtual float GetStoppingERP(int axis = 0) = 0;
	virtual float GetStoppingCFM(int axis = 0) = 0;

protected:

    
    Physics::JointType type;    
    Ptr<PhysicsBody> rigidBody1;
    Ptr<PhysicsBody> rigidBody2;
    Util::String linkName;
    int linkIndex;
    bool isAttached;	
	friend PhysicsBody;
	JointDescription common;	
};

//------------------------------------------------------------------------------
/**
    Get the joint type.

    @return     the joint type
*/
inline
Physics::JointType
BaseJoint::GetType() const
{
    return this->type;
}

//------------------------------------------------------------------------------
/**
    Set optional link name. This is usually the name of a Nebula2
    character joint (for implementing ragdolls).
*/
inline
void
BaseJoint::SetLinkName(const Util::String& n)
{
    this->linkName = n;
}

//------------------------------------------------------------------------------
/**
    Get optional link name.
*/
inline
const Util::String& 
BaseJoint::GetLinkName()
{
    return this->linkName;
}

//------------------------------------------------------------------------------
/**
    Set link index. This is the link name converted to some index, for
    instance a joint index.
*/
inline
void
BaseJoint::SetLinkIndex(int i)
{
    this->linkIndex = i;
}

//------------------------------------------------------------------------------
/**
    Get link index.
*/
inline
int
BaseJoint::GetLinkIndex() const
{
    return this->linkIndex;
}

//------------------------------------------------------------------------------
/**
    Return true if this joint has a link name set.
*/
inline
bool
BaseJoint::IsLinkValid() const
{
    return this->linkName.IsValid();
}

inline
bool
BaseJoint::IsAttached() const
{
	return this->isAttached;
}

}; // namespace Physics
//------------------------------------------------------------------------------
