//------------------------------------------------------------------------------
//  mayacamerautil.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "renderutil/mayacamerautil.h"
#include "math/quaternion.h"

namespace RenderUtil
{
using namespace Math;

//------------------------------------------------------------------------------
/**
*/
MayaCameraUtil::MayaCameraUtil() :
    defaultCenterOfInterest(0.0f, 0.0f, 0.0f),
    defaultEyePos(0.0f, 0.0f, 10.0f),
    defaultUpVec(0.0f, 1.0f, 0.0f),
    viewDistance(0.0f),
    centerOfInterest(0.0f, 0.0f, 0.0f),
    cameraTransform(matrix44::identity()),
    orbitButton(false),
    panButton(false),
    zoomButton(false),
    zoomInButton(false),
    zoomOutButton(false),
    mouseMovement(0.0f, 0.0f),
    zoomIn(0.0f),
    zoomOut(0.0f),
    panning(0.0f, 0.0f),
    orbiting(0.0f, 0.0f)
{
    this->Setup(point(0.0f, 0.0f, 0.0f), point(5.0f, 5.0f, 5.0f), vector(0.0f, 1.0f, 0.0f));
}
    
//------------------------------------------------------------------------------
/**
*/
void
MayaCameraUtil::Setup(const point& defCoi, const point& defEyePos, const vector& defUpVec)
{
    this->defaultCenterOfInterest = defCoi;
    this->defaultEyePos = defEyePos;
    this->defaultUpVec = defUpVec;
    this->Reset();
}

//------------------------------------------------------------------------------
/**
*/
void
MayaCameraUtil::Reset()
{
    vector viewVec = this->defaultEyePos - this->defaultCenterOfInterest;
    this->viewDistance = viewVec.length();
    this->viewAngles.set(float4::normalize(viewVec));
    this->centerOfInterest = this->defaultCenterOfInterest;
    this->cameraTransform = matrix44::identity();
    this->Update();
}

//------------------------------------------------------------------------------
/**
*/
void
MayaCameraUtil::Update()
{
    const scalar defOrbitVelocity = 0.015f;
    const scalar defPanVelocity  = 0.01f; // before 0.008f
    const scalar defZoomVelocity = 0.015f;
    const scalar minPanVelocity  = 0.01f; // before 0.008f
    const scalar minZoomVelocity = 0.050f;

    // zooming and panning speed is greater when farther away
    scalar panVelocity = defPanVelocity * this->viewDistance;
    panVelocity = defPanVelocity * 10;
    scalar zoomVelocity = defZoomVelocity * this->viewDistance;
    zoomVelocity = defZoomVelocity * 10;

    // handle input
    scalar panHori  = -this->panning.x();
    scalar panVert  =  this->panning.y();
    scalar lookHori = -this->orbiting.x();
    scalar lookVert = -this->orbiting.y();
    scalar zoom = this->zoomOut - zoomIn;
    if (this->orbitButton)
    {
        lookHori += this->mouseMovement.x();
        lookVert += this->mouseMovement.y();
    }
    if (this->panButton)
    {
        panHori += this->mouseMovement.x();
        panVert -= this->mouseMovement.y();
    }
    if (this->zoomButton)
    {
        zoom += this->mouseMovement.y();
    }

    // handle panning
    vector horiMove = this->cameraTransform.getrow0() * panHori * panVelocity;
    vector vertMove = this->cameraTransform.getrow1() * panVert * panVelocity;
    this->centerOfInterest += horiMove + vertMove;

    // handle zooming
    if (this->zoomInButton)
    {
        zoom -= 1.0f;
    }
    if (this->zoomOutButton)
    {
        zoom += 1.0f;
    }
    this->viewDistance += zoom * zoomVelocity;

    // handle orbiting
    this->viewAngles.theta += lookVert * defOrbitVelocity;
    this->viewAngles.rho   += lookHori * defOrbitVelocity;

    // avoid that the camera slips past the center of interest
    if (this->viewDistance < 1.0f)
    {
		this->centerOfInterest -= this->cameraTransform.getrow2() * (1.0f-this->viewDistance);
        this->viewDistance = 1.0f;
    }

    // get polar vector in cartesian space
    matrix44 m = matrix44::translation(0.0f, 0.0f, this->viewDistance);
    m = matrix44::multiply(m, matrix44::rotationx(this->viewAngles.theta - (N_PI * 0.5f)));
    m = matrix44::multiply(m, matrix44::rotationy(this->viewAngles.rho));
    m = matrix44::multiply(m, matrix44::translation(this->centerOfInterest));
    this->cameraTransform = m;

    // reset input
    this->orbitButton = false;
    this->panButton = false;
    this->zoomButton = false;
    this->zoomInButton = false;
    this->zoomOutButton = false;
    this->mouseMovement.set(0.0f, 0.0f);
    this->zoomIn = 0.0f;
    this->zoomOut = 0.0f;
    this->panning.set(0.0f, 0.0f);
    this->orbiting.set(0.0f, 0.0f);
}

} // namespace RenderUtil