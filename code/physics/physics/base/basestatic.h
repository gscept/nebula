#pragma once
//------------------------------------------------------------------------------
/**
    @class Physics::BaseStatic

    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "physics/physicsobject.h"
#include "physics/collider.h"
#include "math/matrix44.h"

namespace Physics
{
class Scene;

class BaseStatic : public PhysicsObject
{
__DeclareClass(BaseStatic);

public:
	BaseStatic(){}
	~BaseStatic(){}
	
protected:	
	friend class PhysicsObject;
	virtual void SetupFromTemplate(const PhysicsCommon & templ);

};
}