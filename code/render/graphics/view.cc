//------------------------------------------------------------------------------
// view.cc
// (C)2017-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "view.h"
#include "cameracontext.h"

#include "graphics/globalconstants.h"

#ifndef PUBLIC_BUILD
#include "debug/framescriptinspector.h"
#endif

using namespace CoreGraphics;
namespace Graphics
{

ViewAllocator viewAllocator;
//__ImplementClass(Graphics::View, 'VIEW', Core::RefCounted);
////------------------------------------------------------------------------------
///**
//*/
//View::View() :
//    func(nullptr),
//    camera(GraphicsEntityId::Invalid()),
//    enabled(true),
//    stageMask(Graphics::PRIMARY_STAGE_MASK)
//{
//    // empty
//}
//
////------------------------------------------------------------------------------
///**
//*/
//View::~View()
//{
//    // empty
//}
//
////------------------------------------------------------------------------------
///**
//*/
//void
//View::UpdateConstants()
//{
//    if (this->camera != GraphicsEntityId::Invalid())
//    {
//        // update camera
//        auto settings = CameraContext::GetSettings(this->camera);
//
//        Math::mat4 view = CameraContext::GetView(this->camera);
//        Math::mat4 proj = CameraContext::GetProjection(this->camera);
//        proj.row1 = -proj.row1;
//        Math::mat4 viewProj = proj * view;
//        Math::mat4 invView = Math::inverse(view);
//        Math::mat4 invProj = Math::inverse(proj);
//        Math::mat4 invViewProj = Math::inverse(viewProj);
//
//        view.store(&this->viewConstants.View[0][0]);
//        proj.store(&this->viewConstants.Projection[0][0]);
//        viewProj.store(&this->viewConstants.ViewProjection[0][0]);
//        invView.store(&this->viewConstants.InvView[0][0]);
//        invProj.store(&this->viewConstants.InvProjection[0][0]);
//        invViewProj.store(&this->viewConstants.InvViewProjection[0][0]);
//        invView.position.store(this->viewConstants.EyePos);
//
//        this->viewConstants.FocalLengthNearFar[0] = settings.GetFocalLength().x;
//        this->viewConstants.FocalLengthNearFar[1] = settings.GetFocalLength().y;
//        this->viewConstants.FocalLengthNearFar[2] = settings.GetZNear();
//        this->viewConstants.FocalLengthNearFar[3] = settings.GetZFar();
//        this->viewConstants.Time_Random_Luminance_X[0] = (float)FrameSync::FrameSyncTimer::Instance()->GetTime();
//        this->viewConstants.Time_Random_Luminance_X[1] = Math::rand(0, 1);
//        this->viewConstants.StageMask = this->stageMask;
//
//        // Apply view transforms
//        Graphics::UpdateViewConstants(this->viewConstants);
//        Graphics::UpdateShadowConstants(this->shadowViewConstants);
//    }
//}
//
////------------------------------------------------------------------------------
///**
//*/
//bool
//View::Render(const IndexT frameIndex, const Timing::Time time, const IndexT bufferIndex)
//{
//    bool resize = false;
//    // run the actual script
//    if (this->func != nullptr)
//    {
//        N_SCOPE(ViewExecute, Graphics);
//        resize = this->func(this->viewport, frameIndex, bufferIndex);
//    }
//    return resize;
//}

//------------------------------------------------------------------------------
/**
*/
ViewId
CreateView(const ViewCreateInfo& info)
{
    Ids::Id32 id = viewAllocator.Alloc();
    viewAllocator.Set<View_FrameScriptFunc>(id, info.frameScript);
    viewAllocator.Set<View_Viewport>(id, info.viewport);
    viewAllocator.Set<View_Camera>(id, info.camera);
    viewAllocator.Set<View_StageMask>(id, info.stageMask);
    return ViewId(id);
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyView(const ViewId id)
{
    CoreGraphics::TextureId tex = viewAllocator.Get<View_OutputTarget>(id.id);
    if (tex != CoreGraphics::InvalidTextureId)
    {
        CoreGraphics::DestroyTexture(tex);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
ViewApply(const ViewId id)
{
    const GraphicsEntityId cam = viewAllocator.Get<View_Camera>(id.id);
    if (cam != GraphicsEntityId::Invalid())
    {
        Shared::ViewConstants::STRUCT& viewConstants = viewAllocator.Get<View_ViewConstants>(id.id);
        Shared::ShadowViewConstants::STRUCT& shadowViewConstants = viewAllocator.Get<View_ShadowConstants>(id.id);
        Graphics::StageMask stageMask = viewAllocator.Get<View_StageMask>(id.id);
        // update camera
        auto settings = CameraContext::GetSettings(cam);

        Math::mat4 view = CameraContext::GetView(cam);
        Math::mat4 proj = CameraContext::GetProjection(cam);
        proj.row1 = -proj.row1;
        Math::mat4 viewProj = proj * view;
        Math::mat4 invView = Math::inverse(view);
        Math::mat4 invProj = Math::inverse(proj);
        Math::mat4 invViewProj = Math::inverse(viewProj);

        view.store(&viewConstants.View[0][0]);
        proj.store(&viewConstants.Projection[0][0]);
        viewProj.store(&viewConstants.ViewProjection[0][0]);
        invView.store(&viewConstants.InvView[0][0]);
        invProj.store(&viewConstants.InvProjection[0][0]);
        invViewProj.store(&viewConstants.InvViewProjection[0][0]);
        invView.position.store(viewConstants.EyePos);

        viewConstants.FocalLengthNearFar[0] = settings.GetFocalLength().x;
        viewConstants.FocalLengthNearFar[1] = settings.GetFocalLength().y;
        viewConstants.FocalLengthNearFar[2] = settings.GetZNear();
        viewConstants.FocalLengthNearFar[3] = settings.GetZFar();
        viewConstants.Time_Random_Luminance_X[0] = (float)FrameSync::FrameSyncTimer::Instance()->GetTime();
        viewConstants.Time_Random_Luminance_X[1] = Math::rand(0, 1);
        viewConstants.StageMask = stageMask;

        // Apply view transforms
        Graphics::UpdateViewConstants(viewConstants);
        Graphics::UpdateShadowConstants(shadowViewConstants);
    }
}

//------------------------------------------------------------------------------
/**
*/
bool
ViewRender(const ViewId id, const IndexT frameIndex, const Timing::Time time, const IndexT bufferIndex)
{
    bool resize = false;

    auto func = viewAllocator.Get<View_FrameScriptFunc>(id.id);
    const Math::rectangle<int> viewport = viewAllocator.Get<View_Viewport>(id.id);
    // run the actual script
    if (func != nullptr)
    {
        N_SCOPE(ViewExecute, Graphics);
        resize = func(viewport, frameIndex, bufferIndex);
    }
    return resize;
}

//------------------------------------------------------------------------------
/**
*/
void
ViewSetFrameScript(const ViewId id, bool(*func)(const Math::rectangle<int>& viewport, IndexT frameIndex, IndexT bufferIndex))
{
    viewAllocator.Set<View_FrameScriptFunc>(id.id, func);
}

//------------------------------------------------------------------------------
/**
*/
void
ViewSetViewport(const ViewId id, const Math::rectangle<int>& rect)
{
    viewAllocator.Set<View_Viewport>(id.id, rect);
}

//------------------------------------------------------------------------------
/**
*/
const Math::rectangle<int>&
ViewGetViewport(const ViewId id)
{
    return viewAllocator.Get<View_Viewport>(id.id);
}

//------------------------------------------------------------------------------
/**
*/
void
ViewSetCamera(const ViewId id, const GraphicsEntityId& camera)
{
    viewAllocator.Set<View_Camera>(id.id, camera);
}

//------------------------------------------------------------------------------
/**
*/
const GraphicsEntityId&
ViewGetCamera(const ViewId id)
{
    return viewAllocator.Get<View_Camera>(id.id);
}

//------------------------------------------------------------------------------
/**
*/
Shared::ViewConstants::STRUCT&
ViewGetViewConstants(const ViewId id)
{
    return viewAllocator.Get<View_ViewConstants>(id.id);
}

//------------------------------------------------------------------------------
/**
*/
Shared::ShadowViewConstants::STRUCT&
ViewGetShadowConstants(const ViewId id)
{
    return viewAllocator.Get<View_ShadowConstants>(id.id);
}

//------------------------------------------------------------------------------
/**
*/
void
ViewSetStageMask(const ViewId id, const Graphics::StageMask stage)
{
    viewAllocator.Set<View_StageMask>(id.id, stage);
}

//------------------------------------------------------------------------------
/**
*/
const Graphics::StageMask
ViewGetStageMask(const ViewId id)
{
    return viewAllocator.Get<View_StageMask>(id.id);
}

//------------------------------------------------------------------------------
/**
*/
void
ViewEnable(const ViewId id)
{
    viewAllocator.Set<View_Enabled>(id.id, true);
}

//------------------------------------------------------------------------------
/**
*/
void
ViewDisable(const ViewId id)
{
    viewAllocator.Set<View_Enabled>(id.id, false);
}

//------------------------------------------------------------------------------
/**
*/
bool
ViewIsEnabled(const ViewId id)
{
    return viewAllocator.Get<View_Enabled>(id.id);
}

} // namespace Graphics
