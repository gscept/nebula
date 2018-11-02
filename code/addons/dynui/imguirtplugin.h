#pragma once
//------------------------------------------------------------------------------
/**
	@class Imgui::ImguiRTPlugin
	
	Use this class to integrate Imgui with the Nebula rendering loop.
	
	(C) 2012-2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "rendermodules/rt/rtplugin.h"
#include "imguirenderer.h"
namespace Dynui
{
class ImguiRTPlugin : public RenderModules::RTPlugin
{
	__DeclareClass(ImguiRTPlugin);
public:
	/// constructor
	ImguiRTPlugin();
	/// destructor
	virtual ~ImguiRTPlugin();

	/// called when plugin is registered on the render-thread side
	virtual void OnRegister();
	/// called when plugin is unregistered on the render-thread side
	virtual void OnUnregister();

	/// called when rendering a frame batch
	virtual void OnRenderFrameBatch(const Ptr<Frame::FrameBatch>& frameBatch);

	/// called if the window size has changed
	virtual void OnWindowResized(IndexT windowId, SizeT width, SizeT height);

private:
	Ptr<ImguiRenderer> renderer;
};
} // namespace Imgui