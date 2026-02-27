#pragma once
//------------------------------------------------------------------------------
/**
    @class Util::FixedArray
    
    Implements a fixed size one-dimensional array.
    
    @copyright
    (C) 2006 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "util/array.h"


//------------------------------------------------------------------------------
namespace Util
{
template<class TYPE, bool StackAlloc = false> class FixedArray
{
public:
    /// define element iterator
    typedef TYPE* Iterator;

    /// default constructor
    FixedArray();
    /// constructor with size
    FixedArray(const SizeT s);
    /// constructor with size and initial value
    FixedArray(const SizeT s, const TYPE& initialValue);
    /// copy constructor
    FixedArray(const FixedArray<TYPE, false>& rhs);
    /// copy constructor
    FixedArray(const FixedArray<TYPE, true>& rhs);
    /// construct from array
    FixedArray(const Array<TYPE>& rhs);
    /// move from array
    FixedArray(Array<TYPE>&& rhs);
    /// move constructor
    FixedArray(FixedArray<TYPE, StackAlloc>&& rhs);
    /// constructor from initializer list
    FixedArray(std::initializer_list<TYPE> list);
    /// construct an empty fixed array
    FixedArray(std::nullptr_t);
    /// destructor
    ~FixedArray();
    /// assignment operator
    void operator=(const FixedArray<TYPE, StackAlloc>& rhs);
    /// move assignment operator
    void operator=(FixedArray<TYPE, StackAlloc>&& rhs) noexcept;
    /// write [] operator
    TYPE& operator[](IndexT index) const;
    /// equality operator
    bool operator==(const FixedArray<TYPE, StackAlloc>& rhs) const;
    /// inequality operator
    bool operator!=(const FixedArray<TYPE, StackAlloc>& rhs) const;

    /// set number of elements (clears existing content)
    void SetSize(SizeT s);
    /// get number of elements
    const SizeT Size() const;
    /// get total byte size
    const SizeT ByteSize() const;
    /// resize array without deleting existing content
    void Resize(SizeT newSize);
    /// return true if array if empty (has no elements)
    bool IsEmpty() const;
    /// clear the array, free elements
    void Clear();
    /// Reset the size and destroy all elements
    void Reset();
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

    /// for range-based iteration (C++11)
    Iterator begin() const;
    Iterator end() const;
    size_t size() const;
    void resize(size_t size);
private:

    template<class T, bool S>
    friend class FixedArray;

    /// delete content
    void Delete();
    /// allocate array for given size
    void Alloc(SizeT s);
    /// copy content
    void Copy(const FixedArray<TYPE, StackAlloc>& src);

    SizeT count;
    TYPE* elements;
};


//------------------------------------------------------------------------------
/**
*/
template<class TYPE, bool StackAlloc>
FixedArray<TYPE, StackAlloc>::FixedArray() :
    count(0),
    elements(nullptr)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, bool StackAlloc> void
FixedArray<TYPE, StackAlloc>::Delete()
{
    if (this->elements)
    {
        if constexpr (StackAlloc)
        {
            ArrayFreeStack(this->count, this->elements);
        }
        else
        {
            ArrayFree(this->count, this->elements);
        }

        this->elements = nullptr;
        
    }
    this->count = 0;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, bool StackAlloc> void
FixedArray<TYPE, StackAlloc>::Alloc(SizeT s)
{
    #if NEBULA_BOUNDSCHECKS
    n_assert(0 == this->elements);
    #endif
    if (s > 0)
    {
        if constexpr (StackAlloc)
        {
            this->elements = ArrayAllocStack<TYPE>(s);
        }
        else
        {
            this->elements = ArrayAlloc<TYPE>(s);
        }
    }
    this->count = s;
}

//------------------------------------------------------------------------------
/**
    NOTE: only works on deleted array. This is intended.
*/
template<class TYPE, bool StackAlloc> void
FixedArray<TYPE, StackAlloc>::Copy(const FixedArray<TYPE, StackAlloc>& rhs)
{
    if (this->elements != rhs.elements && rhs.count > 0)
    {
        this->Alloc(rhs.count);
        if constexpr (!std::is_trivially_copyable<TYPE>::value)
        {
            IndexT i;
            for (i = 0; i < this->count; i++)
            {
                this->elements[i] = rhs.elements[i];
            }
        }
        else
            memcpy(this->elements, rhs.elements, this->count * sizeof(TYPE));
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, bool StackAlloc>
FixedArray<TYPE, StackAlloc>::FixedArray(const SizeT s) :
    count(0),
    elements(nullptr)
{
    this->Alloc(s);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, bool StackAlloc>
FixedArray<TYPE, StackAlloc>::FixedArray(const SizeT s, const TYPE& initialValue) :
    count(0),
    elements(nullptr)
{
    this->Alloc(s);
    this->Fill(initialValue);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, bool StackAlloc>
FixedArray<TYPE, StackAlloc>::FixedArray(const FixedArray<TYPE, true>& rhs) :
    count(0),
    elements(nullptr)
{
    if (this->elements != rhs.elements && rhs.count > 0)
    {
        this->Alloc(rhs.count);
        if constexpr (!std::is_trivially_copyable<TYPE>::value)
        {
            IndexT i;
            for (i = 0; i < this->count; i++)
            {
                this->elements[i] = rhs.elements[i];
            }
        }
        else
            memcpy(this->elements, rhs.elements, this->count * sizeof(TYPE));
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, bool StackAlloc>
FixedArray<TYPE, StackAlloc>::FixedArray(const FixedArray<TYPE, false>& rhs) :
    count(0),
    elements(nullptr)
{
    if (this->elements != rhs.elements && rhs.count > 0)
    {
        this->Alloc(rhs.count);
        if constexpr (!std::is_trivially_copyable<TYPE>::value)
        {
            IndexT i;
            for (i = 0; i < this->count; i++)
            {
                this->elements[i] = rhs.elements[i];
            }
        }
        else
            memcpy(this->elements, rhs.elements, this->count * sizeof(TYPE));
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, bool StackAlloc>
FixedArray<TYPE, StackAlloc>::FixedArray(const Array<TYPE>& rhs) :
    count(rhs.Size()),
    elements(nullptr)
{
    if (this->count > 0)
    {
        this->Alloc(this->count);
        if constexpr (!std::is_trivially_copyable<TYPE>::value)
        {
            IndexT i;
            for (i = 0; i < this->count; i++)
            {
                this->elements[i] = rhs.Begin()[i];
            }
        }
        else
            memcpy(this->elements, rhs.Begin(), this->count * sizeof(TYPE));
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, bool StackAlloc>
FixedArray<TYPE, StackAlloc>::FixedArray(Array<TYPE>&& rhs)
    : elements(nullptr)
    , count(0)
{
    // If array lives on the stack, then we need to allocate
    if (rhs.count > 0)
    {
        if (rhs.stackElements.data() == rhs.elements || StackAlloc)
        {
            this->Alloc(rhs.count);
            if constexpr (!std::is_trivially_copyable<TYPE>::value)
            {
                IndexT i;
                for (i = 0; i < this->count; i++)
                {
                    this->elements[i] = rhs.Begin()[i];
                }
            }
            else
                memcpy(this->elements, rhs.Begin(), this->count * sizeof(TYPE));
        }
        else
        {
            this->elements = rhs.elements;
            rhs.elements = nullptr;
            this->count = rhs.count;
            rhs.capacity = 0;
            rhs.count = 0;
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, bool StackAlloc>
FixedArray<TYPE, StackAlloc>::FixedArray(FixedArray<TYPE, StackAlloc>&& rhs) :
    count(rhs.count),
    elements(rhs.elements)
{
    if constexpr (StackAlloc)
    {
        this->Alloc(rhs.count);
        if constexpr (!std::is_trivially_copyable<TYPE>::value)
        {
            IndexT i;
            for (i = 0; i < this->count; i++)
            {
                this->elements[i] = rhs.Begin()[i];
            }
        }
        else
            memcpy(this->elements, rhs.Begin(), this->count * sizeof(TYPE));
    }
    else
    {
        rhs.count = 0;
        rhs.elements = nullptr;
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, bool StackAlloc>
FixedArray<TYPE, StackAlloc>::FixedArray(std::initializer_list<TYPE> list) :
    count(0),
    elements(nullptr)
{
    this->Alloc((SizeT)list.size());
    if constexpr (!std::is_trivially_copyable<TYPE>::value)
    {
        IndexT i;
        for (i = 0; i < this->count; i++)
        {
            this->elements[i] = list.begin()[i];
        }
    }
    else
        memcpy(this->elements, list.begin(), this->count * sizeof(TYPE));
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, bool StackAlloc>
FixedArray<TYPE, StackAlloc>::FixedArray(std::nullptr_t) :
    count(0),
    elements(nullptr)
{
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, bool StackAlloc>
FixedArray<TYPE, StackAlloc>::~FixedArray()
{
    this->Delete();
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, bool StackAlloc> void
FixedArray<TYPE, StackAlloc>::operator=(const FixedArray<TYPE, StackAlloc>& rhs)
{
    if (this != &rhs)
    {
        this->Delete();
        this->Copy(rhs);
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, bool StackAlloc> void
FixedArray<TYPE, StackAlloc>::operator=(FixedArray<TYPE, StackAlloc>&& rhs) noexcept
{
    if (this != &rhs)
    {
        this->Delete();
        this->elements = rhs.elements;
        this->count = rhs.count;
        rhs.elements = nullptr;
        rhs.count = 0;
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, bool StackAlloc> TYPE&
FixedArray<TYPE, StackAlloc>::operator[](IndexT index) const
{
    #if NEBULA_BOUNDSCHECKS
    n_assert(this->elements && (index < this->count));
    #endif
    return this->elements[index];
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, bool StackAlloc> bool
FixedArray<TYPE, StackAlloc>::operator==(const FixedArray<TYPE, StackAlloc>& rhs) const
{
    if (this->count != rhs.count)
    {
        return false;
    }
    else
    {
        #if NEBULA_BOUNDSCHECKS
        n_assert(this->elements && rhs.elements);
        #endif
        IndexT i;
        SizeT num = this->count;
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
template<class TYPE, bool StackAlloc> bool
FixedArray<TYPE, StackAlloc>::operator!=(const FixedArray<TYPE, StackAlloc>& rhs) const
{
    return !(*this == rhs);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, bool StackAlloc> void
FixedArray<TYPE, StackAlloc>::SetSize(SizeT s)
{
    this->Delete();
    this->Alloc(s);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, bool StackAlloc> void
FixedArray<TYPE, StackAlloc>::Resize(SizeT newSize)
{
    // allocate new array and copy over old elements
    if (newSize == this->count)
        return;

    TYPE* newElements = 0;
    if (newSize > 0)
    {
        if constexpr (StackAlloc)
        {
            newElements = ArrayAllocStack<TYPE>(newSize);
        }
        else
        {
            newElements = ArrayAlloc<TYPE>(newSize);
        }
        SizeT numCopy = this->count;
        if (numCopy > 0)
        {
            if (numCopy > newSize)
                numCopy = newSize;
            if constexpr (!std::is_trivially_move_assignable<TYPE>::value && std::is_move_assignable<TYPE>::value)
            {
                IndexT i;
                for (i = 0; i < numCopy; i++)
                {
                    newElements[i] = std::move(this->elements[i]);
                }
            }
            else
            {
                Memory::MoveElements(this->elements, newElements, numCopy);
            }
        }
    }

    // delete old elements
    this->Delete();

    // set content to new elements
    this->elements = newElements;
    this->count = newSize;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, bool StackAlloc> const SizeT
FixedArray<TYPE, StackAlloc>::Size() const
{
    return this->count;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, bool StackAlloc> const SizeT
FixedArray<TYPE, StackAlloc>::ByteSize() const
{
    return this->count * sizeof(TYPE);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, bool StackAlloc> bool
FixedArray<TYPE, StackAlloc>::IsEmpty() const
{
    return 0 == this->count;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, bool StackAlloc> void
FixedArray<TYPE, StackAlloc>::Clear()
{
    this->Delete();
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, bool StackAlloc>
inline void FixedArray<TYPE, StackAlloc>::Reset()
{
    if (this->count > 0)
    {
        if (std::is_trivially_destructible<TYPE>::value)
        {
            memset(this->elements, 0, this->count * sizeof(TYPE));
        }
        else
        {
            IndexT i;
            for (i = 0; i < this->count; i++)
            {
                this->elements[i].~TYPE();
            }
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, bool StackAlloc> void
FixedArray<TYPE, StackAlloc>::Fill(const TYPE& val)
{
    IndexT i;
    for (i = 0; i < this->count; i++)
    {
        this->elements[i] = val;
    }
}       

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, bool StackAlloc> void
FixedArray<TYPE, StackAlloc>::Fill(IndexT first, SizeT num, const TYPE& val)
{
    #if NEBULA_BOUNDSCHECKS
    n_assert((first + num) <= this->count);
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
template<class TYPE, bool StackAlloc> typename FixedArray<TYPE, StackAlloc>::Iterator
FixedArray<TYPE, StackAlloc>::Begin() const
{
    return this->elements;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, bool StackAlloc> typename FixedArray<TYPE, StackAlloc>::Iterator
FixedArray<TYPE, StackAlloc>::End() const
{
    return this->elements + this->count;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, bool StackAlloc> typename FixedArray<TYPE, StackAlloc>::Iterator
FixedArray<TYPE, StackAlloc>::Find(const TYPE& elm) const
{
    IndexT i;
    for (i = 0; i < this->count; i++)
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
template<class TYPE, bool StackAlloc> IndexT
FixedArray<TYPE, StackAlloc>::FindIndex(const TYPE& elm) const
{
    IndexT i;
    for (i = 0; i < this->count; i++)
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
template<class TYPE, bool StackAlloc> void
FixedArray<TYPE, StackAlloc>::Sort()
{
    std::sort(this->Begin(), this->End());
}

//------------------------------------------------------------------------------
/**
    @todo hmm, this is copy-pasted from Array...
*/
template<class TYPE, bool StackAlloc> IndexT
FixedArray<TYPE, StackAlloc>::BinarySearchIndex(const TYPE& elm) const
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
template<class TYPE, bool StackAlloc> Array<TYPE>
FixedArray<TYPE, StackAlloc>::AsArray() const
{
    Array<TYPE> result;
    result.Reserve(this->count);
    IndexT i;
    for (i = 0; i < this->count; i++)
    {
        result.Append(this->elements[i]);
    }
    return result;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, bool StackAlloc> typename FixedArray<TYPE, StackAlloc>::Iterator
FixedArray<TYPE, StackAlloc>::begin() const
{
    return this->elements;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, bool StackAlloc> typename FixedArray<TYPE, StackAlloc>::Iterator
FixedArray<TYPE, StackAlloc>::end() const
{
    return this->elements + this->count;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, bool StackAlloc> void
FixedArray<TYPE, StackAlloc>::resize(size_t s)
{
    n_error("Trying to resize a fixed array");
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, bool StackAlloc> size_t
FixedArray<TYPE, StackAlloc>::size() const
{
    return this->count;
}

} // namespace Util
//------------------------------------------------------------------------------
