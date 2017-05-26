//------------------------------------------------------------------------------
// frameevent.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "frameevent.h"

namespace Frame2
{

__ImplementClass(Frame2::FrameEvent, 'FREV', Frame2::FrameOp);
//------------------------------------------------------------------------------
/**
*/
FrameEvent::FrameEvent()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
FrameEvent::~FrameEvent()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
FrameEvent::AddAction(const Action a)
{
	this->actions.Append(a);
}

//------------------------------------------------------------------------------
/**
*/
void
FrameEvent::Discard()
{
	this->event = 0;
}

//------------------------------------------------------------------------------
/**
*/
void
FrameEvent::Run(const IndexT frameIndex)
{
	IndexT i;
	for (i = 0; i < this->actions.Size(); i++)
	{
		switch (this->actions[i])
		{
		case Set:
			this->event->Signal();
			break;
		case Reset:
			this->event->Reset();
			break;
		case Wait:
			this->event->Wait();
			break;
		}
	}
}

} // namespace Frame2