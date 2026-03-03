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

__ImplementClass(Graphics::View, 'VIEW', Core::RefCounted);
//------------------------------------------------------------------------------
/**
*/
View::View() :
    func(nullptr),
    camera(GraphicsEntityId::Invalid()),
    enabled(true),
    stageMask(Graphics::PRIMARY_STAGE_MASK)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
View::~View()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
View::UpdateConstants()
{
    if (this->camera != GraphicsEntityId::Invalid())
    {
        // update camera
        auto settings = CameraContext::GetSettings(this->camera);

        Shared::ViewConstants::STRUCT constants = Graphics::GetViewConstants();
        Math::mat4 view = CameraContext::GetView(this->camera);
        Math::mat4 proj = CameraContext::GetProjection(this->camera);
        proj.row1 = -proj.row1;
        Math::mat4 viewProj = proj * view;
        Math::mat4 invView = Math::inverse(view);
        Math::mat4 invProj = Math::inverse(proj);
        Math::mat4 invViewProj = Math::inverse(viewProj);

        // update block structure
        view.store(&constants.View[0][0]);
        proj.store(&constants.Projection[0][0]);
        viewProj.store(&constants.ViewProjection[0][0]);
        invView.store(&constants.InvView[0][0]);
        invProj.store(&constants.InvProjection[0][0]);
        invViewProj.store(&constants.InvViewProjection[0][0]);
        invView.position.store(constants.EyePos);

        constants.FocalLengthNearFar[0] = settings.GetFocalLength().x;
        constants.FocalLengthNearFar[1] = settings.GetFocalLength().y;
        constants.FocalLengthNearFar[2] = settings.GetZNear();
        constants.FocalLengthNearFar[3] = settings.GetZFar();
        constants.Time_Random_Luminance_X[0] = (float)FrameSync::FrameSyncTimer::Instance()->GetTime();
        constants.Time_Random_Luminance_X[1] = Math::rand(0, 1);

        view.store(&this->viewConstants.View[0][0]);
        proj.store(&this->viewConstants.Projection[0][0]);
        viewProj.store(&this->viewConstants.ViewProjection[0][0]);
        invView.store(&this->viewConstants.InvView[0][0]);
        invProj.store(&this->viewConstants.InvProjection[0][0]);
        invViewProj.store(&this->viewConstants.InvViewProjection[0][0]);
        invView.position.store(this->viewConstants.EyePos);

        this->viewConstants.FocalLengthNearFar[0] = settings.GetFocalLength().x;
        this->viewConstants.FocalLengthNearFar[1] = settings.GetFocalLength().y;
        this->viewConstants.FocalLengthNearFar[2] = settings.GetZNear();
        this->viewConstants.FocalLengthNearFar[3] = settings.GetZFar();
        this->viewConstants.Time_Random_Luminance_X[0] = (float)FrameSync::FrameSyncTimer::Instance()->GetTime();
        this->viewConstants.Time_Random_Luminance_X[1] = Math::rand(0, 1);

        this->constantBufferOffsets.viewOffset = CoreGraphics::AllocateConstantBufferMemory(sizeof(this->viewConstants));

        // Apply view transforms
        Graphics::UpdateViewConstants(constants);
    }
}

//------------------------------------------------------------------------------
/**
*/
bool
View::Render(const IndexT frameIndex, const Timing::Time time, const IndexT bufferIndex)
{
    // run the actual script
    if (this->func != nullptr)
    {
        N_SCOPE(ViewExecute, Graphics);
        return this->func(this->viewport, frameIndex, bufferIndex);
    }
    return false;
}


} // namespace Graphics
