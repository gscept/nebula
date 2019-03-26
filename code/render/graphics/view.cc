//------------------------------------------------------------------------------
// view.cc
// (C)2017-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "view.h"
#include "coregraphics/graphicsdevice.h"
#include "coregraphics/displaydevice.h"
#include "coregraphics/transformdevice.h"
#include "cameracontext.h"
#include "stage.h"

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
	enabled(true)
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
View::Render(const IndexT frameIndex, const Timing::Time time)
{
	DisplayDevice* displayDevice = DisplayDevice::Instance();
	TransformDevice* transDev = TransformDevice::Instance();

	if (this->camera != GraphicsEntityId::Invalid() && CoreGraphics::BeginFrame(frameIndex))
	{
		n_assert(this->stage.isvalid());
		n_assert(this->script.isvalid());
		auto settings = CameraContext::GetSettings(this->camera);
		transDev->SetViewTransform(CameraContext::GetTransform(this->camera));
		transDev->SetProjTransform(CameraContext::GetProjection(this->camera));
		transDev->SetFocalLength(settings.GetFocalLength());
		transDev->SetNearFarPlane(Math::float2(settings.GetZNear(), settings.GetZFar()));
		transDev->ApplyViewSettings();
		this->script->Run(frameIndex);
		CoreGraphics::EndFrame(frameIndex);
		CoreGraphics::Present();
	}
}
} // namespace Graphics