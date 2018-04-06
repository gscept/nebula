#pragma once
//------------------------------------------------------------------------------
/**
    @class Bullet::NebulaMotionState

    (C) 2012-2016 Individual contributors, see AUTHORS file
*/

#include "LinearMath/btMotionState.h"
#include "physics/physicsserver.h"

namespace Bullet
{

class NebulaMotionState : public btMotionState
{
public:	
	/// constructor
	NebulaMotionState(BulletBody*b): body(b) {}

	/// destructor
	~NebulaMotionState(){}
	
	/// get current world transform
	virtual void getWorldTransform(btTransform& worldTrans) const
	{
		worldTrans = Neb2BtM44Transform(this->transform);
	}
	/// set world transform, called by bullets simulation
	virtual void setWorldTransform(const btTransform& worldTransform)
	{
        this->transform = Bt2NebTransform(worldTransform);		
		body->UpdateTransform( this->transform);
	}
    		
private:
    Math::matrix44  transform;		
	BulletBody* body;
};

};
