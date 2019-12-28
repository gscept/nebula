//------------------------------------------------------------------------------
//  cameracontext.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "cameracontext.h"
#include "graphics/graphicsserver.h"

namespace Graphics
{

CameraContext::CameraAllocator CameraContext::cameraAllocator;
_ImplementContext(CameraContext, CameraContext::cameraAllocator);

//------------------------------------------------------------------------------
/**
*/
CameraContext::CameraContext()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
CameraContext::~CameraContext()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
CameraContext::Create()
{
	_CreateContext();

	__bundle.OnBeforeFrame = CameraContext::OnBeforeFrame;
	__bundle.OnWindowResized = CameraContext::OnWindowResized;
	Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&__bundle, &__state);
}

//------------------------------------------------------------------------------
/**
*/
void
CameraContext::OnBeforeFrame(const IndexT frameIndex, const Timing::Time frameTime, const Timing::Time time, const Timing::Tick ticks)
{
	const Util::Array<Math::matrix44>& proj = cameraAllocator.GetArray<Camera_Projection>();
	const Util::Array<Math::matrix44>& views = cameraAllocator.GetArray<Camera_View>();
	const Util::Array<Math::matrix44>& viewproj = cameraAllocator.GetArray<Camera_ViewProjection>();

	IndexT i;
	for (i = 0; i < viewproj.Size(); i++)
	{
		viewproj[i] = Math::matrix44::multiply(views[i], proj[i]);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
CameraContext::SetupProjectionFov(const Graphics::GraphicsEntityId id, float aspect, float fov, float znear, float zfar)
{
	const ContextEntityId cid = GetContextId(id);
	CameraSettings& settings = cameraAllocator.Get<Camera_Settings>(cid.id);
	settings.SetupPerspectiveFov(fov, aspect, znear, zfar);
	cameraAllocator.Get<Camera_Projection>(cid.id) = settings.GetProjTransform();
}

//------------------------------------------------------------------------------
/**
*/
void
CameraContext::SetupOrthographic(const Graphics::GraphicsEntityId id, float width, float height, float znear, float zfar)
{
	const ContextEntityId cid = GetContextId(id);
	CameraSettings& settings = cameraAllocator.Get<Camera_Settings>(cid.id);
	settings.SetupOrthogonal(width, height, znear, zfar);
	cameraAllocator.Get<Camera_Projection>(cid.id) = settings.GetProjTransform();
}

//------------------------------------------------------------------------------
/**
*/
void
CameraContext::SetTransform(const Graphics::GraphicsEntityId id, const Math::matrix44& mat)
{
	const ContextEntityId cid = GetContextId(id);
	cameraAllocator.Get<Camera_View>(cid.id) = mat;
}

//------------------------------------------------------------------------------
/**
*/
const Math::matrix44&
CameraContext::GetTransform(const Graphics::GraphicsEntityId id)
{
	const ContextEntityId cid = GetContextId(id);
	return cameraAllocator.Get<Camera_View>(cid.id);
}

//------------------------------------------------------------------------------
/**
*/
const Math::matrix44&
CameraContext::GetProjection(const Graphics::GraphicsEntityId id)
{
	const ContextEntityId cid = GetContextId(id);
	return cameraAllocator.Get<Camera_Projection>(cid.id);
}

//------------------------------------------------------------------------------
/**
*/
const Math::matrix44&
CameraContext::GetViewProjection(const Graphics::GraphicsEntityId id)
{
	const ContextEntityId cid = GetContextId(id);
	return cameraAllocator.Get<Camera_ViewProjection>(cid.id);
}

//------------------------------------------------------------------------------
/**
*/
const CameraSettings&
CameraContext::GetSettings(const Graphics::GraphicsEntityId id)
{
	const ContextEntityId cid = GetContextId(id);
	return cameraAllocator.Get<Camera_Settings>(cid.id);
}

//------------------------------------------------------------------------------
/**
*/
void 
CameraContext::OnWindowResized(const CoreGraphics::WindowId windowId, SizeT width, SizeT height)
{
	Util::Array<CameraSettings>& settings = cameraAllocator.GetArray<Camera_Settings>();
	IndexT i;
	for (i = 0; i < settings.Size(); i++)
	{
		CameraSettings& setting = settings[i];

		setting.SetupPerspectiveFov(setting.GetFov(), height / float(width), setting.GetZNear(), setting.GetZFar());
		cameraAllocator.GetArray<Camera_Projection>()[i] = setting.GetProjTransform();
	}

}

} // namespace Graphics
