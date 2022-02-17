//------------------------------------------------------------------------------
// view.cc
// (C)2017-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "view.h"
#include "coregraphics/graphicsdevice.h"
#include "coregraphics/displaydevice.h"
#include "coregraphics/transformdevice.h"
#include "cameracontext.h"
#include "stage.h"

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
    script(nullptr),
    camera(GraphicsEntityId::Invalid()),
    stage(nullptr),
    enabled(true),
    inBeginFrame(false)
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
View::UpdateResources(const IndexT frameIndex, const IndexT bufferIndex)
{
    if (this->camera != GraphicsEntityId::Invalid())
    {
        // update camera
        TransformDevice* transDev = TransformDevice::Instance();
        auto settings = CameraContext::GetSettings(this->camera);
        transDev->SetViewTransform(CameraContext::GetView(this->camera));
        transDev->SetProjTransform(CameraContext::GetProjection(this->camera));
        transDev->SetFocalLength(settings.GetFocalLength());
        transDev->SetNearFarPlane(Math::vec2(settings.GetZNear(), settings.GetZFar()));

        // fixme! view should hold its own resource tables and send them to ApplyViewSettings!
        transDev->ApplyViewSettings();
    }   
}

//------------------------------------------------------------------------------
/**
*/
void 
View::BeginFrame(const IndexT frameIndex, const Timing::Time time, const IndexT bufferIndex)
{
    n_assert(!inBeginFrame);
    DisplayDevice* displayDevice = DisplayDevice::Instance();
    if (this->camera != GraphicsEntityId::Invalid())
    {
        //n_assert(this->stage.isvalid()); // hmm, we never use stages
        inBeginFrame = true;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
View::Render(const IndexT frameIndex, const Timing::Time time, const IndexT bufferIndex)
{
    n_assert(inBeginFrame);

    // run the actual script
    if (this->script != nullptr)
    {
        N_SCOPE(ViewExecute, Graphics);
        this->script->Run(frameIndex, bufferIndex);
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
View::EndFrame(const IndexT frameIndex, const Timing::Time time, const IndexT bufferIndex)
{
    n_assert(inBeginFrame);
    inBeginFrame = false;
}


//------------------------------------------------------------------------------
/**
*/
void
View::BuildFrameScript()
{
    this->script->Build();
}

} // namespace Graphics
