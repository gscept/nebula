#pragma once
//------------------------------------------------------------------------------
/**
	@class Dynui::ImguiFeatureUnit
	
	Feature unit used for Imgui.	
	
	(C) 2012-2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "imguiinputhandler.h"

namespace Dynui
{
class ImguiAddon : public Core::RefCounted
{
	__DeclareClass(ImguiAddon);
public:
	/// constructor
	ImguiAddon();
	/// destructor
	virtual ~ImguiAddon();

	/// setup addon
	void Setup();
	/// discard addon
	void Discard();

	/// call before performing immediate UI rendering
	static void BeginFrame();
	/// call after frame is done, will reset all windows and inputs
	static void EndFrame();

private:
	Ptr<ImguiInputHandler> inputHandler;
};
} // namespace Dynui