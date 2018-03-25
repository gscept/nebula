#pragma once
//------------------------------------------------------------------------------
/**
    @class Importer::Take
    
    Handles an animation take, or animation clip-set
    
    (C) 2012-2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "util/string.h"
#include "util/array.h"
#include "core/refcounted.h"
namespace ToolkitUtil
{

class Clip;
class Take : public Core::RefCounted
{
	__DeclareClass(Take);
public:
	/// constructor
	Take();
	/// destructor
	~Take();

    /// cleanup take
    void Cleanup();

	/// adds a clip to the take
	void AddClip(const Ptr<Clip>& clip);
    /// removes clip from the take
    void RemoveClip(const Ptr<Clip>& clip);
	/// removes clip from the take
	void RemoveClip(const uint index);
    
	/// gets clip from the take
	const Ptr<Clip>& GetClip(const uint index);
	/// gets all clips from the take
	const Util::Array<Ptr<Clip> >& GetClips() const;
	/// gets the number of clips
	const SizeT GetNumClips();
	/// gets the index of a clip, returns InvalidIndex if clip isn't found
	const IndexT FindClip(const Ptr<Clip>& clip);

	/// sets the take name
	void SetName(const Util::String& name);
	/// gets the take name
	const Util::String& GetName() const;
private:
	Util::String name;
	Util::Array<Ptr<Clip> > clips;
};
}