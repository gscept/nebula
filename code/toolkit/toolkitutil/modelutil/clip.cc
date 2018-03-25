//------------------------------------------------------------------------------
//  clip.cc
//  (C) 2012-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "clip.h"
#include "take.h"

namespace ToolkitUtil
{
__ImplementClass(ToolkitUtil::Clip, 'CLIP', Core::RefCounted);
//------------------------------------------------------------------------------
/**
*/
Clip::Clip() :
    take(0)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
Clip::~Clip()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void 
Clip::Cleanup()
{
    this->take = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
void 
Clip::SetStart( int start )
{
	this->start = start;
}

//------------------------------------------------------------------------------
/**
*/
const int 
Clip::GetStart() const
{
	return this->start;
}

//------------------------------------------------------------------------------
/**
*/
void 
Clip::SetEnd( int end )
{
	this->end = end;
}

//------------------------------------------------------------------------------
/**
*/
const int 
Clip::GetEnd() const
{
	return this->end;
}

//------------------------------------------------------------------------------
/**
*/
void 
Clip::AddEvent( const Ptr<ClipEvent>& event )
{
    n_assert(this->events.FindIndex(event) == InvalidIndex);
    this->events.Append(event);
}

//------------------------------------------------------------------------------
/**
*/
void 
Clip::RemoveEvent( const Ptr<ClipEvent>& event )
{
    IndexT index = this->events.FindIndex(event);
    n_assert(index != InvalidIndex);
    this->events.EraseIndex(index);
}

} // namespace ToolkitUtil