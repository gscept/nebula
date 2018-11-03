#pragma once
//------------------------------------------------------------------------------
/**
	@class Dynui::ImguiAddon
	

	
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

private:
	Ptr<ImguiInputHandler> inputHandler;
};
} // namespace Dynui