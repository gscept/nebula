#pragma once
//------------------------------------------------------------------------------
/**
	A frame event is a wait-signal-reset type event which can be used to wait for executions
	within a frame script.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "frameop.h"
#include "coregraphics/event.h"
namespace Frame
{
class FrameEvent : public FrameOp
{
	__DeclareClass(FrameEvent);
public:

	enum Action
	{
		Set,
		Reset,
		Wait
	};

	/// constructor
	FrameEvent();
	/// destructor
	virtual ~FrameEvent();

	/// add action to execute on the event
	void AddAction(const Action a);
	/// set event to use
	void SetEvent(const Ptr<CoreGraphics::Event>& event);

	/// discard operation
	void Discard();
	/// run operation
	void Run(const IndexT frameIndex);
private:
	Util::Array<Action> actions;
	Ptr<CoreGraphics::Event> event;
};

//------------------------------------------------------------------------------
/**
*/
inline void
FrameEvent::SetEvent(const Ptr<CoreGraphics::Event>& event)
{
	this->event = event;
}

} // namespace Frame2