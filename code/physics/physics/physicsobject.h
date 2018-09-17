#pragma once
//------------------------------------------------------------------------------
/**
    @class Physics::PhysicsObject

	Abstract class for most physics-object classes to derive from.
        
	(C) 2012-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "math/matrix44.h"
#include "physics/collider.h"
#include "core/weakptr.h"
#include "physics/materialtable.h"
#include "physics/contact.h"
#include "physics/model/templates.h"


//------------------------------------------------------------------------------

namespace Physics
{

class BaseScene;
class Collider;
class PhysicsObject;

class PhysicsUserData : public Core::RefCounted
{	
	__DeclareClass(PhysicsUserData);
public:
	/// constructor
	PhysicsUserData():physicsObject(NULL),enableCollisionCallback(false){}

	/// pointer to physics object used in simulation
	PhysicsObject* physicsObject;
	/// pointer to object used on nebulas side, can be 0
	Ptr<Core::RefCounted> object;
	/// enabling collision callbacks 
	bool enableCollisionCallback;
};

class PhysicsObject : public Core::RefCounted
{

	__DeclareClass(PhysicsObject);
public:  

	/// an unique physics id
	typedef unsigned int Id;

	/// constructor
	PhysicsObject();
	/// destructor
	virtual ~PhysicsObject();
	
	/// render debug representation of object
	virtual void RenderDebug();

	/// set transform
	virtual void SetTransform(const Math::matrix44 & trans);
	/// get transform
	virtual const Math::matrix44 & GetTransform();

	/// get pointer to Collider we are attached to
	const Ptr<Collider>& GetCollider() const;

	/// set collider category
	virtual void SetCollideCategory(CollideCategory coll);
	/// get collider category
	CollideCategory GetCollideCategory() const;	

	/// set collision filter
	virtual void SetCollideFilter(uint mask);
	/// get collision filter
	uint GetCollideFilter() const;

	/// get the objects unique id
	Id GetUniqueId() const;

	/// set name of rigid body
	void SetName(const Util::String& n);
	/// get name of rigid body
	const Util::String& GetName() const;
	

	/// called before simulation step is taken
	virtual void OnStepBefore(){}
	/// called after simulation step is taken
	virtual void OnStepAfter(){}
	/// called before simulation takes place
	virtual void OnFrameBefore(){}
	/// called after simulation takes place
	virtual void OnFrameAfter(){}

	/// enable/disable the object
	virtual void SetEnabled(bool b);
	/// get enabled/disabled state of the object
	virtual bool IsEnabled() const;

	/// set material
	virtual void SetMaterialType(MaterialType t);
	/// get material
	MaterialType GetMaterialType() const;
	/// set user data (normally a PhysicsUserData object
	void SetUserData(const Ptr<RefCounted> & object);
	/// get user data
	PhysicsUserData* GetUserData() const;

	/// enable collision feedback (enable callbacks)
	virtual void SetEnableCollisionCallback(bool enable);
	/// callbacks enabled
	bool GetEnableCollisionCallback();

	/// create a object from stream, will create subclass depending on resource
	static Util::Array<Ptr<PhysicsObject>> CreateFromStream(const Util::String & filename, const Math::matrix44 & transform);
	/// create a new physics object using the descripting in the template
	static Ptr<PhysicsObject> CreateFromTemplate(const PhysicsCommon & tmpl);	
	/// get template used for creation
	PhysicsCommon & GetTemplate();

protected:
	friend class BaseScene;
	///
	virtual void Attach(BaseScene * world) = 0;
	///
	virtual void Detach() = 0;
	///
	virtual void SetupFromTemplate(const PhysicsCommon & tmpl);

	
	Ptr<PhysicsUserData> userData;
	Math::matrix44 transform;	
	Id uniqueId;
	PhysicsCommon common;
	bool enabled;
	bool attached;
private:
	static Id uniqueIdCounter;	
};

inline PhysicsCommon & 
PhysicsObject::GetTemplate() 
{
	return this->common;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
PhysicsObject::SetTransform(const Math::matrix44 & trans)
{
	this->transform = trans;
}

//------------------------------------------------------------------------------
/**
*/
inline
const Math::matrix44 &
PhysicsObject::GetTransform()
{
	return this->transform;
}

inline
void
PhysicsObject::SetEnabled(bool b)
{
	this->enabled = b;
}

inline
bool
PhysicsObject::IsEnabled() const
{
	return this->enabled;
}

//------------------------------------------------------------------------------
/**
*/
inline
PhysicsObject::Id
PhysicsObject::GetUniqueId() const
{
	return this->uniqueId;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
PhysicsObject::SetName(const Util::String& n)
{
	this->common.name = n;
}

//------------------------------------------------------------------------------
/**
*/
inline
const Util::String&
PhysicsObject::GetName() const
{
	return this->common.name;
}

inline void
PhysicsObject::SetUserData(const Ptr<Core::RefCounted> & data)
{
	this->userData->object = data;
}
inline PhysicsUserData* 
PhysicsObject::GetUserData() const
{
	return this->userData.get();
}

inline const Ptr<Collider> &
PhysicsObject::GetCollider() const
{
	return this->common.collider;
}

inline
CollideCategory 
PhysicsObject::GetCollideCategory() const
{
	return this->common.category;
}

inline
void
PhysicsObject::SetCollideCategory(CollideCategory coll)
{
	this->common.category = coll;
}

inline
uint 
PhysicsObject::GetCollideFilter() const
{
	return this->common.collideFilterMask;
}

inline
void
PhysicsObject::SetCollideFilter(uint coll)
{
	this->common.collideFilterMask = coll;
}

inline
void
PhysicsObject::SetMaterialType(MaterialType mat)
{
	this->common.material = mat;
}
inline
MaterialType
PhysicsObject::GetMaterialType() const
{
	return this->common.material;
}
inline
bool
PhysicsObject::GetEnableCollisionCallback()
{
	return this->userData->enableCollisionCallback;
}
inline
void
PhysicsObject::SetEnableCollisionCallback(bool enable)
{
	this->userData->enableCollisionCallback = enable;
}
}