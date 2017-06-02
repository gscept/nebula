#pragma once
//------------------------------------------------------------------------------
/**
	A view describes a camera which can observe a Stage. The view processes 
	the attached Stage through its FrameScript each frame.
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "frame/framescript.h"
namespace Graphics
{
class Stage;
class View : public Core::RefCounted
{
	__DeclareClass(View);
public:
	/// constructor
	View();
	/// destructor
	virtual ~View();

private:

	Ptr<Frame::FrameScript> script;
	Ptr<Stage> stage;
};
} // namespace Graphics