#pragma once
//------------------------------------------------------------------------------
/**
    @class Util::TrivialArray

    Array class based on Util::Array for trivial and POD types that avoids
    any per element copying and constructor/destructor calls

    @copyright
    (C) 2018-2020 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "util/array.h"

//------------------------------------------------------------------------------
namespace Util
{
template<class TYPE> class TrivialArray : public Array<TYPE>
{
public:
    /// define iterator
    typedef TYPE* Iterator;

    /// constructor with default parameters
    TrivialArray();
    /// constuctor with initial size and grow size
    TrivialArray(SizeT initialCapacity, SizeT initialGrow);
    /// constructor with initial size, grow size and initial values
    TrivialArray(SizeT initialSize, SizeT initialGrow, const TYPE& initialValue);
    /// copy constructor
    TrivialArray(const TrivialArray<TYPE>& rhs);
    /// copy constructor from Array
    TrivialArray(const Array<TYPE>& rhs);
    /// constructor from initializer list
    TrivialArray(std::initializer_list<TYPE> list);
    /// destructor
    ~TrivialArray();

    /// assignment operator
    void operator=(const TrivialArray<TYPE>& rhs);
    /// assignment operator from array
    void operator=(const Array<TYPE>& rhs);
    
    /// erase element at index, keep sorting intact
    void EraseIndex(IndexT index);    
    /// erase element at index, fill gap by swapping in last element, destroys sorting!
    void EraseIndexSwap(IndexT index);
    
    /// clear array (calls destructors)
    void Clear();
    
private:
    /// does nothing
    void Destroy(TYPE* elm);
    /// copy content
    void Copy(const TrivialArray<TYPE>& src);
    /// copy content
    void Copy(const Array<TYPE>& src);
    /// grow array to target size
    void GrowTo(SizeT newCapacity);
    /// move elements, grows array if needed
    void Move(IndexT fromIndex, IndexT toIndex);    
};

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
TrivialArray<TYPE>::TrivialArray()
{
    static_assert(std::is_trivial<TYPE>::value, "Non trivial type used, try Util::Array instead");
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
TrivialArray<TYPE>::TrivialArray(SizeT _capacity, SizeT _grow) : 
    Array<TYPE>(_capacity, _grow)
    
{
    static_assert(std::is_trivial<TYPE>::value, "Non trivial type used, try Util::Array instead");
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
TrivialArray<TYPE>::TrivialArray(SizeT initialSize, SizeT _grow, const TYPE& initialValue) :
    Array<TYPE>(initialSize, _grow, initialValue)    
{
    static_assert(std::is_trivial<TYPE>::value, "Non trivial type used, try Util::Array instead");
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
TrivialArray<TYPE>::TrivialArray(std::initializer_list<TYPE> list) :
    Array<TYPE>(list)   
{
    static_assert(std::is_trivial<TYPE>::value, "Non trivial type used, try Util::Array instead");

}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
TrivialArray<TYPE>::TrivialArray(const TrivialArray<TYPE>& rhs) 
{
    this->elements = 0;
    this->Copy(rhs);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
TrivialArray<TYPE>::TrivialArray(const Array<TYPE>& rhs)
{
    this->elements = 0;
    this->Copy(rhs);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> void
TrivialArray<TYPE>::Copy(const TrivialArray<TYPE>& src)
{
    #if NEBULA_BOUNDSCHECKS
    n_assert(0 == this->elements);
    #endif

    this->grow = src.grow;
    this->capacity = src.capacity;
    this->size = src.size;
    if (this->capacity > 0)
    {
        this->elements = n_new_array(TYPE, this->capacity);
        Memory::Copy(src.elements, this->elements, this->size * sizeof(TYPE));        
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> void
TrivialArray<TYPE>::Copy(const Array<TYPE>& src)
{
#if NEBULA_BOUNDSCHECKS
    n_assert(0 == this->elements);
#endif    
    this->grow = src.grow;
    this->capacity = src.capacity;
    this->size = src.size;
    if (this->capacity > 0)
    {
        this->elements = n_new_array(TYPE, this->capacity);
        Memory::Copy(src.elements, this->elements, this->size * sizeof(TYPE));        
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> void
TrivialArray<TYPE>::Destroy(TYPE* elm)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
TrivialArray<TYPE>::~TrivialArray()
{
    this->Delete();
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> void 
TrivialArray<TYPE>::operator=(const TrivialArray<TYPE>& rhs)
{
    if (this != &rhs)
    {
        if ((this->capacity > 0) && (rhs.size <= this->capacity))
        {
            // source array fits into our capacity, copy in place
            n_assert(0 != this->elements);
            Memory::Copy(rhs.elements, this->elements, rhs.size * sizeof(TYPE));            
            
            this->grow = rhs.grow;
            this->size = rhs.size;
        }
        else
        {
            // source array doesn't fit into our capacity, need to reallocate
            this->Delete();
            this->Copy(rhs);
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> void
TrivialArray<TYPE>::operator=(const Array<TYPE>& rhs)
{
    if (this != &rhs)
    {
        if ((this->capacity > 0) && (rhs.size <= this->capacity))
        {
            // source array fits into our capacity, copy in place
            n_assert(0 != this->elements);
            Memory::Copy(rhs.elements, this->elements, rhs.size * sizeof(TYPE));
            
            this->grow = rhs.grow;
            this->size = rhs.size;
        }
        else
        {
            // source array doesn't fit into our capacity, need to reallocate
            this->Delete();
            this->Copy(rhs);
        }
    }
}
//------------------------------------------------------------------------------
/**
*/
template<class TYPE> void
TrivialArray<TYPE>::GrowTo(SizeT newCapacity)
{
    TYPE* newArray = n_new_array(TYPE, newCapacity);
    if (this->elements)
    {
        Memory::Copy(this->elements, newArray, this->size * sizeof(TYPE));        
        // discard old array
        n_delete_array(this->elements);
    }
    this->elements  = newArray;
    this->capacity = newCapacity;
}

//------------------------------------------------------------------------------
/**
    30-Jan-03   floh    serious bugfixes!
    07-Dec-04   jo      bugfix: neededSize >= this->capacity => neededSize > capacity   
*/
template<class TYPE> void
TrivialArray<TYPE>::Move(IndexT fromIndex, IndexT toIndex)
{
    #if NEBULA_BOUNDSCHECKS
    n_assert(this->elements);
    n_assert(fromIndex < this->size);
    #endif

    // nothing to move?
    if (fromIndex == toIndex)
    {
        return;
    }

    // compute number of elements to move
    SizeT num = this->size - fromIndex;

    // check if array needs to grow
    SizeT neededSize = toIndex + num;
    while (neededSize > this->capacity)
    {
        this->Grow();
    }

    //TODO: check if memory move is faster than this
    if (fromIndex > toIndex)
    {
        // this is a backward move
        IndexT i;
        for (i = 0; i < num; i++)
        {
            this->elements[toIndex + i] = this->elements[fromIndex + i];
        }       
    }
    else
    {
        // this is a forward move
        int i;  // NOTE: this must remain signed for the following loop to work!!!
        for (i = num - 1; i >= 0; --i)
        {
            this->elements[toIndex + i] = this->elements[fromIndex + i];
        }       
    }

    // adjust array size
    this->size = toIndex + num;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> void
TrivialArray<TYPE>::EraseIndex(IndexT index)
{
    #if NEBULA_BOUNDSCHECKS
    n_assert(this->elements && (index < this->size));
    #endif
    if (index == (this->size - 1))
    {
        // special case: last element        
        this->size--;
    }
    else
    {
        this->Move(index + 1, index);
    }
}

//------------------------------------------------------------------------------
/**    
    NOTE: this method is fast but destroys the sorting order!
*/
template<class TYPE> void
TrivialArray<TYPE>::EraseIndexSwap(IndexT index)
{
    #if NEBULA_BOUNDSCHECKS
    n_assert(this->elements && (index < this->size));
    #endif

    // swap with last element, and destroy last element
    IndexT lastElementIndex = this->size - 1;
    if (index < lastElementIndex)
    {
        this->elements[index] = this->elements[lastElementIndex];
    }    
    this->size--;
}

//------------------------------------------------------------------------------
/**
    The current implementation of this method does not shrink the 
    preallocated space. It simply sets the array size to 0.
*/
template<class TYPE> void
TrivialArray<TYPE>::Clear()
{    
    this->size = 0;
}

} // namespace Util
//------------------------------------------------------------------------------
