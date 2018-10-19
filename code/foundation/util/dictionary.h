#pragma once
//------------------------------------------------------------------------------
/**
    @class Util::Dictionary
  
    A collection of key/value pairs with quick value retrieval
    by key at roughly O(log n). 

    Internally the dictionary is implemented as a sorted array.

    On insertion performance:
    Key/value pairs are inserted with the Add() method, which normally
    calls the Util::Array::InsertSorted() method internally. If many
    insertions are performed at once, it may be beneficial to call
    BeginBulkAdd() before, and EndBulkAdd() after adding the 
    key/value pairs. Between BeginBulkAdd() and EndBulkAdd(), the
    Add() method will just append the new elements to the internal
    array, and only call Util::Array::Sort() inside EndBulkAdd().

    Any methods which require the internal array to be sorted will
    throw an assertion between BeginBulkAdd() and EndBulkAdd().

    (C) 2006 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file	
*/    
#include "util/array.h"
#include "util/keyvaluepair.h"

//------------------------------------------------------------------------------
namespace Util
{
template<class KEYTYPE, class VALUETYPE> class Dictionary
{
public:
    /// default constructor
    Dictionary();
    /// copy constructor
    Dictionary(const Dictionary<KEYTYPE, VALUETYPE>& rhs);
    /// move constructor
    Dictionary(Dictionary<KEYTYPE, VALUETYPE>&& rhs);
    /// assignment operator
    void operator=(const Dictionary<KEYTYPE, VALUETYPE>& rhs);
    /// move operator
    void operator=(Dictionary<KEYTYPE, VALUETYPE>&& rhs);
    /// read/write [] operator
    VALUETYPE& operator[](const KEYTYPE& key);
    /// read-only [] operator
    const VALUETYPE& operator[](const KEYTYPE& key) const;
    /// return number of key/value pairs in the dictionary
    SizeT Size() const;
    /// clear the dictionary
    void Clear();
    /// return true if empty
    bool IsEmpty() const;
    /// reserve space (useful if number of elements is known beforehand)
    void Reserve(SizeT numElements);
    /// begin a bulk insert (array will be sorted at End)
    void BeginBulkAdd();
    /// add a key/value pair
    void Add(const KeyValuePair<KEYTYPE, VALUETYPE>& kvp);
    /// add a key and associated value
    void Add(const KEYTYPE& key, const VALUETYPE& value);
	/// creates a new entry of VALUETYPE if key does not exist, 
	VALUETYPE& AddUnique(const KEYTYPE& key);
    /// end a bulk insert (this will sort the internal array)
    void EndBulkAdd();
	/// merge two dictionaries
	void Merge(const Dictionary<KEYTYPE, VALUETYPE>& rhs);
    /// erase a key and its associated value
    void Erase(const KEYTYPE& key);
    /// erase a key at index
    void EraseAtIndex(IndexT index);
    /// find index of key/value pair (InvalidIndex if doesn't exist)
    IndexT FindIndex(const KEYTYPE& key) const;
    /// return true if key exists in the array
    bool Contains(const KEYTYPE& key) const;
	/// return true if key exists in the array, and saves index
	bool Contains(const KEYTYPE& key, IndexT& index) const;
    /// get a key at given index
    const KEYTYPE& KeyAtIndex(IndexT index) const;
    /// access to value at given index
    VALUETYPE& ValueAtIndex(IndexT index);
    /// get a value at given index
    const VALUETYPE& ValueAtIndex(IndexT index) const;
    /// get key/value pair at index
    KeyValuePair<KEYTYPE, VALUETYPE>& KeyValuePairAtIndex(IndexT index) const;
    /// get all keys as an Util::Array
    Array<KEYTYPE> KeysAsArray() const;
    /// get all keys as an Util::Array
    Array<VALUETYPE> ValuesAsArray() const;
    /// get all keys as (typically) an array
    template<class RETURNTYPE> RETURNTYPE KeysAs() const;
    /// get all keys as (typically) an array
    template<class RETURNTYPE> RETURNTYPE ValuesAs() const;

protected:
    /// make sure the key value pair array is sorted
    void SortIfDirty() const;

    Array<KeyValuePair<KEYTYPE, VALUETYPE> > keyValuePairs;
    bool inBulkInsert;
};

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
Dictionary<KEYTYPE, VALUETYPE>::Dictionary() :
    inBulkInsert(false)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
Dictionary<KEYTYPE, VALUETYPE>::Dictionary(const Dictionary<KEYTYPE, VALUETYPE>& rhs) :
    keyValuePairs(rhs.keyValuePairs),
    inBulkInsert(false)
{
    #if NEBULA_BOUNDSCHECKS
    n_assert(!rhs.inBulkInsert);
    #endif
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
Dictionary<KEYTYPE, VALUETYPE>::Dictionary(Dictionary<KEYTYPE, VALUETYPE>&& rhs) :
    keyValuePairs(std::move(rhs.keyValuePairs)),
    inBulkInsert(false)
{
#if NEBULA_BOUNDSCHECKS
    n_assert(!rhs.inBulkInsert);
#endif
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
inline void
Dictionary<KEYTYPE, VALUETYPE>::operator=(const Dictionary<KEYTYPE, VALUETYPE>& rhs)
{
    #if NEBULA_BOUNDSCHECKS
    n_assert(!this->inBulkInsert);
    n_assert(!rhs.inBulkInsert);
    #endif
    this->keyValuePairs = rhs.keyValuePairs;
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
inline void
Dictionary<KEYTYPE, VALUETYPE>::operator=(Dictionary<KEYTYPE, VALUETYPE>&& rhs)
{
#if NEBULA_BOUNDSCHECKS
    n_assert(!this->inBulkInsert);
    n_assert(!rhs.inBulkInsert);
#endif
    this->keyValuePairs = std::move(rhs.keyValuePairs);   
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
inline void
Dictionary<KEYTYPE, VALUETYPE>::Clear()
{
    #if NEBULA_BOUNDSCHECKS
    n_assert(!this->inBulkInsert);
    #endif
    this->keyValuePairs.Clear();
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
inline SizeT
Dictionary<KEYTYPE, VALUETYPE>::Size() const
{
    return this->keyValuePairs.Size();
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE> 
inline bool
Dictionary<KEYTYPE, VALUETYPE>::IsEmpty() const
{
    return (0 == this->keyValuePairs.Size());
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
inline void
Dictionary<KEYTYPE, VALUETYPE>::Reserve(SizeT numElements)
{
    this->keyValuePairs.Reserve(numElements);
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
inline void
Dictionary<KEYTYPE, VALUETYPE>::BeginBulkAdd()
{
    #if NEBULA_BOUNDSCHECKS
    n_assert(!this->inBulkInsert);
    #endif
    this->inBulkInsert = true;
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
inline void
Dictionary<KEYTYPE, VALUETYPE>::EndBulkAdd()
{
    #if NEBULA_BOUNDSCHECKS
    n_assert(this->inBulkInsert);
    #endif
    this->keyValuePairs.Sort();
    this->inBulkInsert = false;
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
inline void
Dictionary<KEYTYPE, VALUETYPE>::Merge(const Dictionary<KEYTYPE, VALUETYPE>& rhs)
{
	this->BeginBulkAdd();
	IndexT i;
	for (i = 0; i < rhs.keyValuePairs.Size(); i++)
	{
		this->keyValuePairs.Append(rhs.keyValuePairs[i]);
	}
	this->EndBulkAdd();
}


//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
inline void
Dictionary<KEYTYPE, VALUETYPE>::Add(const KeyValuePair<KEYTYPE, VALUETYPE>& kvp)
{
	if (this->inBulkInsert)
	{
		this->keyValuePairs.Append(kvp);
	}
	else
	{
		this->keyValuePairs.InsertSorted(kvp);
	}
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
inline void
Dictionary<KEYTYPE, VALUETYPE>::Add(const KEYTYPE& key, const VALUETYPE& value)
{
    //n_assert(!this->Contains(key));
    KeyValuePair<KEYTYPE, VALUETYPE> kvp(key, value);
    if (this->inBulkInsert)
    {
        this->keyValuePairs.Append(kvp);
    }
    else
    {
        this->keyValuePairs.InsertSorted(kvp);
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
inline VALUETYPE&
Dictionary<KEYTYPE, VALUETYPE>::AddUnique(const KEYTYPE& key)
{
	IndexT i = this->FindIndex(key);
	if (i == InvalidIndex)
	{
		this->Add(key, VALUETYPE());
		return this->ValueAtIndex(this->FindIndex(key));
	}
	else
	{
		return this->ValueAtIndex(i);
	}
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
inline void
Dictionary<KEYTYPE, VALUETYPE>::Erase(const KEYTYPE& key)
{
    #if NEBULA_BOUNDSCHECKS
    n_assert(!this->inBulkInsert);
    #endif
    IndexT eraseIndex = this->keyValuePairs.BinarySearchIndex(key);
    #if NEBULA_BOUNDSCHECKS
    n_assert(InvalidIndex != eraseIndex);
    #endif
    this->keyValuePairs.EraseIndex(eraseIndex);
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
inline void
Dictionary<KEYTYPE, VALUETYPE>::EraseAtIndex(IndexT index)
{
    #if NEBULA_BOUNDSCHECKS
    n_assert(!this->inBulkInsert);
    #endif
    this->keyValuePairs.EraseIndex(index);
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE> 
inline IndexT
Dictionary<KEYTYPE, VALUETYPE>::FindIndex(const KEYTYPE& key) const
{
    #if NEBULA_BOUNDSCHECKS
    n_assert(!this->inBulkInsert);
    #endif
    return this->keyValuePairs.BinarySearchIndex(key);
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
inline bool
Dictionary<KEYTYPE, VALUETYPE>::Contains(const KEYTYPE& key) const
{
	#if NEBULA_BOUNDSCHECKS
	n_assert(!this->inBulkInsert);
	#endif
    return (InvalidIndex != this->keyValuePairs.BinarySearchIndex(key));
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
inline bool
Dictionary<KEYTYPE, VALUETYPE>::Contains(const KEYTYPE& key, IndexT& index) const
{
#if NEBULA_BOUNDSCHECKS
	n_assert(!this->inBulkInsert);
#endif
	index = this->keyValuePairs.BinarySearchIndex(key);
	return (InvalidIndex != index);
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE> 
inline const KEYTYPE&
Dictionary<KEYTYPE, VALUETYPE>::KeyAtIndex(IndexT index) const
{
    #if NEBULA_BOUNDSCHECKS
    n_assert(!this->inBulkInsert);
    #endif
    return this->keyValuePairs[index].Key();
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE> 
inline VALUETYPE&
Dictionary<KEYTYPE, VALUETYPE>::ValueAtIndex(IndexT index)
{
    #if NEBULA_BOUNDSCHECKS
    n_assert(!this->inBulkInsert);
    #endif
    return this->keyValuePairs[index].Value();
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE> 
inline const VALUETYPE&
Dictionary<KEYTYPE, VALUETYPE>::ValueAtIndex(IndexT index) const
{
    #if NEBULA_BOUNDSCHECKS
    n_assert(!this->inBulkInsert);
    #endif
    return this->keyValuePairs[index].Value();
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE> 
inline KeyValuePair<KEYTYPE, VALUETYPE>&
Dictionary<KEYTYPE, VALUETYPE>::KeyValuePairAtIndex(IndexT index) const
{
    #if NEBULA_BOUNDSCHECKS
    n_assert(!this->inBulkInsert);
    #endif
    return this->keyValuePairs[index];
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE> 
inline VALUETYPE&
Dictionary<KEYTYPE, VALUETYPE>::operator[](const KEYTYPE& key)
{
    int keyValuePairIndex = this->FindIndex(key);
    #if NEBULA_BOUNDSCHECKS
	n_assert(InvalidIndex != keyValuePairIndex);
    #endif   
    return this->keyValuePairs[keyValuePairIndex].Value();
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE> 
inline const VALUETYPE&
Dictionary<KEYTYPE, VALUETYPE>::operator[](const KEYTYPE& key) const
{
    int keyValuePairIndex = this->FindIndex(key);
    #if NEBULA_BOUNDSCHECKS
    n_assert(InvalidIndex != keyValuePairIndex);
    #endif
    return this->keyValuePairs[keyValuePairIndex].Value();
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
template<class RETURNTYPE>
RETURNTYPE
Dictionary<KEYTYPE, VALUETYPE>::ValuesAs() const
{
    #if NEBULA_BOUNDSCHECKS
    n_assert(!this->inBulkInsert);
    #endif
    RETURNTYPE result(this->Size(),this->Size());
    IndexT i;
    for (i = 0; i < this->keyValuePairs.Size(); i++)
    {
        result.Append(this->keyValuePairs[i].Value());
    }
    return result;
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
inline Array<VALUETYPE>
Dictionary<KEYTYPE, VALUETYPE>::ValuesAsArray() const
{
    return this->ValuesAs<Array<VALUETYPE> >();
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE> 
template<class RETURNTYPE>
inline RETURNTYPE
Dictionary<KEYTYPE, VALUETYPE>::KeysAs() const
{
    #if NEBULA_BOUNDSCHECKS    
    n_assert(!this->inBulkInsert);
    #endif
    RETURNTYPE result(this->Size(),this->Size());
    IndexT i;
    for (i = 0; i < this->keyValuePairs.Size(); i++)
    {
        result.Append(this->keyValuePairs[i].Key());
    }
    return result;
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
inline Array<KEYTYPE>
Dictionary<KEYTYPE, VALUETYPE>::KeysAsArray() const
{
    return this->KeysAs<Array<KEYTYPE> >();
}

} // namespace Util
//------------------------------------------------------------------------------
