//------------------------------------------------------------------------------
//  clipevent.cc
//  (C) 2015-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "clipevent.h"

namespace ToolkitUtil
{
__ImplementClass(ToolkitUtil::ClipEvent, 'CLEV', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
ClipEvent::ClipEvent()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
ClipEvent::~ClipEvent()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void 
ClipEvent::SetMarkerAsMilliseconds( Timing::Tick time )
{
    this->time = time;
    this->markerType = Ticks;
}

//------------------------------------------------------------------------------
/**
*/
void 
ClipEvent::SetMarkerAsFrames( int frame )
{
    this->time = frame;
    this->markerType = Frames;
}

} // namespace ToolkitUtil