#pragma once
//------------------------------------------------------------------------------
/**
    @class Physics::BaseChainJoint
    
	Base class for a joint chain, where a series of physicsbodies and joints 
	are appended in a sequence. Note that the number of physics bodies must 
	equal num joints + 1.
    
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "physics/joint.h"

//------------------------------------------------------------------------------
namespace Physics
{
class PhysicsBody;

class BaseChainJoint : public Joint
{
	__DeclareAbstractClass(BaseChainJoint);
public:
	/// constructor
	BaseChainJoint();
	/// destructor
	virtual ~BaseChainJoint();

	/// attach to scene
	virtual void Attach(Physics::BaseScene * world) {n_error("Not Implemented"); };
	/// detach from scene
	virtual void Detach() {n_error("Not Implemented"); };

	/// append a new joint, with a target rotation of body B relative to A (must call AppendPhysicsBody after this)
	virtual void AppendJoint(const Math::point& pivotInA, const Math::point& pivotInB, const Math::quaternion& targetRotation);
	/// append a new physicsbody
	virtual void AppendPhysicsBody(const Ptr<PhysicsBody>& body);
		
	/// get the number of individual joints this chain consists of
	virtual int GetNumJoints();

protected:
	bool initialized;

}; 
} 
// namespace Physics
//------------------------------------------------------------------------------