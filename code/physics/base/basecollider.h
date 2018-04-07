#pragma once
//------------------------------------------------------------------------------
/**
    @class Physics::BaseCollider
    
    
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "core/ptr.h"
#include "math/bbox.h"
#include "physics/model/templates.h"

namespace Physics
{
class ManagedPhysicsMesh;

class BaseCollider : public Core::RefCounted
{
    __DeclareAbstractClass(BaseCollider);
public:
	   
    /// default constructor
    BaseCollider();
    /// destructor
    virtual ~BaseCollider();
    /// render debug visualization
	virtual void RenderDebug(const Math::matrix44& t) = 0;
    ///
    virtual void AddFromDescription(const ColliderDescription & description);
    
	const Util::String & GetName() const;

	const Util::Array<ColliderDescription>& GetDescriptions() const;
	
protected:    
	Util::String name;
	Util::Array<ColliderDescription> descriptions;
		
};

//------------------------------------------------------------------------------
/**
*/
inline
const Util::String &
BaseCollider::GetName() const
{
	return this->name;    
}

//------------------------------------------------------------------------------
/**
*/
inline
const Util::Array<ColliderDescription>&
BaseCollider::GetDescriptions() const
{
	return this->descriptions;    
}

}; // namespace Physics


    