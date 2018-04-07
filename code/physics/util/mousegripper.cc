//------------------------------------------------------------------------------
//  physics/mousegripper.cc
//  (C) 2012-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "physics/util/mousegripper.h"
#include "physics/physicsserver.h"
#include "physics/physicsbody.h"
#include "math/line.h"
#include "debugrender/debugshaperenderer.h"
#include "debug/debugfloat.h"
#include "../filterset.h"

namespace Physics
{
__ImplementClass(Physics::MouseGripper, 'PMOU', Core::RefCounted);

using namespace Math;
using namespace Debug;

const float MouseGripper::positionGain = -3.0f;
const float MouseGripper::positionStepSize = 0.01f;

//------------------------------------------------------------------------------
/**
*/
MouseGripper::MouseGripper() :
    maxDistance(20.0f),
    enabled(true),
    gripOpen(true),	
    maxForce(100.0f),
	useCenter(false)
{    
}

//------------------------------------------------------------------------------
/**
*/
MouseGripper::~MouseGripper()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    This method is called before a physics frame (consisting of multiple steps) 
    is evaluated.
*/
void
MouseGripper::OnFrameBefore()
{
    // check if grabbed rigid body has gone away for some reason
    if (!this->gripOpen)
    {
        Ptr<PhysicsBody> rigidBody = this->contactPoint->GetCollisionObject().cast<PhysicsBody>();
        if (!rigidBody.isvalid())
        {
            this->gripOpen = true;
            this->contactPoint->Clear();
			return;
        }		
		this->UpdateGripForce();
    }   
	this->UpdateGripPosition();
}

//------------------------------------------------------------------------------
/**
    This method is called after a physics frame is evaluated.
*/
void
MouseGripper::OnFrameAfter()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    This method is called before a physics step is taken.
*/
void
MouseGripper::OnStepBefore()
{
    
}

//------------------------------------------------------------------------------
/**
    This method is called after a physics step is taken.
*/
void
MouseGripper::OnStepAfter()
{
    // decide if joint should break
    if (!this->gripOpen)
    {
/*
		n_assert(this->dummyJointId);
        dJointFeedback* jf = dJointGetFeedback(this->dummyJointId);
        n_assert(jf);
        Math::vector f1, f2;
        PhysicsServer::OdeToVector3(jf->f1, f1);
        PhysicsServer::OdeToVector3(jf->f2, f2);
        float maxForce = n_max(f1.length(), f2.length());
        if (maxForce > this->maxForce)
        {
            // break the joint (open the grip)
            this->OpenGrip();
        }
		*/
    }
}

//------------------------------------------------------------------------------
/**
    Open the grip, this will release the currently grabbed physics entity
    (if any).
*/
void
MouseGripper::OpenGrip()
{
	if(this->body.isvalid())
	{
		this->body->SetSleeping(false);
		this->body = 0;
		this->grabbingJoint = 0;						
		this->contactPoint = 0;
		this->gripOpen = true;
	}    
}

//------------------------------------------------------------------------------
/**
    Close the grip, if a physics entity is under the mouse, it will be grabbed.
*/

void
MouseGripper::CloseGrip()
{
    if (!this->gripOpen)
    {
        // already closed
        return;
    }
	if(this->contactPoint.isvalid() && this->contactPoint->GetCollisionObject().isvalid()!= 0 && this->contactPoint->GetCollisionObject()->IsA(PhysicsBody::RTTI))
	{
	    
		body = this->contactPoint->GetCollisionObject().cast<PhysicsBody>();
	    body->SetSleeping(false);

		
		// compute the distance from the camera to the contact point            
		Math::vector diffVec = this->worldMouseRay.start() - this->contactPoint->GetPoint();
		this->curDistance = diffVec.length();

		this->grabbingJoint = PointToPointjoint::Create();
		
		this->grabbingJoint->SetBodies(this->contactPoint->GetCollisionObject().cast<PhysicsBody>(),0);
		
		vector attachedPoint = vector(0,0,0);
		
		if(!useCenter)		
		{
			// use a point, vector doesnt do homogeneous
			point pickPos = this->contactPoint->GetPoint();			
			point localPivot = matrix44::transform(pickPos,matrix44::inverse(body->GetTransform()));
			attachedPoint = localPivot;			
		}		
		this->grabbingJoint->Setup(attachedPoint);
		this->grabbingJoint->Attach(PhysicsServer::Instance()->GetScene());

		// constraint settings
		this->grabbingJoint->SetJointParams(0.001f, 1.3f, 30);	
	this->gripOpen = false;
	}
}

//------------------------------------------------------------------------------
/**
    This updates the grip position depending on the current mouse position.
*/
void
MouseGripper::UpdateGripPosition()
{
    Physics::PhysicsServer* physicsServer = Physics::PhysicsServer::Instance();

    // do a ray check into the environment, using maximum or current distance
    float rayLen;
    if (this->gripOpen)
    {
        rayLen = this->maxDistance;
    }
    else
    {
        rayLen = this->curDistance;
    }
    Timing::Time time = physicsServer->GetTime();
    Ptr<Physics::Contact> contactPtr;
    FilterSet excludeSet;    
    vector rayDir = vector::normalize(this->worldMouseRay.end());
    this->worldMouseRay.set(this->worldMouseRay.start(), this->worldMouseRay.start() + rayDir * rayLen);
    contactPtr = physicsServer->GetScene()->GetClosestContactUnderMouse(this->worldMouseRay, excludeSet);
    if (this->gripOpen)
    {
        if (contactPtr.isvalid())
        {
            this->contactPoint = contactPtr;
            this->gripPosition.Reset(time, positionStepSize, positionGain, contactPtr->GetPoint());
        }
        else
        {
            this->gripPosition.Reset(time, positionStepSize, positionGain, this->worldMouseRay.end());
            //this->contactPoint->Clear();
        }
    }
    else
    {
		//DebugFloat::print(this->worldMouseRay.end(),"end of ray");		
        this->gripPosition.SetGoal(this->worldMouseRay.end());
    }
    this->gripPosition.Update(time);
}

//------------------------------------------------------------------------------
/**
    Render a debug visualization of the gripper.
*/
void
MouseGripper::RenderDebug()
{
    DebugShapeRenderer* shapeRenderer = DebugShapeRenderer::Instance();

    Math::matrix44 gripTransform = Math::matrix44::identity();
    Math::float4 gripColor;
    Math::matrix44 bodyTransform = Math::matrix44::identity();
    Math::float4 bodyColor(1.0f, 0.0f, 0.0f, 1.0f);
	gripColor.set(1.0f, 1.0f, 1.0f, 1.0f);

    gripTransform.scale(Math::vector(0.1f, 0.1f, 0.1f));
    float4 pos = this->gripPosition.GetState();
    gripTransform.set_position(pos);
    if (this->gripOpen)
    {
        gripColor.set(1.0f, 1.0f, 0.0f, 1.0f);
    }
    else
    {        
		Ptr<PhysicsObject> obj = this->contactPoint->GetCollisionObject();
        if (obj.isvalid())
        {
            bodyTransform.scale(Math::vector(0.1f, 0.1f, 0.1f));
            bodyTransform.set_position(this->grabbingJoint->GetPivotB());
            shapeRenderer->DrawSphere(bodyTransform, bodyColor);
        }
        gripColor.set(1.0f, 0.0f, 1.0f, 1.0f);
    }
    shapeRenderer->DrawSphere(gripTransform, gripColor);
}
    
//------------------------------------------------------------------------------
/**
    If grip closed around a valid rigid body, apply a force to the
    rigid body which moves the body into the direction of the 
    mouse.
*/
void
MouseGripper::UpdateGripForce()
{
    if (!this->gripOpen && (0 != this->body.isvalid()))
    {        
        {
            // update the dummy body's position to correspond with the mouse            
            body->SetEnabled(true);            
            const Math::vector& pos = this->gripPosition.GetState();			
			this->grabbingJoint->SetPivotB(pos);
			//this->grabbingJoint->SetPivotB(this->worldMouseRay.end());			
        }
    }
}

//------------------------------------------------------------------------------
/**
    Returns the entity id of the currently grabbed entity, or 0 if nothing
    grabbed.
*/
WeakPtr<PhysicsBody> 
MouseGripper::GetGrabbedEntity() const
{
    if (!this->gripOpen && this->body.isvalid())
	{
		return this->body;
    }
    else
    {
        return 0;
    }
}

} // namespace Physics