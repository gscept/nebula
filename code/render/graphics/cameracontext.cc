//------------------------------------------------------------------------------
//  cameracontext.cc
//  (C) 2018-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "cameracontext.h"
#include "graphics/graphicsserver.h"

namespace Graphics
{

CameraContext::CameraAllocator CameraContext::cameraAllocator;
Util::Array<Graphics::GraphicsEntityId> CameraContext::LodCameras;
Graphics::GraphicsEntityId CameraContext::lodCamera;
__ImplementContext(CameraContext, CameraContext::cameraAllocator);

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
    __CreateContext();

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
        viewproj[i] = proj[i] * views[i];
    }
}

//------------------------------------------------------------------------------
/**
*/
void
CameraContext::SetupProjectionFov(const Graphics::GraphicsEntityId id, float aspect, float fov, float znear, float zfar, const Graphics::StageMask stageMask)
{
    const ContextEntityId cid = GetContextId(id);
    CameraSettings& settings = cameraAllocator.Get<Camera_Settings>(cid.id);
    settings.SetupPerspectiveFov(fov, aspect, znear, zfar);
    cameraAllocator.Set<Camera_Projection>(cid.id, settings.GetProjTransform());
    cameraAllocator.Set<Camera_StageMask>(cid.id, stageMask);
}

//------------------------------------------------------------------------------
/**
*/
void
CameraContext::SetupOrthographic(const Graphics::GraphicsEntityId id, float width, float height, float znear, float zfar, const Graphics::StageMask stageMask)
{
    const ContextEntityId cid = GetContextId(id);
    CameraSettings& settings = cameraAllocator.Get<Camera_Settings>(cid.id);
    settings.SetupOrthogonal(width, height, znear, zfar);
    cameraAllocator.Set<Camera_Projection>(cid.id, settings.GetProjTransform());
    cameraAllocator.Set<Camera_StageMask>(cid.id, stageMask);
}

//------------------------------------------------------------------------------
/**
*/
void
CameraContext::SetView(const Graphics::GraphicsEntityId id, const Math::mat4& mat)
{
    const ContextEntityId cid = GetContextId(id);
    cameraAllocator.Set<Camera_View>(cid.id, mat);
}

//------------------------------------------------------------------------------
/**
*/
const Math::mat4&
CameraContext::GetView(const Graphics::GraphicsEntityId id)
{
    const ContextEntityId cid = GetContextId(id);
    return cameraAllocator.Get<Camera_View>(cid.id);
}

//------------------------------------------------------------------------------
/**
*/
const Math::mat4
CameraContext::GetTransform(const Graphics::GraphicsEntityId id)
{
    const ContextEntityId cid = GetContextId(id);
    return inverse(cameraAllocator.Get<Camera_View>(cid.id));
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
Graphics::StageMask
CameraContext::GetStageMask(const Graphics::GraphicsEntityId id)
{
    const ContextEntityId cid = GetContextId(id);
    return cameraAllocator.Get<Camera_StageMask>(cid.id);
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<Graphics::GraphicsEntityId>&
CameraContext::GetLODCameras()
{
    return CameraContext::LodCameras;
}

//------------------------------------------------------------------------------
/**
*/
void
CameraContext::AddLODCamera(const Graphics::GraphicsEntityId id)
{
    CameraContext::LodCameras.Append(id);
}

//------------------------------------------------------------------------------
/**
*/
void
CameraContext::RemoveLODCamera(const Graphics::GraphicsEntityId id)
{
    IndexT i = CameraContext::LodCameras.FindIndex(id);
    n_assert(i != InvalidIndex);
    CameraContext::LodCameras.EraseIndex(i);
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
