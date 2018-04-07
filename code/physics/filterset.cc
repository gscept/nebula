//------------------------------------------------------------------------------
//  physics/filterset.cc
//  (C) 2005 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "collider.h"
#include "physics/filterset.h"
#include "physics/physicsobject.h"

namespace Physics
{
//------------------------------------------------------------------------------
/**
*/
bool
FilterSet::CheckObject(const PhysicsObject * object) const
{
    n_assert(object != NULL);
	if(this->CheckRigidBodyId(object->GetUniqueId()))
	{
		return true;
	}
	if(this->CheckMaterialType(object->GetMaterialType()))
	{
		return true;
	}
    if (0 != (this->collideBits & object->GetCollideCategory()))
    {            
		return true;		
	}
    return false;
}

};
