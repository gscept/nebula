#pragma once
//------------------------------------------------------------------------------
/**
    @class Importer::Clip
    
    Encapsulates an animation clip
    
    (C) 2012-2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "clipevent.h"

namespace ToolkitUtil
{

class Take;
class Clip : public Core::RefCounted
{
	__DeclareClass(Clip);
public:

	enum InfinityType
	{
		Constant,
		Cycle
	};

	/// constructor
	Clip();
	/// destructor
	virtual ~Clip();

    /// cleans up clip
    void Cleanup();

	/// sets the name
	void SetName(const Util::String& name);
	/// gets the name
	const Util::String& GetName() const;
    /// sets the take
    void SetTake(const Ptr<Take>& take);
    /// gets the take
    const Ptr<Take>& GetTake() const;
	/// sets the start time
	void SetStart(int start);
	/// gets the start value
	const int GetStart() const;
	/// sets the end time
	void SetEnd(int end);
	/// gets the end time
	const int GetEnd() const;
	/// sets the pre infinity type
	void SetPreInfinity(InfinityType infinityType);
	/// gets the pre infinity type
	const InfinityType GetPreInfinity() const;
	/// sets the post infinity type
	void SetPostInfinity(InfinityType infinityType);
	/// gets the post infinity type
	const InfinityType GetPostInfinity() const;

    /// gets list of events
    const Util::Array<Ptr<ClipEvent>>& GetEvents() const;
    /// get number of events
    const SizeT GetNumEvents() const;
    /// adds event to clip
    void AddEvent(const Ptr<ClipEvent>& event);
    /// removes event from clip
    void RemoveEvent(const Ptr<ClipEvent>& event);
private:
	Util::String name;
	int start;
	int end;
	InfinityType preInfinity;
	InfinityType postInfinity;
    Ptr<Take> take;
    Util::Array<Ptr<ClipEvent>> events;

};

//------------------------------------------------------------------------------
/**
*/
inline void 
Clip::SetName( const Util::String& name )
{
    this->name = name;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String& 
Clip::GetName() const
{
    return this->name;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
Clip::SetTake( const Ptr<Take>& take )
{
    n_assert(take.isvalid());
    this->take = take;
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<Take>& 
Clip::GetTake() const
{
    return this->take;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
Clip::SetPreInfinity( Clip::InfinityType infinityType )
{
    this->preInfinity = infinityType;
}

//------------------------------------------------------------------------------
/**
*/
inline const Clip::InfinityType 
Clip::GetPreInfinity() const
{
    return this->preInfinity;
}


//------------------------------------------------------------------------------
/**
*/
inline void 
Clip::SetPostInfinity(  Clip::InfinityType infinityType )
{
    this->postInfinity = infinityType;
}

//------------------------------------------------------------------------------
/**
*/
inline const Clip::InfinityType 
Clip::GetPostInfinity() const
{
    return this->postInfinity;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<Ptr<ClipEvent>>& 
Clip::GetEvents() const
{
    return this->events;
}

//------------------------------------------------------------------------------
/**
*/
inline const SizeT 
Clip::GetNumEvents() const
{
    return this->events.Size();
}

} // namespace ToolkitUtil