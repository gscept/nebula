//------------------------------------------------------------------------------
//  imguifeatureunit.cc
//  (C) 2012-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "imguiaddon.h"
#include "rendermodules/rt/rtpluginregistry.h"
#include "input/inputserver.h"
#include "imguirtplugin.h"
#include "imgui/imgui.h"
#include "framesync/framesynctimer.h"

namespace Dynui
{
__ImplementClass(Dynui::ImguiAddon, 'IMFU', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
ImguiAddon::ImguiAddon()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
ImguiAddon::~ImguiAddon()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
ImguiAddon::Setup()
{
	// register render plugin
	RenderModules::RTPluginRegistry::Instance()->RegisterRTPlugin(&ImguiRTPlugin::RTTI);

	// create and register input handler
	this->inputHandler = ImguiInputHandler::Create();
	Input::InputServer::Instance()->AttachInputHandler(Input::InputPriority::DynUi, this->inputHandler.upcast<Input::InputHandler>());
}

//------------------------------------------------------------------------------
/**
*/
void
ImguiAddon::Discard()
{
	// remove input handler
	Input::InputServer::Instance()->RemoveInputHandler(this->inputHandler.upcast<Input::InputHandler>());
	this->inputHandler = nullptr;

	// remove RT plugin
	RenderModules::RTPluginRegistry::Instance()->UnregisterRTPlugin(&ImguiRTPlugin::RTTI);
}

//------------------------------------------------------------------------------
/**
*/
void
ImguiAddon::BeginFrame()
{
	// update frame time
	Timing::Time frameTime = FrameSync::FrameSyncTimer::Instance()->GetFrameTime();
	ImGui::GetIO().DeltaTime = (float)frameTime;

	// start new frame
	ImGui::NewFrame();
}

//------------------------------------------------------------------------------
/**
*/
void
ImguiAddon::EndFrame()
{
}

} // namespace Dynui