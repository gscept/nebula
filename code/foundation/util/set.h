#pragma once
//------------------------------------------------------------------------------
/**
    @class Util::Set

	A collection of unique values with quick lookup.
	Internally implemented as a sorted array.

	Adding the same value more than once has no obvious effect.
  
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
template<class KEYTYPE> class Set
{
public:
    /// default constructor
	Set();
    /// copy constructor
	Set(const Set<KEYTYPE>& rhs);
    /// assignment operator
	void operator=(const Set<KEYTYPE>& rhs);

    /// return number of unique keys
    SizeT Size() const;
    /// clear the set
    void Clear();
    /// return true if empty
    bool IsEmpty() const;
    /// reserve space (useful if number of elements is known beforehand)
    void Reserve(SizeT numElements);
    /// begin a bulk insert (array will be sorted at End)
    void BeginBulkAdd();
    /// add a value while in bulk, wont check for duplicates
    void BulkAdd(const KEYTYPE& value);
    /// add a unique value to set, won't get added twice
    void Add(const KEYTYPE& value);
    /// end a bulk insert (this will sort the internal array)
    void EndBulkAdd();
    /// erase a key and its associated value
    void Erase(const KEYTYPE& key);
    /// erase a key at index
    void EraseAtIndex(IndexT index);
    /// find index of key pair (InvalidIndex if doesn't exist)
    IndexT FindIndex(const KEYTYPE& key) const;
    /// return true if key exists in the array
    bool Contains(const KEYTYPE& key) const;
    /// get a key at given index
    const KEYTYPE& KeyAtIndex(IndexT index) const;
    /// get all keys as an Util::Array
    Array<KEYTYPE> KeysAsArray() const;
    /// get all keys as (typically) an array
    template<class RETURNTYPE> RETURNTYPE KeysAs() const;

protected:
    /// make sure the key value pair array is sorted
    void SortIfDirty() const;

    Array<KEYTYPE> values;
    bool inBulkInsert;
};

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE>
Set<KEYTYPE>::Set() :
    inBulkInsert(false)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE>
Set<KEYTYPE>::Set(const Set<KEYTYPE>& rhs) :
    values(rhs.keyValuePairs),
    inBulkInsert(false)
{
    #if NEBULA_BOUNDSCHECKS
    n_assert(!rhs.inBulkInsert);
    #endif
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE> void
Set<KEYTYPE>::operator=(const Set<KEYTYPE>& rhs)
{
    #if NEBULA_BOUNDSCHECKS
    n_assert(!this->inBulkInsert);
    n_assert(!rhs.inBulkInsert);
    #endif
	this->values = rhs.values;
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE> void
Set<KEYTYPE>::Clear()
{
    #if NEBULA_BOUNDSCHECKS
    n_assert(!this->inBulkInsert);
    #endif
	this->values.Clear();
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE> SizeT
Set<KEYTYPE>::Size() const
{
	return this->values.Size();
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE> bool
Set<KEYTYPE>::IsEmpty() const
{
	return (0 == this->values.Size());
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE> void
Set<KEYTYPE>::Reserve(SizeT numElements)
{
	this->values.Reserve(numElements);
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE> void
Set<KEYTYPE>::BeginBulkAdd()
{
#if NEBULA_BOUNDSCHECKS
    n_assert(!this->inBulkInsert);
#endif
    this->inBulkInsert = true;
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE> void
Set<KEYTYPE>::EndBulkAdd()
{
#if NEBULA_BOUNDSCHECKS
    n_assert(this->inBulkInsert);
#endif
	this->values.Sort();    
    this->inBulkInsert = false;
#if NEBULA_BOUNDSCHECKS
    n_assert(this->values.IsSorted());
#endif
}


//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE> void
Set<KEYTYPE>::BulkAdd(const KEYTYPE& key)
{
#if NEBULA_BOUNDSCHECKS
    n_assert(this->inBulkInsert);
#endif
    this->values.Append(key);    
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE> void
Set<KEYTYPE>::Add(const KEYTYPE& key)
{   
#if NEBULA_BOUNDSCHECKS
    n_assert(!this->inBulkInsert);
#endif
	if (!this->Contains(key))
	{
	    this->values.InsertSorted(key);
	}    
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE> void
Set<KEYTYPE>::Erase(const KEYTYPE& key)
{
    #if NEBULA_BOUNDSCHECKS
    n_assert(!this->inBulkInsert);
    #endif
	IndexT eraseIndex = this->values.BinarySearchIndex(key);
    #if NEBULA_BOUNDSCHECKS
    n_assert(InvalidIndex != eraseIndex);
    #endif
	this->values.EraseIndex(eraseIndex);
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE> void
Set<KEYTYPE>::EraseAtIndex(IndexT index)
{
    #if NEBULA_BOUNDSCHECKS
    n_assert(!this->inBulkInsert);
    #endif
	this->values.EraseIndex(index);
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE> IndexT
Set<KEYTYPE>::FindIndex(const KEYTYPE& key) const
{
    #if NEBULA_BOUNDSCHECKS
    n_assert(!this->inBulkInsert);
    #endif
    return this->values.BinarySearchIndex(key);
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE> bool
Set<KEYTYPE>::Contains(const KEYTYPE& key) const
{
    #if NEBULA_BOUNDSCHECKS
    n_assert(!this->inBulkInsert);
    #endif
	return (InvalidIndex != this->values.BinarySearchIndex(key));
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE> const KEYTYPE&
Set<KEYTYPE>::KeyAtIndex(IndexT index) const
{
    #if NEBULA_BOUNDSCHECKS
    n_assert(!this->inBulkInsert);
    #endif
	return this->values[index];
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE> 
template<class RETURNTYPE>
RETURNTYPE
Set<KEYTYPE>::KeysAs() const
{
    #if NEBULA_BOUNDSCHECKS    
    n_assert(!this->inBulkInsert);
    #endif
    RETURNTYPE result(this->Size(), this->Size());
    IndexT i;
	for (i = 0; i < this->values.Size(); i++)
    {
        result.Append(this->values[i]);
    }
    return result;
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE>
Array<KEYTYPE>
Set<KEYTYPE>::KeysAsArray() const
{
    return this->KeysAs<Array<KEYTYPE>>();
}

} // namespace Util
//------------------------------------------------------------------------------
