//------------------------------------------------------------------------------
//  imguiinputhandler.cc
//  (C) 2012-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "imguiinputhandler.h"
#include "input/inputserver.h"
#include "imguirenderer.h"
#include "imgui.h"

namespace Dynui
{
__ImplementClass(Dynui::ImguiInputHandler, 'IMIH', Input::InputHandler);

//------------------------------------------------------------------------------
/**
*/
ImguiInputHandler::ImguiInputHandler()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
ImguiInputHandler::~ImguiInputHandler()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
ImguiInputHandler::BeginCapture()
{
	Input::InputServer::Instance()->ObtainMouseCapture(this);
	Input::InputServer::Instance()->ObtainKeyboardCapture(this);
}

//------------------------------------------------------------------------------
/**
*/
void
ImguiInputHandler::EndCapture()
{
	Input::InputServer::Instance()->ReleaseMouseCapture(this);
	Input::InputServer::Instance()->ReleaseKeyboardCapture(this);
}

//------------------------------------------------------------------------------
/**
*/
bool
ImguiInputHandler::OnEvent(const Input::InputEvent& inputEvent)
{
	const Ptr<ImguiRenderer>& renderer = ImguiRenderer::Instance();
	switch (inputEvent.GetType())
	{
#ifndef _DEBUG
	case Input::InputEvent::AppObtainFocus:
	case Input::InputEvent::AppLoseFocus:
#endif
	case Input::InputEvent::Reset:
		this->OnReset();
		break;

	default:
		return renderer->HandleInput(inputEvent);
	}
	return false;
}

} // namespace Dynui