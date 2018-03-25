//------------------------------------------------------------------------------
// view.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "view.h"
#include "coregraphics/renderdevice.h"
#include "coregraphics/displaydevice.h"

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
	RenderDevice* renderDevice = RenderDevice::Instance();
	DisplayDevice* displayDevice = DisplayDevice::Instance();

	if (this->camera != CameraId::Invalid() && renderDevice->BeginFrame(frameIndex))
	{
		n_assert(this->stage.isvalid());
		n_assert(this->script.isvalid());
		this->script->Run(frameIndex);
		renderDevice->EndFrame(frameIndex);
	}
}
} // namespace Graphics