//------------------------------------------------------------------------------
//  take.cc
//  (C) 2012-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "take.h"
#include "clip.h"
namespace ToolkitUtil
{

__ImplementClass(ToolkitUtil::Take, 'TAKE', Core::RefCounted);
//------------------------------------------------------------------------------
/**
*/
Take::Take()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
Take::~Take()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void 
Take::Cleanup()
{
    IndexT i;
    for (i = 0; i < this->clips.Size(); i++)
    {
        this->clips[i]->Cleanup();
    }
    this->clips.Clear();
}

//------------------------------------------------------------------------------
/**
*/
void 
Take::AddClip( const Ptr<Clip>& clip )
{
	this->clips.Append(clip);
}

//------------------------------------------------------------------------------
/**
*/
void 
Take::RemoveClip( const uint index )
{
	this->clips.EraseIndex(index);
}

//------------------------------------------------------------------------------
/**
*/
void 
Take::RemoveClip( const Ptr<Clip>& clip )
{
    IndexT i = this->clips.FindIndex(clip);
    n_assert(i != InvalidIndex);
    this->clips.EraseIndex(i);
}

//------------------------------------------------------------------------------
/**
*/
const Ptr<Clip>& 
Take::GetClip( const uint index )
{
	return this->clips[index];
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<Ptr<Clip> >& 
Take::GetClips() const
{
	return this->clips;
}

//------------------------------------------------------------------------------
/**
*/
void 
Take::SetName( const Util::String& name )
{
	this->name = name;
}

//------------------------------------------------------------------------------
/**
*/
const Util::String& 
Take::GetName() const
{
	return this->name;
}

//------------------------------------------------------------------------------
/**
*/
const SizeT 
Take::GetNumClips()
{
	return this->clips.Size();
}

//------------------------------------------------------------------------------
/**
*/
const IndexT 
Take::FindClip( const Ptr<Clip>& clip )
{
	return this->clips.FindIndex(clip);
}
}