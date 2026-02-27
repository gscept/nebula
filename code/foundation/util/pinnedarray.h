#pragma once
//------------------------------------------------------------------------------
/**
    A pinned array is an array which manages its own virtual memory

    @copyright
    (C) 2022 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "array.h"

namespace Util
{

template<int MAX_ALLOCS, class TYPE>
class PinnedArray : public Array<TYPE>
{

public:

    /// Default constructor
    PinnedArray();
    /// Construct from other pinned array
    PinnedArray(const PinnedArray<MAX_ALLOCS, TYPE>& rhs);
    /// Construct from capacity and grow
    PinnedArray(SizeT capacity, SizeT grow);
    /// Construct from initial commit size, grow and value
    PinnedArray(SizeT initialSize, SizeT grow, const TYPE& initialValue);
    /// Construct from pointer and size
    PinnedArray(const TYPE* const buf, SizeT num);
    /// Construct from initializer list
    PinnedArray(std::initializer_list<TYPE> list);
    /// Destructor
    ~PinnedArray();

    /// assignment operator
    void operator=(const PinnedArray<MAX_ALLOCS, TYPE>& rhs);
    /// move operator
    void operator=(PinnedArray<MAX_ALLOCS, TYPE>&& rhs) noexcept;

    /// Append multiple elements to the end of the array
    template <typename ...ELEM_TYPE>
    void Append(const TYPE& first, const ELEM_TYPE&... elements);
    /// Append single element
    void Append(const TYPE& elm);
    /// Append single element as rhs
    void Append(const TYPE&& elm);
    /// Emplace element and return reference to it
    TYPE& Emplace();
    /// insert element before element at index
    void Insert(IndexT index, const TYPE& elm);
    /// insert element into sorted array, return index where element was included
    IndexT InsertSorted(const TYPE& elm);
    /// insert element at the first non-identical position, return index of inclusion position
    IndexT InsertAtEndOfIdenticalRange(IndexT startIndex, const TYPE& elm);

    /// Append contents of another array
    void AppendArray(const PinnedArray<MAX_ALLOCS, TYPE>& src);
    /// Append contents of C array
    void AppendArray(const TYPE* arr, const SizeT count);
    /// Emplace an array of elements and return pointer
    TYPE* EmplaceArray(const SizeT count);

    /// Fill array with element
    void Fill(IndexT first, SizeT num, const TYPE& elm);

    /// Reserve an amount of memory (commits to memory)
    void Reserve(const SizeT count);
    /// Reallocate and clear
    void Realloc(SizeT capacity, SizeT grow);
    /// Resize to fit, destroys elements outside of new size
    void Resize(SizeT num);
    /// Resize to fit the provided value, but don't shrink if the new size is smaller
    void Extend(SizeT num);

    /// Free memory
    void Free();

    /// Fit array to capacity
    void Fit();
private:
    /// Grow array using growth parameter
    void Grow();
    /// Grow array by size
    void GrowTo(SizeT newCapacity);
    /// Delete array
    void Delete();
    /// Copy from array
    void Copy(const PinnedArray<MAX_ALLOCS, TYPE>& src);
    /// move elements, grows array if needed
    void Move(IndexT fromIndex, IndexT toIndex);
};

//------------------------------------------------------------------------------
/**
*/
template<int MAX_ALLOCS, class TYPE>
inline 
PinnedArray<MAX_ALLOCS, TYPE>::PinnedArray()
{
    this->grow = 16;
    this->capacity = 0;
    this->count = 0;
    this->elements = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
template<int MAX_ALLOCS, class TYPE>
inline 
PinnedArray<MAX_ALLOCS, TYPE>::PinnedArray(const PinnedArray<MAX_ALLOCS, TYPE>& rhs)
{
    this->Copy(rhs);
}

//------------------------------------------------------------------------------
/**
*/
template<int MAX_ALLOCS, class TYPE>
inline
PinnedArray<MAX_ALLOCS, TYPE>::PinnedArray(SizeT capacity, SizeT grow)
{
    this->grow = grow;
    this->capacity = 0;
    this->count = 0;
    this->elements = nullptr;
    if (0 == this->grow)
    {
        this->grow = 16;
    }
    this->GrowTo(capacity);
}

//------------------------------------------------------------------------------
/**
*/
template<int MAX_ALLOCS, class TYPE>
inline 
PinnedArray<MAX_ALLOCS, TYPE>::PinnedArray(SizeT initialSize, SizeT grow, const TYPE& initialValue)
{
    this->grow = grow;
    this->capacity = 0;
    this->count = 0;
    this->elements = nullptr;
    if (0 == this->grow)
    {
        this->grow = 16;
    }

    this->GrowTo(initialSize);
    this->count = initialSize;
    IndexT i;
    for (i = 0; i < initialSize; i++)
    {
        this->elements[i] = initialValue;
    }
}

//------------------------------------------------------------------------------
/**
*/
template<int MAX_ALLOCS, class TYPE>
inline 
PinnedArray<MAX_ALLOCS, TYPE>::PinnedArray(const TYPE* const buf, SizeT num)
{
    this->grow = 16;
    this->capacity = 0;
    this->count = 0;
    this->elements = nullptr;

    static_assert(std::is_trivially_copyable<TYPE>::value, "TYPE is not trivially copyable; Util::Array cannot be constructed from pointer of TYPE.");
    this->GrowTo(num);
    this->count = num;
    const SizeT bytes = num * sizeof(TYPE);
    Memory::Copy(buf, this->elements, bytes);
}

//------------------------------------------------------------------------------
/**
*/
template<int MAX_ALLOCS, class TYPE>
inline 
PinnedArray<MAX_ALLOCS, TYPE>::PinnedArray(std::initializer_list<TYPE> list)
{
    this->grow = 16;
    this->capacity = 0;
    this->count = 0;
    this->elements = nullptr;

    this->GrowTo((SizeT)list.size());
    this->count = (SizeT)list.size();
    IndexT i;
    for (i = 0; i < this->count; i++)
    {
        this->elements[i] = list.begin()[i];
    }
}

//------------------------------------------------------------------------------
/**
*/
template<int MAX_ALLOCS, class TYPE>
inline 
PinnedArray<MAX_ALLOCS, TYPE>::~PinnedArray()
{
    this->Delete();
}

//------------------------------------------------------------------------------
/**
*/
template<int MAX_ALLOCS, class TYPE>
inline void 
PinnedArray<MAX_ALLOCS, TYPE>::operator=(const PinnedArray<MAX_ALLOCS, TYPE>& rhs)
{
    if (this != &rhs)
    {
        if ((this->capacity > 0) && (rhs.count <= this->capacity))
        {
            // source array fits into our capacity, copy in place
            n_assert(0 != this->elements);

            this->CopyRange(this->elements, rhs.elements, rhs.count);
            if (rhs.count < this->count)
            {
                this->DestroyRange(rhs.count, this->count);
            }
            this->grow = rhs.grow;
            this->count = rhs.count;
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
template<int MAX_ALLOCS, class TYPE>
inline void 
PinnedArray<MAX_ALLOCS, TYPE>::operator=(PinnedArray<MAX_ALLOCS, TYPE>&& rhs) noexcept
{
    if (this != &rhs)
    {
        this->Delete();

        // If rhs is not using stack, simply reassign pointers
        if (rhs.elements != rhs.stackElements.data())
        {
            this->elements = rhs.elements;
            rhs.elements = nullptr;
        }
        else
        {
            // Otherwise, move every element over to the stack of this array
            this->MoveRange(this->elements, rhs.elements, rhs.count);
        }

        this->grow = rhs.grow;
        this->count = rhs.count;
        this->capacity = rhs.capacity;
        rhs.count = 0;
        rhs.capacity = 0;
    }
}


//------------------------------------------------------------------------------
/**
*/
template<int MAX_ALLOCS, class TYPE>
template<typename ...ELEM_TYPE>
inline void
PinnedArray<MAX_ALLOCS, TYPE>::Append(const TYPE& first, const ELEM_TYPE & ...elements)
{
    // The plus one is for the first element
    const int size = sizeof...(elements) + 1;
    this->Reserve(size);
    TYPE res[size] = { first, elements... };
    for (IndexT i = 0; i < size; i++)
    {
        this->elements[this->count++] = res[i];
    }
}

//------------------------------------------------------------------------------
/**
*/
template<int MAX_ALLOCS, class TYPE>
inline void 
PinnedArray<MAX_ALLOCS, TYPE>::Append(const TYPE& elm)
{
    // grow allocated space if exhausted
    if (this->count == this->capacity)
    {
        this->Grow();
    }
#if NEBULA_BOUNDSCHECKS
    n_assert(this->elements);
#endif
    this->elements[this->count++] = std::move(elm);
}

//------------------------------------------------------------------------------
/**
*/
template<int MAX_ALLOCS, class TYPE>
inline void 
PinnedArray<MAX_ALLOCS, TYPE>::Append(const TYPE&& elm)
{
    // grow allocated space if exhausted
    if (this->count == this->capacity)
    {
        this->Grow();
    }
#if NEBULA_BOUNDSCHECKS
    n_assert(this->elements);
#endif
    this->elements[this->count++] = std::move(elm);
}

//------------------------------------------------------------------------------
/**
*/
template<int MAX_ALLOCS, class TYPE>
inline TYPE& 
PinnedArray<MAX_ALLOCS, TYPE>::Emplace()
{
    // grow allocated space if exhausted
    if (this->count == this->capacity)
    {
        this->Grow();
    }
#if NEBULA_BOUNDSCHECKS
    n_assert(this->elements);
#endif
    this->elements[this->count] = TYPE();
    return this->elements[this->count++];
}

//------------------------------------------------------------------------------
/**
*/
template<int MAX_ALLOCS, class TYPE>
inline void 
PinnedArray<MAX_ALLOCS, TYPE>::Insert(IndexT index, const TYPE& elm)
{
#if NEBULA_BOUNDSCHECKS
    n_assert(index <= this->count && (index >= 0));
#endif
    if (index == this->count)
    { 
        // special case: append element to back
        this->Append(elm);
    }
    else
    {
        this->Move(index, index + 1);
        this->elements[index] = elm;
    }
}

//------------------------------------------------------------------------------
/**
*/
template<int MAX_ALLOCS, class TYPE>
inline IndexT PinnedArray<MAX_ALLOCS, TYPE>::InsertSorted(const TYPE& elm)
{
    SizeT num = this->Size();
    if (num == 0)
    {
        // array is currently empty
        this->Append(elm);
        return this->Size() - 1;
    }
    else
    {
        IndexT half;
        IndexT lo = 0;
        IndexT hi = num - 1;
        IndexT mid;
        while (lo <= hi)
        {
            if (0 != (half = num / 2))
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
                    // element already exists at [mid], append the
                    // new element to the end of the range
                    return this->InsertAtEndOfIdenticalRange(mid, elm);
                }
            }
            else if (0 != num)
            {
                if (elm < this->elements[lo])
                {
                    this->Insert(lo, elm);
                    return lo;
                }
                else if (elm > this->elements[lo])
                {
                    this->Insert(lo + 1, elm);
                    return lo + 1;
                }
                else
                {
                    // element already exists at [low], append 
                    // the new element to the end of the range
                    return this->InsertAtEndOfIdenticalRange(lo, elm);
                }
            }
            else
            {
#if NEBULA_BOUNDSCHECKS
                n_assert(0 == lo);
#endif
                this->Insert(lo, elm);
                return lo;
            }
        }
        if (elm < this->elements[lo])
        {
            this->Insert(lo, elm);
            return lo;
        }
        else if (elm > this->elements[lo])
        {
            this->Insert(lo + 1, elm);
            return lo + 1;
        }
        else
        {
            // can't happen(?)
        }
    }
    // can't happen
    n_error("Array::InsertSorted: Can't happen!");
    return InvalidIndex;
}

//------------------------------------------------------------------------------
/**
*/
template<int MAX_ALLOCS, class TYPE>
inline IndexT PinnedArray<MAX_ALLOCS, TYPE>::InsertAtEndOfIdenticalRange(IndexT startIndex, const TYPE& elm)
{
    IndexT i = startIndex + 1;
    for (; i < this->count; i++)
    {
        if (this->elements[i] != elm)
        {
            this->Insert(i, elm);
            return i;
        }
    }

    // fallthrough: new element needs to be appended to end
    this->Append(elm);
    return (this->Size() - 1);
}

//------------------------------------------------------------------------------
/**
*/
template<int MAX_ALLOCS, class TYPE>
inline void 
PinnedArray<MAX_ALLOCS, TYPE>::AppendArray(const PinnedArray<MAX_ALLOCS, TYPE>& src)
{
    SizeT neededCapacity = this->count + src.count;
    if (neededCapacity > this->capacity)
    {
        this->GrowTo(neededCapacity);
    }

    // forward elements from array
    IndexT i;
    for (i = 0; i < src.count; i++)
    {
        this->elements[this->count + i] = src.elements[i];
    }
    this->count += src.count;
}

//------------------------------------------------------------------------------
/**
*/
template<int MAX_ALLOCS, class TYPE>
inline void 
PinnedArray<MAX_ALLOCS, TYPE>::AppendArray(const TYPE* arr, const SizeT count)
{
    SizeT neededCapacity = this->count + count;
    if (neededCapacity > this->capacity)
    {
        this->GrowTo(neededCapacity);
    }

    // forward elements from array
    IndexT i;
    for (i = 0; i < count; i++)
    {
        this->elements[this->count + i] = arr[i];
    }
    this->count += count;
}

//------------------------------------------------------------------------------
/**
*/
template<int MAX_ALLOCS, class TYPE>
inline TYPE* 
PinnedArray<MAX_ALLOCS, TYPE>::EmplaceArray(const SizeT count)
{
    SizeT neededCapacity = this->count + count;
    if (neededCapacity > this->capacity)
    {
        this->GrowTo(neededCapacity);
    }

    // forward elements from array
    IndexT i;
    for (i = 0; i < count; i++)
    {
        this->elements[this->count + i] = TYPE();
    }
    TYPE* first = this->elements[this->count];
    this->count += count;
    return first;
}

//------------------------------------------------------------------------------
/**
*/
template<int MAX_ALLOCS, class TYPE>
inline void 
PinnedArray<MAX_ALLOCS, TYPE>::Fill(IndexT first, SizeT num, const TYPE& elm)
{
    if ((first + num) > this->count)
    {
        this->GrowTo(first + num);
        this->count = first + num;
    }

    IndexT i;
    for (i = first; i < (first + num); i++)
    {
        this->elements[i] = elm;
    }
}

//------------------------------------------------------------------------------
/**
*/
template<int MAX_ALLOCS, class TYPE>
inline void 
PinnedArray<MAX_ALLOCS, TYPE>::Reserve(const SizeT count)
{
#if NEBULA_BOUNDSCHECKS
    n_assert(count >= 0);
#endif

    SizeT neededCapacity = this->count + count;
    if (neededCapacity > this->capacity)
    {
        this->GrowTo(neededCapacity);
    }
}

//------------------------------------------------------------------------------
/**
*/
template<int MAX_ALLOCS, class TYPE>
inline void 
PinnedArray<MAX_ALLOCS, TYPE>::Realloc(SizeT capacity, SizeT grow)
{
    this->Delete();
    this->grow = grow;
    this->capacity = capacity;
    this->count = capacity;
    if (this->capacity > 0)
    {
        this->GrowTo(this->capacity);
    }
    else
    {
        this->elements = 0;
    }
}

//------------------------------------------------------------------------------
/**
*/
template<int MAX_ALLOCS, class TYPE>
inline void 
PinnedArray<MAX_ALLOCS, TYPE>::Resize(SizeT num)
{
    if (num < this->count)
    {
        this->DestroyRange(num, this->count);
    }
    else if (num > this->capacity)
    {
        this->GrowTo(num);
    }

    this->count = num;
}

//------------------------------------------------------------------------------
/**
*/
template<int MAX_ALLOCS, class TYPE>
inline void
PinnedArray<MAX_ALLOCS, TYPE>::Extend(SizeT num)
{
    if (num > this->capacity)
    {
        this->GrowTo(num);
    }
    this->count = Math::max(num, this->count);
}

//------------------------------------------------------------------------------
/**
*/
template<int MAX_ALLOCS, class TYPE>
inline void 
PinnedArray<MAX_ALLOCS, TYPE>::Free()
{
    this->Delete();
    this->grow = 16;
}

//------------------------------------------------------------------------------
/**
*/
template<int MAX_ALLOCS, class TYPE>
inline void 
PinnedArray<MAX_ALLOCS, TYPE>::Fit()
{
    // The start offset of the memory has to be aligned to the page size
    SizeT numUsedBytes = Memory::align(this->count * sizeof(TYPE), System::PageSize);
    SizeT numNeededPages = numUsedBytes / System::PageSize;
    SizeT numUsedPages = this->capacity * sizeof(TYPE) / System::PageSize;
    SizeT numFreeablePages = numUsedPages - numNeededPages;

    Memory::DecommitVirtual(((byte*)this->elements) + numUsedBytes, numFreeablePages * System::PageSize);
    this->capacity = this->count;
}

//------------------------------------------------------------------------------
/**
*/
template<int MAX_ALLOCS, class TYPE>
inline void 
PinnedArray<MAX_ALLOCS, TYPE>::Grow()
{
#if NEBULA_BOUNDSCHECKS
    n_assert(this->grow > 0);
#endif

    SizeT growToSize;
    if (0 == this->capacity)
    {
        growToSize = this->grow;
    }
    else
    {
        // grow by half of the current capacity, but never more then MaxGrowSize
        SizeT growBy = this->capacity >> 1;
        if (growBy == 0)
        {
            growBy = Array<TYPE>::MinGrowSize;
        }
        else if (growBy > Array<TYPE>::MaxGrowSize)
        {
            growBy = Array<TYPE>::MaxGrowSize;
        }
        growToSize = this->capacity + growBy;
    }
    this->GrowTo(growToSize);
}

//------------------------------------------------------------------------------
/**
*/
template<int MAX_ALLOCS, class TYPE>
inline void 
PinnedArray<MAX_ALLOCS, TYPE>::GrowTo(SizeT newCapacity)
{
    if (this->elements == nullptr)
    {
        SizeT pageSize = System::PageSize;
        size_t reservationSize = Memory::align(MAX_ALLOCS * sizeof(TYPE), (size_t)pageSize);
        this->elements = (TYPE*)Memory::AllocVirtual(reservationSize);
    }

    SizeT pageSize = System::PageSize;

    // Total amount of bytes needed to fill new capacity
    size_t totalByteSize = newCapacity * sizeof(TYPE);

    // Rounded up to the page size so we don't waste memory we allocate anyways
    size_t totalBytesNeeded = Memory::align(totalByteSize, (size_t)pageSize);

#if NEBULA_DEBUG
    if (totalBytesNeeded > MAX_ALLOCS * sizeof(TYPE))
    {
        n_printf("[PinnedArray] MAX_ALLOCS '%d' and item size '%lu' will waste '%lu' byte(s) due to alignment of page size '%d'", MAX_ALLOCS, sizeof(TYPE), totalBytesNeeded - MAX_ALLOCS * sizeof(TYPE), pageSize);
    }
#endif
    size_t roundedUpNewCapacity = totalBytesNeeded / sizeof(TYPE);
    size_t offset = Memory::align(this->capacity * sizeof(TYPE), (size_t)pageSize);
    if (totalBytesNeeded > offset)
    {
        // The amount of bytes we need to commit is the difference between the new and the old capacity
        size_t commitSize = Memory::align((roundedUpNewCapacity - this->capacity) * sizeof(TYPE), (size_t)pageSize);
        this->capacity = (uint)roundedUpNewCapacity;
        
        Memory::CommitVirtual(((byte*)this->elements) + offset, commitSize);
    }
}

//------------------------------------------------------------------------------
/**
*/
template<int MAX_ALLOCS, class TYPE>
inline void 
PinnedArray<MAX_ALLOCS, TYPE>::Delete()
{
    this->grow = 16;

    if (this->capacity > 0)
    {
        // If in small vector, run destructor
        this->DestroyRange(0, this->count);
        Memory::FreeVirtual(this->elements, this->capacity * sizeof(TYPE));
    }
    this->elements = nullptr;
    this->capacity = 0;
    this->count = 0;
}

//------------------------------------------------------------------------------
/**
*/
template<int MAX_ALLOCS, class TYPE>
inline void 
PinnedArray<MAX_ALLOCS, TYPE>::Copy(const PinnedArray<MAX_ALLOCS, TYPE>& src)
{
#if NEBULA_BOUNDSCHECKS
// Make sure array is either empty, or stack array before copy
    n_assert(this->stackElements.data() == this->elements);
#endif

    this->GrowTo(src.capacity);
    this->grow = src.grow;
    this->count = src.count;
    IndexT i;
    for (i = 0; i < this->count; i++)
    {
        this->elements[i] = src.elements[i];
    }
}

//------------------------------------------------------------------------------
/**
*/
template<int MAX_ALLOCS, class TYPE>
inline void 
PinnedArray<MAX_ALLOCS, TYPE>::Move(IndexT fromIndex, IndexT toIndex)
{
#if NEBULA_BOUNDSCHECKS
    n_assert(this->elements);
    n_assert(fromIndex < this->count);
#endif

// nothing to move?
    if (fromIndex == toIndex)
    {
        return;
    }

    // compute number of elements to move
    SizeT num = this->count - fromIndex;

    // check if array needs to grow
    SizeT neededSize = toIndex + num;
    while (neededSize > this->capacity)
    {
        this->Grow();
    }

    if (fromIndex > toIndex)
    {
        // this is a backward move
        this->MoveRange(&this->elements[toIndex], &this->elements[fromIndex], num);
        this->DestroyRange(fromIndex + num - 1, this->count);
    }
    else
    {
        // this is a forward move
        int i;  // NOTE: this must remain signed for the following loop to work!!!
        for (i = num - 1; i >= 0; --i)
        {
            this->elements[toIndex + i] = this->elements[fromIndex + i];
        }

        // destroy freed elements
        this->DestroyRange(fromIndex, toIndex);
    }

    // adjust array size
    this->count = toIndex + num;
}

} // namespace Util
