//------------------------------------------------------------------------------
// view.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "view.h"
#include "coregraphics/graphicsdevice.h"
#include "coregraphics/displaydevice.h"
#include "coregraphics/transformdevice.h"
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
	camera(CameraId::Invalid()),
	stage(nullptr)
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

	if (this->camera != CameraId::Invalid() && CoreGraphics::BeginFrame(frameIndex))
	{
		n_assert(this->stage.isvalid());
		n_assert(this->script.isvalid());
		transDev->SetViewTransform(CameraGetTransform(this->camera));
		transDev->SetProjTransform(CameraGetProjection(this->camera));
		transDev->SetFocalLength(CameraGetSettings(this->camera).GetFocalLength());
		transDev->ApplyViewSettings();
		this->script->Run(frameIndex);
		CoreGraphics::EndFrame(frameIndex);
		CoreGraphics::Present();
	}
}
} // namespace Graphics