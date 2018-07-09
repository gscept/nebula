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
#include "camera.h"
#include "stage.h"
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

	/// set camera
	void SetCamera(const CameraId& camera);
	/// get camera
	const CameraId& GetCamera();

	/// set stage
	void SetStage(const Ptr<Stage>& stage);
	/// get stage
	const Ptr<Stage>& GetStage() const;
private:	
	friend class GraphicsServer;

	Ptr<Frame::FrameScript> script;
	CameraId camera;
	Ptr<Stage> stage;
};

//------------------------------------------------------------------------------
/**
*/
inline void
View::SetCamera(const CameraId& camera)
{
	this->camera = camera;
}

//------------------------------------------------------------------------------
/**
*/
inline const CameraId&
Graphics::View::GetCamera()
{
	return this->camera;
}

//------------------------------------------------------------------------------
/**
*/
inline void
View::SetStage(const Ptr<Stage>& stage)
{
	this->stage = stage;
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<Stage>&
View::GetStage() const
{
	return this->stage;
}

} // namespace Graphics