#pragma once
//------------------------------------------------------------------------------
/**
    @class Shared::CameraSettings
    
    Wraps camera settings into an object.
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "math/matrix44.h"
#include "math/float2.h"
#include "math/frustum.h"

//------------------------------------------------------------------------------
namespace Graphics
{
class CameraSettings
{
public:
    /// default constructor
    CameraSettings();

    /// setup a perspective view volume
    void SetupPerspectiveFov(float fov, float aspect, float zNear, float zFar);
    /// setup an orthogonal projection transform
    void SetupOrthogonal(float w, float h, float zNear, float zFar);
    /// setup a custom projection matrix. zNear and zFar are only used as info.
    void SetProjectionMatrix(const Math::matrix44& proj, float fov, float aspect, float zNear, float zFar);
    /// update view matrix
    void UpdateViewMatrix(const Math::matrix44& m);

    /// get projection matrix
    const Math::matrix44& GetProjTransform() const;
    /// get the inverse projection matrix
    const Math::matrix44& GetInvProjTransform() const;
    /// get view transform (inverse transform)
    const Math::matrix44& GetViewTransform() const;
    /// get view projection matrix (non-const!)
    const Math::matrix44& GetViewProjTransform() const;
    /// get view frustum 
    const Math::frustum& GetViewFrustum() const;
    /// get focal length (computed from fov and aspect ratio)
    const Math::float2& GetFocalLength() const;

    /// return true if this is a perspective projection
    bool IsPerspective() const;
    /// return true if this is an orthogonal transform
    bool IsOrthogonal() const;
    /// get near plane distance
    float GetZNear() const;
    /// get far plane distance
    float GetZFar() const;
    /// get field-of-view (only if perspective)
    float GetFov() const;
    /// get aspect ration (only if perspective)
    float GetAspect() const;
    /// get width of near plane
    float GetNearWidth() const;
    /// get height of near plane
    float GetNearHeight() const;
    /// get width of far plane
    float GetFarWidth() const;
    /// get height of far plane
    float GetFarHeight() const;
    
private:
    /// update the view projection matrix
    void UpdateViewProjMatrix() const;
    /// update the view frustum
    void UpdateViewFrustum() const;

    Math::matrix44 projMatrix;
    Math::matrix44 invProjMatrix;
    Math::matrix44 viewMatrix;
    mutable Math::matrix44 viewProjMatrix;
    mutable Math::frustum viewFrustum;
    mutable bool viewProjDirty;
    mutable bool viewFrustumDirty;

    bool isPersp;
    float zNear;
    float zFar;
    float fov;
    float aspect;
    float nearWidth;
    float nearHeight;
    float farWidth;
    float farHeight;
    Math::float2 focalLength;
};

//------------------------------------------------------------------------------
/**
*/
inline void
CameraSettings::UpdateViewMatrix(const Math::matrix44& m)
{
    this->viewMatrix = m;
    this->viewProjDirty = true;
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::matrix44&
CameraSettings::GetProjTransform() const
{
    return this->projMatrix;
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::matrix44&
CameraSettings::GetInvProjTransform() const
{
    return this->invProjMatrix;
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::matrix44&
CameraSettings::GetViewTransform() const
{
    return this->viewMatrix;
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::matrix44&
CameraSettings::GetViewProjTransform() const
{
    if (this->viewProjDirty)
    {
        this->UpdateViewProjMatrix();
    }
    return this->viewProjMatrix;
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::frustum&
CameraSettings::GetViewFrustum() const
{
    if (this->viewFrustumDirty)
    {
        this->UpdateViewFrustum();
    }
    return this->viewFrustum;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
CameraSettings::IsPerspective() const
{
    return this->isPersp;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
CameraSettings::IsOrthogonal() const
{
    return !this->isPersp;
}

//------------------------------------------------------------------------------
/**
*/
inline float
CameraSettings::GetZNear() const
{
    return this->zNear;
}

//------------------------------------------------------------------------------
/**
*/
inline float
CameraSettings::GetZFar() const
{
    return this->zFar;
}

//------------------------------------------------------------------------------
/**
*/
inline float
CameraSettings::GetFov() const
{
    return this->fov;
}

//------------------------------------------------------------------------------
/**
*/
inline float
CameraSettings::GetAspect() const
{
    return this->aspect;
}

//------------------------------------------------------------------------------
/**
*/
inline float
CameraSettings::GetNearWidth() const
{
    return this->nearWidth;
}

//------------------------------------------------------------------------------
/**
*/
inline float
CameraSettings::GetNearHeight() const
{
    return this->nearHeight;
}

//------------------------------------------------------------------------------
/**
*/
inline float
CameraSettings::GetFarWidth() const
{
    return this->farWidth;
}

//------------------------------------------------------------------------------
/**
*/
inline float
CameraSettings::GetFarHeight() const
{
    return this->farHeight;
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::float2&
CameraSettings::GetFocalLength() const
{
    return this->focalLength;
}

} // namespace Graphics
//------------------------------------------------------------------------------
    