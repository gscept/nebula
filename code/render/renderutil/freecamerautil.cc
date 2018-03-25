//------------------------------------------------------------------------------
//  freecamerautil.cc
//  (C) 2012-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "renderutil/freecamerautil.h"

namespace RenderUtil
{
using namespace Math;

//------------------------------------------------------------------------------
/**
*/
FreeCameraUtil::FreeCameraUtil() : 
	defaultEyePos(0,0,0),
	defaultEyeVec(0,0,1),
	rotationSpeed(0.01f),
	moveSpeed(0.01f),
	cameraTransform(matrix44::identity()),
	rotateButton(false),
	accelerateButton(false),
	forwardsKey(false),
	backwardsKey(false),
	leftStrafeKey(false),
	rightStrafeKey(false),
	upKey(false),
	downKey(false)

{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void 
FreeCameraUtil::Setup( const Math::point& defaultEyePos, const Math::vector& defaultEyeVec )
{
	this->defaultEyePos = defaultEyePos;
	this->defaultEyeVec = defaultEyeVec;
	this->position = this->defaultEyePos;
	this->viewAngles.set(this->defaultEyeVec);
	this->Update();
}

//------------------------------------------------------------------------------
/**
*/
void 
FreeCameraUtil::Reset()
{
	this->viewAngles.set(this->defaultEyeVec);
	this->position = this->defaultEyePos;
	this->Update();
}

//------------------------------------------------------------------------------
/**
*/
void 
FreeCameraUtil::Update()
{
	if (this->rotateButton)
	{
		this->viewAngles.rho += this->mouseMovement.x() * rotationSpeed;
		this->viewAngles.theta += this->mouseMovement.y() * rotationSpeed;
	}

	matrix44 xMat = matrix44::rotationx(this->viewAngles.theta - (N_PI * 0.5f));
	matrix44 yMat = matrix44::rotationy(this->viewAngles.rho);
	this->cameraTransform = matrix44::multiply(xMat, yMat);

	float currentMoveSpeed = moveSpeed;
	if(this->accelerateButton)
	{
		currentMoveSpeed *= 20;
	}
	float4 translation = float4(0,0,0,0);
	if (forwardsKey)
	{
		translation.z() -= currentMoveSpeed;
	}
	if (backwardsKey)
	{
		translation.z() += currentMoveSpeed;
	}
	if (rightStrafeKey)
	{
		translation.x() += currentMoveSpeed;
	}
	if (leftStrafeKey)
	{
		translation.x() -= currentMoveSpeed;
	}
	if (upKey)
	{
		translation.y() += currentMoveSpeed;
	}
	if (downKey)
	{
		translation.y() -= currentMoveSpeed;
	}

	translation = matrix44::transform(translation, this->cameraTransform);
	this->position += translation;

	this->cameraTransform.set_position(this->position);
}


} // namespace RenderUtil