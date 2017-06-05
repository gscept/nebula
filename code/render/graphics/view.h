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
#include "timing/time.h"
namespace Graphics
{
class Stage;
class Camera;
class View : public Core::RefCounted
{
	__DeclareClass(View);
public:
	/// constructor
	View();
	/// destructor
	virtual ~View();
	
	/// render through view
	void Render(const IndexT frameIndex, const Timing::Time time);

	Ptr<Frame::FrameScript> script;
	Ptr<Camera> camera;
	Ptr<Stage> stage;
private:

	

};

} // namespace Graphics