//------------------------------------------------------------------------------
//  freecamerautil.cc
//  (C) 2012-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
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
    cameraTransform(mat4()),
    rotationSpeed(0.01f),
    moveSpeed(0.01f),
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
    this->targetPosition = this->defaultEyePos;
    this->viewAngles.set(this->defaultEyeVec);
    this->Update(0.01667f);
}

//------------------------------------------------------------------------------
/**
*/
void 
FreeCameraUtil::Reset()
{
    this->viewAngles.set(this->defaultEyeVec);
    this->position = this->defaultEyePos;
    this->targetPosition = this->defaultEyePos;
    this->Update(0.01667f);
}

//------------------------------------------------------------------------------
/**
*/
void 
FreeCameraUtil::Update(float deltaTime)
{
    if (this->rotateButton)
    {
        this->viewAngles.rho += this->mouseMovement.x * rotationSpeed;
        this->viewAngles.theta += this->mouseMovement.y * rotationSpeed;
    }

    mat4 xMat = rotationx(this->viewAngles.theta - (N_PI * 0.5f));
    mat4 yMat = rotationy(this->viewAngles.rho);
    this->cameraTransform = yMat * xMat;

    float currentMoveSpeed = moveSpeed;
    if(this->accelerateButton)
    {
        currentMoveSpeed *= 20;
    }
    vec4 translation = vec4(0,0,0,0);
    if (forwardsKey)
    {
        translation.z -= currentMoveSpeed;
    }
    if (backwardsKey)
    {
        translation.z += currentMoveSpeed;
    }
    if (rightStrafeKey)
    {
        translation.x += currentMoveSpeed;
    }
    if (leftStrafeKey)
    {
        translation.x -= currentMoveSpeed;
    }
    if (upKey)
    {
        translation.y += currentMoveSpeed;
    }
    if (downKey)
    {
        translation.y -= currentMoveSpeed;
    }

    translation = this->cameraTransform * translation;
    this->targetPosition += xyz(translation);

    this->position = Math::lerp(this->position, this->targetPosition, deltaTime * 10.0f);

    this->cameraTransform.position = point(this->position);
}


} // namespace RenderUtil
