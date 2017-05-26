#pragma once
//------------------------------------------------------------------------------
/**
    @class Models::VisResolveContainer
    
    Helper class which keeps an array of visible nodes by material type.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/ptr.h"
#include "graphics/batchgroup.h"
#include "materials/surfacename.h"

//------------------------------------------------------------------------------
namespace Models
{
template<class TYPE, class IDENTIFIER, int ID_MAX_VALUE>
class VisResolveContainer
{
public:
    /// constructor
    VisResolveContainer();
    /// reset content
    void Reset();
	/// set the resolved flag for a given ModelNodeMaterial
	void SetResolved(const IDENTIFIER& code, bool b);
	/// return true if the resolved flag has been set for ModelNodeMaterial
	bool IsResolved(const IDENTIFIER& code) const;
    /// add a visible element by ModelNodeType
	void Add(IndexT frameIndex, const IDENTIFIER& code, const Ptr<TYPE>& e);
    /// get all visible elements of given ModelNodeType
	const Util::Array<Ptr<TYPE> >& Get(const IDENTIFIER& code) const;

	static const IndexT numEntries = ID_MAX_VALUE;

private:
    struct Entry
    {
        /// constructor
        Entry() : resolved(false) {};

        Util::Array<Ptr<TYPE>> nodes;
        bool resolved;
    };
    Util::FixedArray<Entry> entries;
    IndexT frameIndex;
};

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, class IDENTIFIER, int ID_MAX_VALUE>
VisResolveContainer<TYPE, IDENTIFIER, ID_MAX_VALUE>::VisResolveContainer() :
	entries(numEntries),
    frameIndex(InvalidIndex)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, class IDENTIFIER, int ID_MAX_VALUE>
inline void
VisResolveContainer<TYPE, IDENTIFIER, ID_MAX_VALUE>::SetResolved(const IDENTIFIER& code, bool b)
{
    this->entries[code].resolved = b;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, class IDENTIFIER, int ID_MAX_VALUE>
inline bool
VisResolveContainer<TYPE, IDENTIFIER, ID_MAX_VALUE>::IsResolved(const IDENTIFIER& code) const
{
    return this->entries[code].resolved;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, class IDENTIFIER, int ID_MAX_VALUE>
inline void
VisResolveContainer<TYPE, IDENTIFIER, ID_MAX_VALUE>::Reset()
{
    IndexT i;
    for (i = 0; i < numEntries; i++)
    {
        this->entries[i].resolved = false;
        this->entries[i].nodes.Clear();
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, class IDENTIFIER, int ID_MAX_VALUE>
inline void
VisResolveContainer<TYPE, IDENTIFIER, ID_MAX_VALUE>::Add(IndexT curFrameIndex, const IDENTIFIER& code, const Ptr<TYPE>& e)
{
    if (curFrameIndex != this->frameIndex)
    {
        this->Reset();
        this->frameIndex = curFrameIndex;
    }
    this->entries[code].nodes.Append(e);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, class IDENTIFIER, int ID_MAX_VALUE>
inline const Util::Array<Ptr<TYPE> >&
VisResolveContainer<TYPE, IDENTIFIER, ID_MAX_VALUE>::Get(const IDENTIFIER& code) const
{
    return this->entries[code].nodes;
}

} // namespace Models
//------------------------------------------------------------------------------
    