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
View::UpdateResources(const IndexT frameIndex)
{
	if (this->camera != GraphicsEntityId::Invalid())
	{
		// update camera
		TransformDevice* transDev = TransformDevice::Instance();
		auto settings = CameraContext::GetSettings(this->camera);
		transDev->SetViewTransform(CameraContext::GetTransform(this->camera));
		transDev->SetProjTransform(CameraContext::GetProjection(this->camera));
		transDev->SetFocalLength(settings.GetFocalLength());
		transDev->SetNearFarPlane(Math::float2(settings.GetZNear(), settings.GetZFar()));

		// fixme! view should hold its own resource tables and send them to ApplyViewSettings!
		transDev->ApplyViewSettings();
	}	

	if (this->script)
	{
		this->script->UpdateResources(frameIndex);
		this->script->UpdateViewDependentResources(this, frameIndex);
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
View::BeginFrame(const IndexT frameIndex, const Timing::Time time)
{
	n_assert(!inBeginFrame);
	DisplayDevice* displayDevice = DisplayDevice::Instance();
	if (this->camera != GraphicsEntityId::Invalid())
	{
		//n_assert(this->stage.isvalid()); // hmm, we never use stages
		inBeginFrame = true;
	}

	// run script asynchronous jobs
	if (this->script != nullptr)
	{
		N_SCOPE(ViewRecord, Render);
		this->script->RunJobs(frameIndex);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
View::Render(const IndexT frameIndex, const Timing::Time time)
{
	n_assert(inBeginFrame);

	// run the actual script
	if (this->script != nullptr)
	{
		N_SCOPE(ViewExecute, Render);
		this->script->Run(frameIndex);
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
View::EndFrame(const IndexT frameIndex, const Timing::Time time)
{
	n_assert(inBeginFrame);
	inBeginFrame = false;
}
} // namespace Graphics