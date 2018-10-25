//------------------------------------------------------------------------------
// frameevent.cc
// (C) 2016-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "frameevent.h"

namespace Frame
{

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
FrameEvent::Discard()
{
	this->event = CoreGraphics::EventId::Invalid();
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
			CoreGraphics::EventSignal(this->event, this->queueType, this->dependency);
			break;
		case Reset:
			CoreGraphics::EventReset(this->event, this->queueType, this->dependency);
			break;
		case Wait:
			CoreGraphics::EventWait(this->event, this->queueType);
			break;
		}
	}
}

} // namespace Frame2