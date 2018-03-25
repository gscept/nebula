#pragma once
//------------------------------------------------------------------------------
/**
	A camera represents a viewing point into a Stage by attachment through a View
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/idallocator.h"
#include "camerasettings.h"
namespace Graphics
{

ID_32_TYPE(CameraId);

/// create new camera
CameraId CreateCamera();
/// destroy camera
void DestroyCamera(CameraId id);

/// set as projection using FoV
void CameraSetProjectionFov(CameraId id, float aspect, float fov, float znear, float zfar);
/// set orthogonal
void CameraSetOrthogonal(CameraId id, float width, float height, float znear, float zfar);

/// set transform of camera
void CameraSetTransform(CameraId id, const Math::matrix44& transform);

/// get settings
const CameraSettings& CameraGetSettings(CameraId id);
/// get transform
const Math::matrix44& CameraGetTransform(CameraId id);
/// get projection
const Math::matrix44& CameraGetProjection(CameraId id);

typedef Ids::IdAllocator<
	Graphics::CameraSettings,
	Math::matrix44,				// projection
	Math::matrix44				// view-transform
> CameraAllocator;

extern CameraAllocator cameraAllocator;

} // namespace Graphics