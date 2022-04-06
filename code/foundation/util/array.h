#pragma once
//------------------------------------------------------------------------------
/**
    @class Util::Array

    Nebula's dynamic array class. This class is also used by most other
    collection classes.

    The default constructor will not pre-allocate elements, so no space
    is wasted as long as no elements are added. As soon as the first element
    is added to the array, an initial buffer of 16 elements is created.
    Whenever the element buffer would overflow, a new buffer of twice
    the size of the previous buffer is created and the existing elements 
    are then copied over to the new buffer. The element buffer will
    never shrink, the only way to reclaim unused memory is to 
    copy the Array to a new Array object. This is usually not a problem
    since most arrays will oscillate around some specific size, so once
    the array has reached this specific size, no costly memory free or allocs 
    will be performed.

    It is possible to sort the array using the Sort() method, this uses
    std::sort (one of the very few exceptions where the STL is used in
    Nebula).

    One should generally be careful with costly copy operators, the Array
    class (and the other container classes using Array) may do some heavy
    element shuffling in some situations (especially when sorting and erasing
    elements).

    @copyright
    (C) 2006 RadonLabs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/types.h"

//------------------------------------------------------------------------------
namespace Util
{
template<class TYPE> class Array
{
public:
    /// define iterator
    typedef TYPE* Iterator;

    /// constructor with default parameters
    Array();
    /// constuctor with initial size and grow size
    Array(SizeT initialCapacity, SizeT initialGrow);
    /// constructor with initial size, grow size and initial values
    Array(SizeT initialSize, SizeT initialGrow, const TYPE& initialValue);
    /// copy constructor
    Array(const Array<TYPE>& rhs);
    /// move constructor
    Array(Array<TYPE>&& rhs) noexcept;
    /// constructor from initializer list
    Array(std::initializer_list<TYPE> list);
    /// construct an empty fixed array
    Array(std::nullptr_t);
    /// constructor from TYPE pointer and size. @note copies the buffer.
    Array(const TYPE* const buf, SizeT num);
    /// destructor
    ~Array();

    /// assignment operator
    void operator=(const Array<TYPE>& rhs);
    /// move operator
    void operator=(Array<TYPE>&& rhs) noexcept;
    /// [] operator
    TYPE& operator[](IndexT index) const;
    /// [] operator
    TYPE& operator[](IndexT index);
    /// equality operator
    bool operator==(const Array<TYPE>& rhs) const;
    /// inequality operator
    bool operator!=(const Array<TYPE>& rhs) const;
    /// convert to "anything"
    template<typename T> T As() const;

    /// Get element (same as operator[] but as a function)
    TYPE& Get(IndexT index) const;
    /// append element to end of array
    void Append(const TYPE& elm);
    /// append an element which is being forwarded
    void Append(TYPE&& elm);
    /// append the contents of an array to this array
    void AppendArray(const Array<TYPE>& rhs);
    /// append from C array
    void AppendArray(const TYPE* arr, const SizeT count);
    /// Emplace item (create new item and return reference)
    TYPE& Emplace();
    /// Emplace range of items and return pointer to first
    TYPE* EmplaceArray(const SizeT count);
    /// increase capacity to fit N more elements into the array.
    void Reserve(SizeT num);
    
    /// set number of elements (clears existing content)
    void SetSize(SizeT s);
    /// get number of elements in array
    const SizeT Size() const;
    /// return the byte size of the array.
    const SizeT ByteSize() const;
    /// get overall allocated size of array in number of elements
    const SizeT Capacity() const;
    /// return reference to first element
    TYPE& Front() const;
    /// return reference to last element
    TYPE& Back() const;
    /// return true if array empty
    bool IsEmpty() const;
    /// erase element at index, keep sorting intact
    void EraseIndex(IndexT index);
    /// erase element pointed to by iterator, keep sorting intact
    Iterator Erase(Iterator iter);
    /// erase element at index, fill gap by swapping in last element, destroys sorting!
    void EraseIndexSwap(IndexT index);
    /// erase element at iterator, fill gap by swapping in last element, destroys sorting!
    Iterator EraseSwap(Iterator iter);
    /// erase range, excluding the element at end
    void EraseRange(IndexT start, IndexT end);
    /// erase back
    void EraseBack();
    /// erase front
    void EraseFront();
    /// Pop front
    TYPE PopFront();
    /// Pop back
    TYPE PopBack();
    /// insert element before element at index
    void Insert(IndexT index, const TYPE& elm);
    /// insert element into sorted array, return index where element was included
    IndexT InsertSorted(const TYPE& elm);
    /// insert element at the first non-identical position, return index of inclusion position
    IndexT InsertAtEndOfIdenticalRange(IndexT startIndex, const TYPE& elm);
    /// test if the array is sorted, this is a slow operation!
    bool IsSorted() const;
    /// clear array (calls destructors)
    void Clear();
    /// reset array (does NOT call destructors)
    void Reset();
    /// free memory and reset size
    void Free();
    /// return iterator to beginning of array
    Iterator Begin() const;
    /// return iterator to end of array
    Iterator End() const;
    /// find identical element in array, return iterator
    Iterator Find(const TYPE& elm, const IndexT start = 0) const;
    /// find identical element in array, return index, InvalidIndex if not found
    IndexT FindIndex(const TYPE& elm, const IndexT start = 0) const;
    /// find identical element using a specific key type
    template <typename KEYTYPE> IndexT FindIndex(typename std::enable_if<true, const KEYTYPE&>::type elm, const IndexT start = 0) const;
    /// fill array range with element
    void Fill(IndexT first, SizeT num, const TYPE& elm);
    /// clear contents and preallocate with new attributes
    void Realloc(SizeT capacity, SizeT grow);
    /// returns new array with elements which are not in rhs (slow!)
    Array<TYPE> Difference(const Array<TYPE>& rhs);
    /// sort the array
    void Sort();
    /// sort with custom function
    void SortWithFunc(bool (*func)(const TYPE& lhs, const TYPE& rhs));
    /// do a binary search, requires a sorted array
    IndexT BinarySearchIndex(const TYPE& elm) const;
    /// do a binary search using a specific key type
    template <typename KEYTYPE> IndexT BinarySearchIndex(typename std::enable_if<true, const KEYTYPE&>::type elm) const;
    
    /// Set size. Grows array if num is greater than capacity. Calls destroy on all objects at index > num!
    void Resize(SizeT num);

    /// Returns sizeof(TYPE)
    constexpr SizeT TypeSize() const;

    /// for range-based iteration
    Iterator begin() const;
    Iterator end() const;
    size_t size() const;
    void resize(size_t size);

    /// grow array
    void Grow();
protected:
    /// destroy an element (call destructor without freeing memory)
    void Destroy(TYPE* elm);
    /// copy content
    void Copy(const Array<TYPE>& src);
    /// delete content
    void Delete();
    /// grow array to target size
    void GrowTo(SizeT newCapacity);
    /// move elements, grows array if needed
    void Move(IndexT fromIndex, IndexT toIndex);
    /// destroy range of elements
    void DestroyRange(IndexT fromIndex, IndexT toIndex);
    /// copy range
    void CopyRange(TYPE* to, TYPE* from, SizeT num);
    /// move range
    void MoveRange(TYPE* to, TYPE* from, SizeT num);

    static const SizeT MinGrowSize = 16;
    static const SizeT MaxGrowSize = 65536; // FIXME: big grow size needed for mesh tools
    SizeT grow;                             // grow by this number of elements if array exhausted
    SizeT capacity;                         // number of elements allocated
    SizeT count;                            // number of elements in array
    TYPE* elements;                         // pointer to element array
};

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
Array<TYPE>::Array() :
    grow(16),
    capacity(0),
    count(0),
    elements(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
Array<TYPE>::Array(SizeT _capacity, SizeT _grow) :
    grow(_grow),
    capacity(_capacity),
    count(0)
{
    if (0 == this->grow)
    {
        this->grow = 16;
    }
    if (this->capacity > 0)
    {
        this->elements = n_new_array_alloc<TYPE>(this->capacity);
    }
    else
    {
        this->elements = 0;
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
Array<TYPE>::Array(SizeT initialSize, SizeT _grow, const TYPE& initialValue) :
    grow(_grow),
    capacity(initialSize),
    count(initialSize)
{
    if (0 == this->grow)
    {
        this->grow = 16;
    }
    if (initialSize > 0)
    {
        this->elements = n_new_array_alloc<TYPE>(this->capacity);
        IndexT i;
        for (i = 0; i < initialSize; i++)
        {
            this->elements[i] = initialValue;
        }
    }
    else
    {
        this->elements = 0;
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
Array<TYPE>::Array(const TYPE* const buf, SizeT num) :
    grow(16),
    capacity(num),
    count(num)
{
    static_assert(std::is_trivially_copyable<TYPE>::value, "TYPE is not trivially copyable; Util::Array cannot be constructed from pointer of TYPE.");
    this->elements = n_new_array_alloc<TYPE>(this->capacity);
    const SizeT bytes = num * sizeof(TYPE);
    Memory::Copy(buf, this->elements, bytes);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
Array<TYPE>::Array(std::initializer_list<TYPE> list) :
    grow(16),
    capacity((SizeT)list.size()),
    count((SizeT)list.size())
{
    if (this->capacity > 0)
    {
        this->elements = n_new_array_alloc<TYPE>(this->capacity);
        IndexT i;
        for (i = 0; i < this->count; i++)
        {
            this->elements[i] = list.begin()[i];
        }
    }
    else
    {
        this->elements = 0;
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
Array<TYPE>::Array(std::nullptr_t) :
    grow(0),
    capacity(0),
    count(0),
    elements(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
Array<TYPE>::Array(const Array<TYPE>& rhs) :
    grow(0),
    capacity(0),
    count(0),
    elements(0)
{
    this->Copy(rhs);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
Array<TYPE>::Array(Array<TYPE>&& rhs) noexcept :
    grow(rhs.grow),
    capacity(rhs.capacity),
    count(rhs.count),
    elements(rhs.elements)
{
    rhs.elements = nullptr;
    rhs.count = 0;
    rhs.capacity = 0;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> void
Array<TYPE>::Copy(const Array<TYPE>& src)
{
    #if NEBULA_BOUNDSCHECKS
    n_assert(0 == this->elements);
    #endif

    this->grow = src.grow;
    this->capacity = src.capacity;
    this->count = src.count;
    if (this->capacity > 0)
    {
        this->elements = n_new_array_alloc<TYPE>(this->capacity);
        IndexT i;
        for (i = 0; i < this->count; i++)
        {
            this->elements[i] = src.elements[i];
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> void
Array<TYPE>::Delete()
{
    this->grow = 16;
    this->count = 0;
    
    if (this->elements)
    {
        n_new_array_free(this->capacity, this->elements);
        this->elements = 0;
    }
    this->capacity = 0;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> void
Array<TYPE>::Destroy(TYPE* elm)
{
    elm->~TYPE();
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
Array<TYPE>::~Array()
{
    this->Delete();
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> void
Array<TYPE>::Realloc(SizeT _capacity, SizeT _grow)
{
    this->Delete();
    this->grow = _grow;
    this->capacity = _capacity;
    this->count = 0;
    if (this->capacity > 0)
    {
        this->elements = n_new_array_alloc<TYPE>(this->capacity);
    }
    else
    {
        this->elements = 0;
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> void 
Array<TYPE>::operator=(const Array<TYPE>& rhs)
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
template<class TYPE> void
Array<TYPE>::operator=(Array<TYPE>&& rhs) noexcept
{
    if (this != &rhs)
    {
        if (this->elements)
        {
            n_new_array_free(this->capacity, this->elements);
        }
        this->elements = rhs.elements;
        this->grow = rhs.grow;
        this->count = rhs.count;
        this->capacity = rhs.capacity;
        rhs.elements = nullptr;
        rhs.count = 0;
        rhs.capacity = 0;
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> void
Array<TYPE>::GrowTo(SizeT newCapacity)
{
    TYPE* newArray = n_new_array_alloc<TYPE>(newCapacity);
    if (this->elements)
    {
        this->MoveRange(newArray, this->elements, this->count);

        // discard old array
        n_new_array_free(this->capacity, this->elements);
    }
    this->elements  = newArray;
    this->capacity = newCapacity;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> void
Array<TYPE>::Grow()
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
            growBy = MinGrowSize;
        }
        else if (growBy > MaxGrowSize)
        {
            growBy = MaxGrowSize;
        }
        growToSize = this->capacity + growBy;
    }
    this->GrowTo(growToSize);
}

//------------------------------------------------------------------------------
/**
    30-Jan-03   floh    serious bugfixes!
    07-Dec-04   jo      bugfix: neededSize >= this->capacity => neededSize > capacity   
*/
template<class TYPE> void
Array<TYPE>::Move(IndexT fromIndex, IndexT toIndex)
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

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
inline void 
Array<TYPE>::DestroyRange(IndexT fromIndex, IndexT toIndex)
{    
    if constexpr (!std::is_trivially_destructible<TYPE>::value)
    {
        for (IndexT i = fromIndex; i < toIndex; i++)
        {
            this->Destroy(&(this->elements[i]));
        }
    }
#if NEBULA_DEBUG
    else
    {
        Memory::Clear((void*)&this->elements[fromIndex], sizeof(TYPE) * (toIndex - fromIndex));        
    }
#endif
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
inline void 
Array<TYPE>::CopyRange(TYPE* to, TYPE* from, SizeT num)
{
    // this is a backward move
    if constexpr (!std::is_trivially_copyable<TYPE>::value)
    {
        IndexT i;
        for (i = 0; i < num; i++)
        {
            to[i] = from[i];
        }
    }
    else
    {
        Memory::CopyElements(from, to, num);
    }       
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
inline void 
Array<TYPE>::MoveRange(TYPE* to, TYPE* from, SizeT num)
{
    // copy over contents
    if constexpr (!std::is_trivially_move_assignable<TYPE>::value && std::is_move_assignable<TYPE>::value)
    {
        IndexT i;
        for (i = 0; i < num; i++)
        {
            to[i] = std::move(from[i]);
        }
    }
    else
    {
        Memory::MoveElements(from, to, num);
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
inline TYPE& Array<TYPE>::Get(IndexT index) const
{
#if NEBULA_BOUNDSCHECKS
    n_assert(this->elements != nullptr);
    n_assert(this->capacity > index);
#endif
    return this->elements[index];
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> 
void
Array<TYPE>::Append(const TYPE& elm)
{
    // grow allocated space if exhausted
    if (this->count == this->capacity)
    {
        this->Grow();
    }
    #if NEBULA_BOUNDSCHECKS
    n_assert(this->elements);
    #endif
    this->elements[this->count++] = elm;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> 
void 
Array<TYPE>::Append(TYPE&& elm)
{
    // grow allocated space if exhausted
    if (this->count == this->capacity)
    {
        this->Grow();
    }
#if NEBULA_BOUNDSCHECKS
    n_assert(this->elements);
#endif
    this->elements[this->count++] = std::forward<TYPE>(elm);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> 
void
Array<TYPE>::AppendArray(const Array<TYPE>& rhs)
{
    SizeT neededCapacity = this->count + rhs.count;
    if (neededCapacity > this->capacity)
    {
        this->GrowTo(neededCapacity);
    }

    // forward elements from array
    IndexT i;
    for (i = 0; i < rhs.count; i++)
    {
        this->elements[this->count + i] = rhs.elements[i];
    }
    this->count += rhs.count;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> 
void
Array<TYPE>::AppendArray(const TYPE* arr, const SizeT count)
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
template<class TYPE>
TYPE& 
Array<TYPE>::Emplace()
{
// grow allocated space if exhausted
    if (this->count == this->capacity)
    {
        this->Grow();
    }
#if NEBULA_BOUNDSCHECKS
    n_assert(this->elements);
#endif
    this->elements[count] = TYPE();
    return this->elements[this->count++];
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
TYPE* 
Array<TYPE>::EmplaceArray(const SizeT count)
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
    This increases the capacity to make room for N elements. If the
    number of elements is known before appending the elements, this 
    method can be used to prevent reallocation. If there is already
    enough room for N more elements, nothing will happen.
    
    NOTE: the functionality of this method has been changed as of 26-Apr-08,
    it will now only change the capacity of the array, not its size.
*/
template<class TYPE> void
Array<TYPE>::Reserve(SizeT num)
{
#if NEBULA_BOUNDSCHECKS
    n_assert(num >= 0);
#endif
    
    SizeT neededCapacity = this->count + num;
    if (neededCapacity > this->capacity)
    {
        this->GrowTo(neededCapacity);
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> const SizeT
Array<TYPE>::Size() const
{
    return this->count;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> const SizeT
Array<TYPE>::ByteSize() const
{
    return this->count * sizeof(TYPE);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> const SizeT
Array<TYPE>::Capacity() const
{
    return this->capacity;
}

//------------------------------------------------------------------------------
/**
    Access an element. This method will NOT grow the array, and instead do
    a range check, which may throw an assertion.
*/
template<class TYPE> TYPE&
Array<TYPE>::operator[](IndexT index) const
{
    #if NEBULA_BOUNDSCHECKS
    n_assert(this->elements && (index < this->count) && (index >= 0));
    #endif
    return this->elements[index];
}

//------------------------------------------------------------------------------
/**
    Access an element. This method will NOT grow the array, and instead do
    a range check, which may throw an assertion.
*/
template<class TYPE> TYPE&
Array<TYPE>::operator[](IndexT index) 
{
#if NEBULA_BOUNDSCHECKS
    n_assert(this->elements && (index < this->count) && (index >= 0));
#endif
    return this->elements[index];
}

//------------------------------------------------------------------------------
/**
    The equality operator returns true if all elements are identical. The
    TYPE class must support the equality operator.
*/
template<class TYPE> bool
Array<TYPE>::operator==(const Array<TYPE>& rhs) const
{
    if (rhs.Size() == this->Size())
    {
        IndexT i;
        SizeT num = this->Size();
        for (i = 0; i < num; i++)
        {
            if (!(this->elements[i] == rhs.elements[i]))
            {
                return false;
            }
        }
        return true;
    }
    else
    {
        return false;
    }
}

//------------------------------------------------------------------------------
/**
    The inequality operator returns true if at least one element in the 
    array is different, or the array sizes are different.
*/
template<class TYPE> bool
Array<TYPE>::operator!=(const Array<TYPE>& rhs) const
{
    return !(*this == rhs);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> TYPE&
Array<TYPE>::Front() const
{
    #if NEBULA_BOUNDSCHECKS
    n_assert(this->elements && (this->count > 0));
    #endif
    return this->elements[0];
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> TYPE&
Array<TYPE>::Back() const
{
    #if NEBULA_BOUNDSCHECKS
    n_assert(this->elements && (this->count > 0));
    #endif
    return this->elements[this->count - 1];
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> bool 
Array<TYPE>::IsEmpty() const
{
    return (this->count == 0);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> void
Array<TYPE>::EraseIndex(IndexT index)
{
    #if NEBULA_BOUNDSCHECKS
    n_assert(this->elements && (index < this->count) && (index >= 0));
    #endif
    if (index == (this->count - 1))
    {
        // special case: last element
        this->EraseBack();
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
Array<TYPE>::EraseIndexSwap(IndexT index)
{
    #if NEBULA_BOUNDSCHECKS
    n_assert(this->elements && (index < this->count) && (index >= 0));
    #endif

    // swap with last element, and destroy last element
    IndexT lastElementIndex = this->count - 1;
    if (index < lastElementIndex)
    {
        if constexpr (!std::is_trivially_move_assignable<TYPE>::value)
            this->elements[index] = std::move(this->elements[lastElementIndex]);
        else
            this->elements[index] = this->elements[lastElementIndex];
    }
    this->count--;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> typename Array<TYPE>::Iterator
Array<TYPE>::Erase(typename Array<TYPE>::Iterator iter)
{
    #if NEBULA_BOUNDSCHECKS
    n_assert(this->elements && (iter >= this->elements) && (iter < (this->elements + this->count)));
    #endif
    this->EraseIndex(IndexT(iter - this->elements));
    return iter;
}

//------------------------------------------------------------------------------
/**
    NOTE: this method is fast but destroys the sorting order!
*/
template<class TYPE> typename Array<TYPE>::Iterator
Array<TYPE>::EraseSwap(typename Array<TYPE>::Iterator iter)
{
    #if NEBULA_BOUNDSCHECKS
    n_assert(this->elements && (iter >= this->elements) && (iter < (this->elements + this->count)));
    #endif
    this->EraseIndexSwap(IndexT(iter - this->elements));
    return iter;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> void 
Array<TYPE>::EraseRange(IndexT start, IndexT end)
{
    n_assert(end >= start);
    n_assert(end < this->count);
    if (start == end)
        this->EraseIndex(start);
    else
    {
        // add 1 to end to remove and move including that element
        this->DestroyRange(start, end);
        SizeT numMove = this->count - end;
        this->MoveRange(&this->elements[start], &this->elements[end], numMove);
        this->count -= end - start;
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> void
Array<TYPE>::EraseBack()
{
    n_assert(this->count > 0);
    if constexpr (!std::is_trivially_destructible<TYPE>::value)
        this->Destroy(&(this->elements[this->count - 1]));
    this->count--;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> void
Array<TYPE>::EraseFront()
{
    this->EraseIndex(0);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
inline TYPE 
Array<TYPE>::PopFront()
{
    TYPE ret = std::move(this->elements[0]);
    this->EraseIndex(0);
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
inline TYPE 
Array<TYPE>::PopBack()
{
    this->count--;
    return std::move(this->elements[this->count - 1]);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> void
Array<TYPE>::Insert(IndexT index, const TYPE& elm)
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
    The current implementation of this method does not shrink the 
    preallocated space. It simply sets the array _size to 0.
*/
template<class TYPE> void
Array<TYPE>::Clear()
{
    this->DestroyRange(0, this->count);
    this->count = 0;
}

//------------------------------------------------------------------------------
/**
    This is identical with Clear(), but does NOT call destructors (it just
    resets the _size member. USE WITH CARE!
*/
template<class TYPE> void
Array<TYPE>::Reset()
{
    this->count = 0;
}

//------------------------------------------------------------------------------
/**
    Free up memory and reset the grow
*/
template<class TYPE> void 
Array<TYPE>::Free()
{
    this->Delete();
    this->grow = 8;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> typename Array<TYPE>::Iterator
Array<TYPE>::Begin() const
{
    return this->elements;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> typename Array<TYPE>::Iterator
Array<TYPE>::End() const
{
    return this->elements + this->count;
}

//------------------------------------------------------------------------------
/**
    Find element in array, return iterator, or 0 if element not
    found.

    @param  elm     element to find
    @return         element iterator, or 0 if not found
*/
template<class TYPE> typename Array<TYPE>::Iterator
Array<TYPE>::Find(const TYPE& elm, const IndexT start) const
{
    n_assert(start <= this->count);
    IndexT index;
    for (index = start; index < this->count; index++)
    {
        if (this->elements[index] == elm)
        {
            return &(this->elements[index]);
        }
    }
    return 0;
}

//------------------------------------------------------------------------------
/**
    Find element in array, return element index, or InvalidIndex if element not
    found.

    @param  elm     element to find
    @return         index to element, or InvalidIndex if not found
*/
template<class TYPE> IndexT
Array<TYPE>::FindIndex(const TYPE& elm, const IndexT start) const
{
    n_assert(start <= this->count);
    IndexT index;
    for (index = start; index < this->count; index++)
    {
        if (this->elements[index] == elm)
        {
            return index;
        }
    }
    return InvalidIndex;
}

//------------------------------------------------------------------------------
/**
    Find element in array, return element index, or InvalidIndex if element not
    found.

    Template type is used to force a specific type comparison. This might mitigate
    some expensive implicit constructions to TYPE.

    This templated method requires a explicit template type, which is enforced
    by using typename to put the template type in a non-deducable context.
    The enable_if does nothing except allow us to use typename.

    @param  elm     element to find
    @return         index to element, or InvalidIndex if not found
*/
template<class TYPE>
template<typename KEYTYPE> inline IndexT
Array<TYPE>::FindIndex(typename std::enable_if<true, const KEYTYPE&>::type elm, const IndexT start) const
{
    n_assert(start <= this->count);
    IndexT index;
    for (index = start; index < this->count; index++)
    {
        if (this->elements[index] == elm)
        {
            return index;
        }
    }
    return InvalidIndex;
}

//------------------------------------------------------------------------------
/**
    Fills an array range with the given element value. Will grow the
    array if necessary

    @param  first   index of first element to start fill
    @param  num     num elements to fill
    @param  elm     fill value
*/
template<class TYPE> void
Array<TYPE>::Fill(IndexT first, SizeT num, const TYPE& elm)
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
    Returns a new array with all element which are in rhs, but not in this.
    Carefull, this method may be very slow with large arrays!

    @todo this method is broken, check test case to see why!
*/
template<class TYPE> Array<TYPE>
Array<TYPE>::Difference(const Array<TYPE>& rhs)
{
    Array<TYPE> diff;
    IndexT i;
    SizeT num = rhs.Size();
    for (i = 0; i < num; i++)
    {
        if (0 == this->Find(rhs[i]))
        {
            diff.Append(rhs[i]);
        }
    }
    return diff;
}

//------------------------------------------------------------------------------
/**
    Sorts the array. This just calls the STL sort algorithm.
*/
template<class TYPE> void
Array<TYPE>::Sort()
{
    std::sort(this->Begin(), this->End());
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> void
Util::Array<TYPE>::SortWithFunc(bool (*func)(const TYPE& lhs, const TYPE& rhs))
{
    std::sort(this->Begin(), this->End(), func);
}

//------------------------------------------------------------------------------
/**
    Does a binary search on the array, returns the index of the identical
    element, or InvalidIndex if not found
*/
template<class TYPE> IndexT
Array<TYPE>::BinarySearchIndex(const TYPE& elm) const
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
            else if (0 != num) 
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
    Template type is used to force a specific type comparison. This might mitigate
    some expensive implicit constructions to TYPE.

    This templated method requires a explicit template type, which is enforced
    by using typename to put the template type in a non-deducable context.
    The enable_if does nothing except allow us to use typename.
*/
template<class TYPE>
template<typename KEYTYPE> inline IndexT 
Array<TYPE>::BinarySearchIndex(typename std::enable_if<true, const KEYTYPE&>::type elm) const
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
            if (0 != (half = num / 2))
            {
                mid = lo + ((num & 1) ? half : (half - 1));
                if (this->elements[mid] > elm)
                {
                    hi = mid - 1;
                    num = num & 1 ? half : half - 1;
                }
                else if (this->elements[mid] < elm)
                {
                    lo = mid + 1;
                    num = half;
                }
                else
                {
                    return mid;
                }
            }
            else if (0 != num)
            {
                if (this->elements[lo] != elm)
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
template<class TYPE> void
Array<TYPE>::Resize(SizeT num)
{
    if (num < this->count)
    {
        this->DestroyRange(num, this->count);
    }
    else if (num > capacity)
    {
        this->GrowTo(num);
    }

    this->count = num;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> void
Array<TYPE>::SetSize(SizeT s)
{
    this->Realloc(s, 8);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> inline constexpr SizeT 
Array<TYPE>::TypeSize() const
{
    return sizeof(TYPE);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> size_t
Array<TYPE>::size() const
{
    return this->count;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> typename Array<TYPE>::Iterator
Array<TYPE>::begin() const
{
    return this->elements;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> typename Array<TYPE>::Iterator
Array<TYPE>::end() const
{
    return this->elements + this->count;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> void
Array<TYPE>::resize(size_t s)
{
    if (static_cast<SizeT>(s) > this->capacity)
    {
        this->GrowTo(static_cast<SizeT>(s));
    }
    this->count = static_cast<SizeT>(s);
}


//------------------------------------------------------------------------------
/**
    This tests, whether the array is sorted. This is a slow operation
    O(n).
*/
template<class TYPE> bool
Array<TYPE>::IsSorted() const
{
    if (this->count > 1)
    {
        IndexT i;
        for (i = 0; i < this->count - 1; i++)
        {
            if (this->elements[i] > this->elements[i + 1])
            {
                return false;
            }
        }
    }
    return true;
}

//------------------------------------------------------------------------------
/**
    This inserts an element at the end of a range of identical elements
    starting at a given index. Performance is O(n). Returns the index
    at which the element was added.
*/
template<class TYPE> IndexT
Array<TYPE>::InsertAtEndOfIdenticalRange(IndexT startIndex, const TYPE& elm)
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
    This inserts the element into a sorted array. Returns the index
    at which the element was inserted.
*/
template<class TYPE> IndexT
Array<TYPE>::InsertSorted(const TYPE& elm)
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


} // namespace Core
//------------------------------------------------------------------------------
