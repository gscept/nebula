//------------------------------------------------------------------------------
//  camerasettings.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "graphics/camerasettings.h"
#include "coregraphics/displaydevice.h"
#include "coregraphics/displaymode.h"

namespace Graphics
{
using namespace Math;
using namespace CoreGraphics;
//------------------------------------------------------------------------------
/**
*/
CameraSettings::CameraSettings() :
    viewProjDirty(true),
    viewMatrix(matrix44::identity()),
    viewProjMatrix(matrix44::identity())
{
	DisplayMode mode = WindowGetDisplayMode(DisplayDevice::Instance()->GetCurrentWindow());
    this->SetupPerspectiveFov(n_deg2rad(60.0f), mode.GetHeight() / (float)mode.GetWidth(), 0.1f, 2500.0f);
}

//------------------------------------------------------------------------------
/**
    Setup camera as perspective projection. This method can be called
    before or after setting up the object. When the object is alive,
    an update message will be sent to the render-thread.
*/
void
CameraSettings::SetupPerspectiveFov(float fov_, float aspect_, float zNear_, float zFar_)
{
    this->isPersp = true;
    this->zNear   = zNear_;
    this->zFar    = zFar_;
    this->fov     = fov_;
    this->aspect  = aspect_;

    this->projMatrix = matrix44::perspfovrh(this->fov, this->aspect, this->zNear, this->zFar);
    this->invProjMatrix = matrix44::inverse(this->projMatrix);

    this->nearWidth  = 2.0f * this->zNear / this->projMatrix.getrow0().x();
    this->nearHeight = 2.0f * this->zNear / this->projMatrix.getrow1().y();
    this->farWidth   = (this->nearWidth / this->zNear) * this->zFar;
    this->farHeight  = (this->nearHeight / this->zNear) * this->zFar;
    float yLen = Math::n_tan(0.5f * this->fov);
    float xLen = yLen * this->aspect;
    this->focalLength.set(xLen, yLen);

    this->viewProjDirty = true;
}

//------------------------------------------------------------------------------
/**
    Setup camera as orthogonal projection.  This method can be called
    before or after setting up the object. When the object is alive,
    an update message will be sent to the render-thread.
*/
void
CameraSettings::SetupOrthogonal(float w, float h, float zNear_, float zFar_)
{
    this->isPersp    = false;
    this->zNear      = zNear_;
    this->zFar       = zFar_;
    this->fov        = 0.0f;
    this->aspect     = w / h;
    this->nearWidth  = w;
    this->nearHeight = h;
    this->farWidth   = w;
    this->farHeight  = h;
    this->focalLength.set(1.0f, 1.0f);

    this->projMatrix = matrix44::orthorh(w, h, this->zNear, this->zFar);
    this->invProjMatrix = matrix44::inverse(this->projMatrix);

    this->viewProjDirty = true;
}

//------------------------------------------------------------------------------
/**
    Updates the view-projection matrix.
*/
void
CameraSettings::UpdateViewProjMatrix() const
{
    n_assert(this->viewProjDirty);
    this->viewProjDirty = false;
    this->viewProjMatrix = matrix44::multiply(this->viewMatrix, this->projMatrix);
}

//------------------------------------------------------------------------------
/**
*/
void
CameraSettings::SetProjectionMatrix(const Math::matrix44 & proj, float fov, float aspect, float zNear, float zFar)
{    
    this->viewProjDirty = true;
    this->zFar = zFar;
    this->zNear = zNear;
    this->projMatrix = proj;
    this->aspect = aspect;
    this->fov = fov;
    this->isPersp = true;
    this->invProjMatrix = matrix44::inverse(this->projMatrix);


    this->nearWidth = 2.0f * this->zNear / this->projMatrix.getrow0().x();
    this->nearHeight = 2.0f * this->zNear / this->projMatrix.getrow1().y();
    this->farWidth = (this->nearWidth / this->zNear) * this->zFar;
    this->farHeight = (this->nearHeight / this->zNear) * this->zFar;
    float yLen = Math::n_tan(0.5f * this->fov);
    float xLen = yLen * this->aspect;
    this->focalLength.set(xLen, yLen);

    this->viewProjDirty = true;
}

//------------------------------------------------------------------------------
/**
*/
void
CameraSettings::UpdateViewFrustum() const
{
    Math::matrix44 invViewProj = Math::matrix44::inverse(this->GetViewProjTransform());
    this->viewFrustum.set(invViewProj);
    this->viewFrustumDirty = false;
}

} // namespace Shared