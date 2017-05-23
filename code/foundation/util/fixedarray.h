#pragma once
//------------------------------------------------------------------------------
/**
    @class Util::FixedArray
    
    Implements a fixed size one-dimensional array.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "util/array.h"


//------------------------------------------------------------------------------
namespace Util
{
template<class TYPE> class FixedArray
{
public:
    /// define element iterator
    typedef TYPE* Iterator;

    /// default constructor
    FixedArray();
    /// constructor with size
    FixedArray(SizeT s);
    /// constructor with size and initial value
    FixedArray(SizeT s, const TYPE& initialValue);
    /// copy constructor
    FixedArray(const FixedArray<TYPE>& rhs);
    /// destructor
    ~FixedArray();
    /// assignment operator
    void operator=(const FixedArray<TYPE>& rhs);
    /// write [] operator
    TYPE& operator[](IndexT index) const;
    /// equality operator
    bool operator==(const FixedArray<TYPE>& rhs) const;
    /// inequality operator
    bool operator!=(const FixedArray<TYPE>& rhs) const;

    /// set number of elements (clears existing content)
    void SetSize(SizeT s);
    /// get number of elements
    SizeT Size() const;
    /// resize array without deleting existing content
    void Resize(SizeT newSize);
    /// return true if array if empty (has no elements)
    bool IsEmpty() const;
    /// clear the array, free elements
    void Clear();
    /// fill the entire array with a value
    void Fill(const TYPE& val);
    /// fill array range with element
    void Fill(IndexT first, SizeT num, const TYPE& val);
    /// get iterator to first element
    Iterator Begin() const;
    /// get iterator past last element
    Iterator End() const;
    /// find identical element in unsorted array (slow)
    Iterator Find(const TYPE& val) const;
    /// find index of identical element in unsorted array (slow)
    IndexT FindIndex(const TYPE& val) const;
    /// sort the array
    void Sort();
    /// do a binary search, requires a sorted array
    IndexT BinarySearchIndex(const TYPE& val) const;
    /// return content as Array (slow!)
    Array<TYPE> AsArray() const;

private:
    /// delete content
    void Delete();
    /// allocate array for given size
    void Alloc(SizeT s);
    /// copy content
    void Copy(const FixedArray<TYPE>& src);

    SizeT size;
    TYPE* elements;
};

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
FixedArray<TYPE>::FixedArray() :
    size(0),
    elements(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> void
FixedArray<TYPE>::Delete()
{
    if (this->elements)
    {
        n_delete_array(this->elements);
        this->elements = 0;
    }
    this->size = 0;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> void
FixedArray<TYPE>::Alloc(SizeT s)
{
    #if NEBULA3_BOUNDSCHECKS
    n_assert(0 == this->elements) 
    #endif
    if (s > 0)
    {
        this->elements = n_new_array(TYPE, s);
    }
    this->size = s;
}

//------------------------------------------------------------------------------
/**
    NOTE: only works on deleted array. This is intended.
*/
template<class TYPE> void
FixedArray<TYPE>::Copy(const FixedArray<TYPE>& rhs)
{
    if (this != &rhs)
    {
        this->Alloc(rhs.size);
        IndexT i;
        for (i = 0; i < this->size; i++)
        {
            this->elements[i] = rhs.elements[i];
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
FixedArray<TYPE>::FixedArray(SizeT s) :
    size(0),
    elements(0)
{
    this->Alloc(s);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
FixedArray<TYPE>::FixedArray(SizeT s, const TYPE& initialValue) :
    size(0),
    elements(0)
{
    this->Alloc(s);
    this->Fill(initialValue);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
FixedArray<TYPE>::FixedArray(const FixedArray<TYPE>& rhs) :
    size(0),
    elements(0)
{
    this->Copy(rhs);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
FixedArray<TYPE>::~FixedArray()
{
    this->Delete();
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> void
FixedArray<TYPE>::operator=(const FixedArray<TYPE>& rhs)
{
    this->Delete();
    this->Copy(rhs);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> TYPE&
FixedArray<TYPE>::operator[](IndexT index) const
{
    #if NEBULA3_BOUNDSCHECKS
    n_assert(this->elements && (index < this->size));
    #endif
    return this->elements[index];
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> bool
FixedArray<TYPE>::operator==(const FixedArray<TYPE>& rhs) const
{
    if (this->size != rhs.size)
    {
        return false;
    }
    else
    {
        #if NEBULA3_BOUNDSCHECKS
        n_assert(this->elements && rhs.elements);
        #endif
        IndexT i;
        SizeT num = this->size;
        for (i = 0; i < num; i++)
        {
            if (this->elements[i] != rhs.elements[i])
            {
                return false;
            }
        }
        return true;
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> bool
FixedArray<TYPE>::operator!=(const FixedArray<TYPE>& rhs) const
{
    return !(*this == rhs);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> void
FixedArray<TYPE>::SetSize(SizeT s)
{
    this->Delete();
    this->Alloc(s);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> void
FixedArray<TYPE>::Resize(SizeT newSize)
{
    // allocate new array and copy over old elements
    TYPE* newElements = 0;
    if (newSize > 0)
    {
        newElements = n_new_array(TYPE, newSize);
        SizeT numCopy = this->size;
        if (numCopy > newSize) numCopy = newSize;
        IndexT i;
        for (i = 0; i < numCopy; i++)
        {
            newElements[i] = this->elements[i];
        }
    }

    // delete old elements
    this->Delete();

    // set content to new elements
    this->elements = newElements;
    this->size = newSize;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> SizeT
FixedArray<TYPE>::Size() const
{
    return this->size;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> bool
FixedArray<TYPE>::IsEmpty() const
{
    return 0 == this->size;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> void
FixedArray<TYPE>::Clear()
{
    this->Delete();
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> void
FixedArray<TYPE>::Fill(const TYPE& val)
{
    IndexT i;
    for (i = 0; i < this->size; i++)
    {
        this->elements[i] = val;
    }
}       

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> void
FixedArray<TYPE>::Fill(IndexT first, SizeT num, const TYPE& val)
{
    #if NEBULA3_BOUNDSCHECKS
    n_assert((first + num) < this->size);
    n_assert(0 != this->elements);
    #endif
    IndexT i;
    for (i = first; i < (first + num); i++)
    {
        this->elements[i] = val;
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> typename FixedArray<TYPE>::Iterator
FixedArray<TYPE>::Begin() const
{
    return this->elements;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> typename FixedArray<TYPE>::Iterator
FixedArray<TYPE>::End() const
{
    return this->elements + this->size;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> typename FixedArray<TYPE>::Iterator
FixedArray<TYPE>::Find(const TYPE& elm) const
{
    IndexT i;
    for (i = 0; i < this->size; i++)
    {
        if (elm == this->elements[i])
        {
            return &(this->elements[i]);
        }
    }
    return 0;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> IndexT
FixedArray<TYPE>::FindIndex(const TYPE& elm) const
{
    IndexT i;
    for (i = 0; i < this->size; i++)
    {
        if (elm == this->elements[i])
        {
            return i;
        }
    }
    return InvalidIndex;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> void
FixedArray<TYPE>::Sort()
{
    std::sort(this->Begin(), this->End());
}

//------------------------------------------------------------------------------
/**
    @todo hmm, this is copy-pasted from Array...
*/
template<class TYPE> IndexT
FixedArray<TYPE>::BinarySearchIndex(const TYPE& elm) const
{
    SizeT num = this->Size();
    if (num > 0)
    {
        IndexT half;
        IndexT lo = 0;
	    IndexT hi = num - 1;
	    IndexT mid;
        while (lo <= hi) 
        {
            if (0 != (half = num/2)) 
            {
                mid = lo + ((num & 1) ? half : (half - 1));
                if (elm < this->elements[mid])
                {
                    hi = mid - 1;
                    num = num & 1 ? half : half - 1;
                } 
                else if (elm > this->elements[mid]) 
                {
                    lo = mid + 1;
                    num = half;
                } 
                else
                {
                    return mid;
                }
            } 
            else if (num) 
            {
                if (elm != this->elements[lo])
                {
                    return InvalidIndex;
                }
                else      
                {
                    return lo;
                }
            } 
            else 
            {
                break;
            }
        }
    }
    return InvalidIndex;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> Array<TYPE>
FixedArray<TYPE>::AsArray() const
{
    Array<TYPE> result;
    result.Reserve(this->size);
    IndexT i;
    for (i = 0; i < this->size; i++)
    {
        result.Append(this->elements[i]);
    }
    return result;
}

} // namespace Util
//------------------------------------------------------------------------------
