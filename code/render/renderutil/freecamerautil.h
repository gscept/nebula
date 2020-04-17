#pragma once
//------------------------------------------------------------------------------
/**
    @class RenderUtil::FreeCameraUtil
    
    Implements a free camera
    
    (C) 2012-2020 Individual contributors, see AUTHORS file
*/
#include "math/mat4.h"
#include "math/vec2.h"
#include "math/polar.h"
#include "math/scalar.h"

//------------------------------------------------------------------------------
namespace RenderUtil
{
class FreeCameraUtil
{
public:
	/// constructor
	FreeCameraUtil();

	/// sets up free camera
	void Setup(const Math::vec3& defaultEyePos, const Math::vec3& defaultEyeVec);
	/// resets free camera to default values
	void Reset();
	/// updates camera matrix
	void Update();
	/// gets camera transform
	const Math::mat4& GetTransform() const;

	/// sets the state of the rotate button
	void SetRotateButton(bool state);
	/// sets the state of the accelerate button
	void SetAccelerateButton(bool state);
	/// sets the mouse movement
	void SetMouseMovement(Math::vec2 movement);
	/// sets the rotation speed
	void SetRotationSpeed(float speed);
	/// sets the movement speed
	void SetMovementSpeed(float speed);

	/// sets the forward movement key
	void SetForwardsKey(bool state);
	/// sets the backward movement key
	void SetBackwardsKey(bool state);
	/// sets the left strafe key
	void SetLeftStrafeKey(bool state);
	/// sets the right strafe key
	void SetRightStrafeKey(bool state);
	/// sets the up movement key
	void SetUpKey(bool state);
	/// sets the down movement key
	void SetDownKey(bool state);
	
private:
	Math::vec3 defaultEyePos;
	Math::vec3 defaultEyeVec;
	Math::vec2 mouseMovement;

	Math::polar viewAngles;
	Math::vec3 position;
	Math::mat4 cameraTransform;

	float rotationSpeed;
	float moveSpeed;

	bool rotateButton;
	bool accelerateButton;

	bool forwardsKey;
	bool backwardsKey;
	bool leftStrafeKey;
	bool rightStrafeKey;
	bool upKey;
	bool downKey;
}; 


//------------------------------------------------------------------------------
/**
*/
inline const Math::mat4& 
FreeCameraUtil::GetTransform() const
{
	return this->cameraTransform;
}


//------------------------------------------------------------------------------
/**
*/
inline void 
FreeCameraUtil::SetRotateButton( bool state )
{
	this->rotateButton = state;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
FreeCameraUtil::SetAccelerateButton( bool state )
{
	this->accelerateButton = state;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
FreeCameraUtil::SetMouseMovement( Math::vec2 movement )
{
	this->mouseMovement = movement;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
FreeCameraUtil::SetRotationSpeed( float speed )
{
	this->rotationSpeed = speed;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
FreeCameraUtil::SetMovementSpeed( float speed )
{
	this->moveSpeed = speed;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
FreeCameraUtil::SetForwardsKey( bool state )
{
	this->forwardsKey = state;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
FreeCameraUtil::SetBackwardsKey( bool state )
{
	this->backwardsKey = state;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
FreeCameraUtil::SetLeftStrafeKey( bool state )
{
	this->leftStrafeKey = state;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
FreeCameraUtil::SetRightStrafeKey( bool state )
{
	this->rightStrafeKey = state;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
FreeCameraUtil::SetUpKey( bool state )
{
	this->upKey = state;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
FreeCameraUtil::SetDownKey( bool state )
{
	this->downKey = state;
}

} // namespace RenderUtil
//------------------------------------------------------------------------------