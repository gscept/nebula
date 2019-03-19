//------------------------------------------------------------------------------
//  cameracontext.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "cameracontext.h"
#include "graphics/graphicsserver.h"

namespace Graphics
{

_ImplementContext(CameraContext);
CameraContext::CameraAllocator CameraContext::cameraAllocator;

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
	Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&__bundle, &__state);
}

//------------------------------------------------------------------------------
/**
*/
void
CameraContext::OnBeforeFrame(const IndexT frameIndex, const Timing::Time frameTime, const Timing::Time time, const Timing::Tick ticks)
{
	const Util::Array<Math::matrix44>& proj = cameraAllocator.GetArray<1>();
	const Util::Array<Math::matrix44>& views = cameraAllocator.GetArray<2>();
	const Util::Array<Math::matrix44>& viewproj = cameraAllocator.GetArray<3>();

	IndexT i;
	for (i = 0; i < viewproj.Size(); i++)
	{
		viewproj[i] = Math::matrix44::multiply(proj[i], views[i]);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
CameraContext::SetupProjectionFov(const Graphics::GraphicsEntityId id, float aspect, float fov, float znear, float zfar)
{
	const ContextEntityId cid = GetContextId(id);
	CameraSettings& settings = cameraAllocator.Get<0>(cid.id);
	settings.SetupPerspectiveFov(fov, aspect, znear, zfar);
	cameraAllocator.Get<1>(cid.id) = settings.GetProjTransform();
}

//------------------------------------------------------------------------------
/**
*/
void
CameraContext::SetupOrthographic(const Graphics::GraphicsEntityId id, float width, float height, float znear, float zfar)
{
	const ContextEntityId cid = GetContextId(id);
	CameraSettings& settings = cameraAllocator.Get<0>(cid.id);
	settings.SetupOrthogonal(width, height, znear, zfar);
	cameraAllocator.Get<1>(cid.id) = settings.GetProjTransform();
}

//------------------------------------------------------------------------------
/**
*/
void
CameraContext::SetTransform(const Graphics::GraphicsEntityId id, const Math::matrix44& mat)
{
	const ContextEntityId cid = GetContextId(id);
	cameraAllocator.Get<2>(cid.id) = mat;
}

//------------------------------------------------------------------------------
/**
*/
const Math::matrix44&
CameraContext::GetTransform(const Graphics::GraphicsEntityId id)
{
	const ContextEntityId cid = GetContextId(id);
	return cameraAllocator.Get<2>(cid.id);
}

//------------------------------------------------------------------------------
/**
*/
const Math::matrix44&
CameraContext::GetProjection(const Graphics::GraphicsEntityId id)
{
	const ContextEntityId cid = GetContextId(id);
	return cameraAllocator.Get<1>(cid.id);
}

//------------------------------------------------------------------------------
/**
*/
const Math::matrix44&
CameraContext::GetViewProjection(const Graphics::GraphicsEntityId id)
{
	const ContextEntityId cid = GetContextId(id);
	return cameraAllocator.Get<3>(cid.id);
}

//------------------------------------------------------------------------------
/**
*/
const CameraSettings&
CameraContext::GetSettings(const Graphics::GraphicsEntityId id)
{
	const ContextEntityId cid = GetContextId(id);
	return cameraAllocator.Get<0>(cid.id);
}

} // namespace Graphics
