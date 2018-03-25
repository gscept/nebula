//------------------------------------------------------------------------------
// camera.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "camera.h"

namespace Graphics
{

CameraAllocator cameraAllocator;
//------------------------------------------------------------------------------
/**
*/
CameraId CreateCamera()
{
	return cameraAllocator.AllocObject();
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyCamera(CameraId id)
{
	cameraAllocator.DeallocObject(id.id);
}

//------------------------------------------------------------------------------
/**
*/
void
CameraSetProjectionFov(CameraId id, float aspect, float fov, float znear, float zfar)
{
	CameraSettings& settings = cameraAllocator.Get<0>(id.id);
	settings.SetupPerspectiveFov(aspect, fov, znear, zfar);
	cameraAllocator.Get<1>(id.id) = settings.GetProjTransform();
}

//------------------------------------------------------------------------------
/**
*/
void
CameraSetOrthogonal(CameraId id, float width, float height, float znear, float zfar)
{
	CameraSettings& settings = cameraAllocator.Get<0>(id.id);
	settings.SetupOrthogonal(width, height, znear, zfar);
	cameraAllocator.Get<1>(id.id) = settings.GetProjTransform();
}

//------------------------------------------------------------------------------
/**
*/
void
CameraSetTransform(CameraId id, const Math::matrix44& transform)
{
	CameraSettings& settings = cameraAllocator.Get<0>(id.id);
	cameraAllocator.Get<2>(id.id) = transform;
}

//------------------------------------------------------------------------------
/**
*/
const CameraSettings&
CameraGetSettings(CameraId id)
{
	return cameraAllocator.Get<0>(id.id);
}

//------------------------------------------------------------------------------
/**
*/
const Math::matrix44&
CameraGetTransform(CameraId id)
{
	return cameraAllocator.Get<2>(id.id);
}

//------------------------------------------------------------------------------
/**
*/
const Math::matrix44&
CameraGetProjection(CameraId id)
{
	return cameraAllocator.Get<1>(id.id);
}

} // namespace Graphics