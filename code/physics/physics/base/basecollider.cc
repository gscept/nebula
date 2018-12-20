//------------------------------------------------------------------------------
//  baseshape.cc
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "physics/resource/managedphysicsmesh.h"
#include "physics/base/basecollider.h"
#include "resources/resourcemanager.h"

namespace Physics
{
__ImplementAbstractClass(Physics::BaseCollider, 'PHBC', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
BaseCollider::BaseCollider()     
{
	/// empty
}

//------------------------------------------------------------------------------
/**
*/
BaseCollider::~BaseCollider()
{	
	///empty
}

//------------------------------------------------------------------------------
/**
*/
void 
BaseCollider::AddFromDescription(const ColliderDescription & description)
{		
	this->descriptions.Append(description);
}

} // namespace Physics
