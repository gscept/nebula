#pragma once
//------------------------------------------------------------------------------
/**
	@class Physics::PhysicsModel

	(C) 2012-2016 Individual contributors, see AUTHORS file
*/
#include "resources/resource.h"
#include "physics/physicsobject.h"
#include "physics/physicsbody.h"

namespace Physics
{

class PhysicsModel : public Resources::Resource
{
    __DeclareClass(PhysicsModel);
public:
    PhysicsModel(){}
    virtual ~PhysicsModel(){};

    // helpers
    /// creates a static object for every collider in the resource
    Util::Array<Ptr<Physics::PhysicsObject>> CreateStaticInstance(const Math::matrix44& worldMatrix);
    /// creates a dynamic object for the collider in the resource (only one support, will assert otherwise)
    Ptr<Physics::PhysicsObject> CreateDynamicInstance(const Math::matrix44& worldMatrix);
    /// tries to create an instance from the resource using provided descriptions in the resource, will assert if only colliders
    Util::Array<Ptr<Physics::PhysicsObject>> CreateInstance();

	/// return if model contains any physics objects
    bool HasObjects();

	/// set name of object
    void SetName(const Util::String & name);
	///
    const Util::String & GetName();
	/// retrieve colliders contained in model
    const Util::HashTable<Util::String, Ptr<Physics::Collider>> & GetColliders();
	
protected:
    friend class StreamPhysicsModelLoader;
    Util::String name;
    Util::HashTable<Util::String, Ptr<Physics::Collider>> colliders;
    Util::Array<PhysicsCommon> objects;
    Util::Array<JointDescription> joints;

};

inline 
void
PhysicsModel::SetName(const Util::String & iname)
{
	this->name = iname;
}

inline
const Util::String & 
PhysicsModel::GetName()
{
	return this->name;
}
inline
const Util::HashTable<Util::String, Ptr<Physics::Collider>> & 
PhysicsModel::GetColliders()
{
	return colliders;
}

inline
bool 
PhysicsModel::HasObjects()
{
	return this->objects.Size()>0;
}
}
