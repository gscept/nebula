//------------------------------------------------------------------------------
//  cameracontext.cc
//  (C) 2018-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "cameracontext.h"
#include "graphics/graphicsserver.h"

namespace Graphics
{

CameraContext::CameraAllocator CameraContext::cameraAllocator;
Graphics::GraphicsEntityId CameraContext::lodCamera;
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

    __bundle.OnBegin = CameraContext::UpdateCameras;
    __bundle.OnWindowResized = CameraContext::OnWindowResized;
    Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&__bundle, &__state);
}

//------------------------------------------------------------------------------
/**
*/
void
CameraContext::UpdateCameras(const Graphics::FrameContext& ctx)
{
    const Util::Array<Math::mat4>& proj = cameraAllocator.GetArray<Camera_Projection>();
    const Util::Array<Math::mat4>& views = cameraAllocator.GetArray<Camera_View>();
    const Util::Array<Math::mat4>& viewproj = cameraAllocator.GetArray<Camera_ViewProjection>();

    IndexT i;
    for (i = 0; i < viewproj.Size(); i++)
    {
        viewproj[i] = views[i] * proj[i];
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
CameraContext::SetTransform(const Graphics::GraphicsEntityId id, const Math::mat4& mat)
{
    const ContextEntityId cid = GetContextId(id);
    cameraAllocator.Set<Camera_View>(cid.id, mat);
}

//------------------------------------------------------------------------------
/**
*/
const Math::mat4&
CameraContext::GetTransform(const Graphics::GraphicsEntityId id)
{
    const ContextEntityId cid = GetContextId(id);
    return cameraAllocator.Get<Camera_View>(cid.id);
}

//------------------------------------------------------------------------------
/**
*/
const Math::mat4&
CameraContext::GetProjection(const Graphics::GraphicsEntityId id)
{
    const ContextEntityId cid = GetContextId(id);
    return cameraAllocator.Get<Camera_Projection>(cid.id);
}

//------------------------------------------------------------------------------
/**
*/
const Math::mat4&
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

//------------------------------------------------------------------------------
/**
*/
Graphics::GraphicsEntityId 
CameraContext::GetLODCamera()
{
    return lodCamera;
}

//------------------------------------------------------------------------------
/**
*/
void 
CameraContext::SetLODCamera(const Graphics::GraphicsEntityId id)
{
    lodCamera = id;
}

} // namespace Graphics
