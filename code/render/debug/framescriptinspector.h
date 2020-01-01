#pragma once
//------------------------------------------------------------------------------
/**
	ImGUI debug interface for inspecting frame scripts

	(C) 2018-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"

namespace Frame
{
class FrameScript;
}

namespace Debug
{

class FrameScriptInspector
{
public:
	/// run per frame
	static void Run(const Ptr<Frame::FrameScript>& script);
private:
};
} // namespace Debug
