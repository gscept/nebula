#pragma once
//------------------------------------------------------------------------------
/**
	A view describes a camera which can observe a Stage. The view processes 
	the attached Stage through its FrameScript each frame.
	
	(C)2017-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "frame/framescript.h"
#include "timing/time.h"
#include "graphicsentity.h"
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
	void SetCamera(const GraphicsEntityId& camera);
	/// get camera
	const GraphicsEntityId& GetCamera();

	/// set stage
	void SetStage(const Ptr<Stage>& stage);
	/// get stage
	const Ptr<Stage>& GetStage() const;
private:	
	friend class GraphicsServer;

	Ptr<Frame::FrameScript> script;
	GraphicsEntityId camera;
	Ptr<Stage> stage;
};

//------------------------------------------------------------------------------
/**
*/
inline void
View::SetCamera(const GraphicsEntityId& camera)
{
	this->camera = camera;
}

//------------------------------------------------------------------------------
/**
*/
inline const GraphicsEntityId&
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