#pragma once
//------------------------------------------------------------------------------
/**
    @class Physics::MouseGripper

    A MouseGripper lets the user manipulate object positions in the physics 
    simulation by grabbing, dragging and releasing them. It's usually
    attached to the mouse, so that the user can intuitively manipulate
    the physics objects around him.
    
    (C) 2012-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "core/ptr.h"
#include "physics/physicsbody.h"
#include "physics/contact.h"
#include "physics/joint.h"
#include "math/pfeedbackloop.h"
#include "math/float2.h"
#include "math/line.h"
#include "physics/joints/pointtopointjoint.h"

//------------------------------------------------------------------------------

namespace Physics
{
class MouseGripper : public Core::RefCounted
{
	__DeclareClass(MouseGripper);
public:
    /// constructor
    MouseGripper();
    /// destructor
    virtual ~MouseGripper();
    /// called before physics frame
    void OnFrameBefore();
    /// called after physics frame
    void OnFrameAfter();
    /// called before one physics step
    void OnStepBefore();
    /// called after one physics step
    void OnStepAfter();
    /// set the maximum grabble distance
    void SetMaxDistance(float d);
    /// get the maximum grabble distance
    float GetMaxDistance() const;
    /// get the currently grabbed entity (0 if none)
    WeakPtr<PhysicsBody> GetGrabbedEntity() const;
    /// enable/disable the gripper
    void SetEnabled(bool b);
    /// currently enabled?
    bool IsEnabled() const;
    /// toggle the grip
    void ToggleGrip();
    /// open the grip
    void OpenGrip();
	/// try to grab entity below mouse
	bool TryGrab();
    /// close the grip
	void CloseGrip();
    /// is grip opened
    bool IsGripOpen() const;
    /// render a debug visualization of the mouse gripper
    void RenderDebug();
    /// set maximum force, which can be handled with mouse gripper
    void SetMaxForce(float force);    

	void SetWorldMouseRay(const Math::line& line);

	/// attach to center of object instead of the actual contact point
	void SetUseCenter(bool center);
	///
	bool GetUseCenter() const;

private:
    /// update the closed gripper position
    void UpdateGripPosition();
    /// update force applied to grabbed rigid body
    void UpdateGripForce();    

    static const float positionGain;
    static const float positionStepSize;

    Math::line worldMouseRay;
    float maxForce;
    float maxDistance;
    float curDistance;
    bool enabled;
    bool gripOpen;
	bool useCenter;
    Ptr<Contact> contactPoint;   
	Ptr<PointToPointjoint> grabbingJoint;
	WeakPtr<PhysicsBody> body;
    Math::PFeedbackLoop<Math::vector> gripPosition;	
};

//------------------------------------------------------------------------------
/**
*/
inline
void
MouseGripper::SetMaxDistance(float d)
{
    this->maxDistance = d;
}

inline
void
MouseGripper::SetWorldMouseRay(const Math::line& line)
{
	this->worldMouseRay = line;
}
//------------------------------------------------------------------------------
/**
*/
inline
float
MouseGripper::GetMaxDistance() const
{
    return this->maxDistance;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
MouseGripper::SetEnabled(bool b)
{
    this->enabled = b;
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
MouseGripper::IsEnabled() const
{
    return this->enabled;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
MouseGripper::SetUseCenter(bool b)
{
	this->useCenter = b;
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
MouseGripper::GetUseCenter() const
{
	return this->useCenter;
}
//------------------------------------------------------------------------------
/**
*/
inline
void
MouseGripper::ToggleGrip()
{
    if (this->gripOpen)
    {
        this->CloseGrip();
    }
    else
    {
        this->OpenGrip();
    }
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
MouseGripper::IsGripOpen() const
{
    return this->gripOpen;
}


//------------------------------------------------------------------------------
/**
*/
inline void 
MouseGripper::SetMaxForce(float force)
{
    this->maxForce = force;
}

}; // namespace Physics
//------------------------------------------------------------------------------
