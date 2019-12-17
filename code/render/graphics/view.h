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
	
	/// begin frame
	void BeginFrame(const IndexT frameIndex, const Timing::Time time);
	/// render through view
	void Render(const IndexT frameIndex, const Timing::Time time);
	/// end frame
	void EndFrame(const IndexT frameIndex, const Timing::Time time);

	/// get frame script
	const Ptr<Frame::FrameScript> GetFrameScript() const;

	/// set camera
	void SetCamera(const GraphicsEntityId& camera);
	/// get camera
	const GraphicsEntityId& GetCamera();

	/// set stage
	void SetStage(const Ptr<Stage>& stage);
	/// get stage
	const Ptr<Stage>& GetStage() const;

	/// returns whether view is enabled
	bool IsEnabled() const;
	
	/// enable this view
	void Enable();

	/// disable this view
	void Disable();
private:	
	friend class GraphicsServer;

	Ptr<Frame::FrameScript> script;
	GraphicsEntityId camera;
	Ptr<Stage> stage;
	bool inBeginFrame;
	bool enabled;
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

//------------------------------------------------------------------------------
/**
*/
inline bool
View::IsEnabled() const
{
	return this->enabled;
}

//------------------------------------------------------------------------------
/**
*/
inline void
View::Enable()
{
	this->enabled = true;
}

//------------------------------------------------------------------------------
/**
*/
inline void
View::Disable()
{
	this->enabled = false;
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<Frame::FrameScript> 
View::GetFrameScript() const
{
	return this->script;
}

} // namespace Graphics