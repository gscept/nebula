//------------------------------------------------------------------------------
//  camerasettings.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "graphics/camerasettings.h"
#include "coregraphics/displaydevice.h"
#include "coregraphics/displaymode.h"
#include "coregraphics/config.h"

namespace Graphics
{
using namespace Math;
using namespace CoreGraphics;
//------------------------------------------------------------------------------
/**
*/
CameraSettings::CameraSettings()
{
    DisplayMode mode = WindowGetDisplayMode(CoreGraphics::MainWindow);
    this->SetupPerspectiveFov(Math::deg2rad(60.0f), mode.GetHeight() / (float)mode.GetWidth(), 0.1f, 2500.0f);
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

#if PROJECTION_HANDEDNESS_LH
    this->projMatrix = perspfovlh(this->fov, this->aspect, this->zNear, this->zFar);
#else
    this->projMatrix = perspfovrh(this->fov, this->aspect, this->zNear, this->zFar);
#endif
    this->invProjMatrix = inverse(this->projMatrix);

    this->nearWidth  = 2.0f * this->zNear / this->projMatrix.r[0].x;
    this->nearHeight = 2.0f * this->zNear / this->projMatrix.r[1].y;
    this->farWidth   = (this->nearWidth / this->zNear) * this->zFar;
    this->farHeight  = (this->nearHeight / this->zNear) * this->zFar;
    float yLen = Math::tan(0.5f * this->fov);
    float xLen = yLen * this->aspect;
    this->focalLength.set(xLen, yLen);
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

#if PROJECTION_HANDEDNESS_LH
    this->projMatrix = ortholh(w, h, this->zNear, this->zFar);
#else
    this->projMatrix = orthorh(w, h, this->zNear, this->zFar);
#endif
    this->invProjMatrix = inverse(this->projMatrix);
}

} // namespace Shared
