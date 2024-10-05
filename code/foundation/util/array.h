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
#include "system/systeminfo.h"
#include "math/scalar.h"
#include <type_traits>



//------------------------------------------------------------------------------
namespace Util
{

template<class TYPE, int STACK_SIZE>
struct _smallvector
{
    TYPE* data() { return stackElements; }
private:
    TYPE stackElements[STACK_SIZE];
};

template<class TYPE>
struct _smallvector<TYPE, 0>
{
    TYPE* data() { return nullptr; }
};

template<class TYPE, int SMALL_VECTOR_SIZE = 0> class Array
{
public:
    /// define iterator
    typedef TYPE* Iterator;
    typedef const TYPE* ConstIterator;

    using ArrayT = Array<TYPE, SMALL_VECTOR_SIZE>;

    /// constructor with default parameters
    Array();
    /// constuctor with initial size and grow size
    Array(SizeT initialCapacity, SizeT initialGrow);
    /// constructor with initial size, grow size and initial values
    Array(SizeT initialSize, SizeT initialGrow, const TYPE& initialValue);
    /// copy constructor
    Array(const ArrayT& rhs);
    /// move constructor
    Array(ArrayT&& rhs) noexcept;
    /// constructor from initializer list
    Array(std::initializer_list<TYPE> list);
    /// construct an empty fixed array
    Array(std::nullptr_t);
    /// constructor from TYPE pointer and size. @note copies the buffer.
    Array(const TYPE* const buf, SizeT num);
    /// destructor
    ~Array();

    /// assignment operator
    void operator=(const Array<TYPE, SMALL_VECTOR_SIZE>& rhs);
    /// move operator
    void operator=(Array<TYPE, SMALL_VECTOR_SIZE>&& rhs) noexcept;
    /// [] operator
    TYPE& operator[](IndexT index) const;
    /// [] operator
    TYPE& operator[](IndexT index);
    /// equality operator
    bool operator==(const Array<TYPE, SMALL_VECTOR_SIZE>& rhs) const;
    /// inequality operator
    bool operator!=(const Array<TYPE, SMALL_VECTOR_SIZE>& rhs) const;
    /// convert to "anything"
    template<typename T> T As() const;

    /// Get element (same as operator[] but as a function)
    TYPE& Get(IndexT index) const;

    /// Append multiple elements to the end of the array
    template <typename ...ELEM_TYPE>
    void Append(const TYPE& first, const ELEM_TYPE&... elements);
    /// append element to end of array
    void Append(const TYPE& elm);
    /// append an element which is being forwarded
    void Append(TYPE&& elm);
    /// append the contents of an array to this array
    void AppendArray(const Array<TYPE, SMALL_VECTOR_SIZE>& rhs);
    /// append from C array
    void AppendArray(const TYPE* arr, const SizeT count);
    /// Emplace item (create new item and return reference)
    TYPE& Emplace();
    /// Emplace range of items and return pointer to first
    TYPE* EmplaceArray(const SizeT count);
    /// increase capacity to fit N more elements into the array.
    void Reserve(SizeT num);
    
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
    /// check if index is valid
    bool IsValidIndex(IndexT index) const;
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
    /// return const iterator to beginning of array
    ConstIterator ConstBegin() const;
    /// return iterator to end of array
    Iterator End() const;
    /// return const iterator to end of array
    ConstIterator ConstEnd() const;
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
    ArrayT Difference(const Array<TYPE, SMALL_VECTOR_SIZE>& rhs);
    /// sort the array
    void Sort();
    /// quick sort the array
    void QuickSort();
    /// sort with custom function
    void SortWithFunc(bool (*func)(const TYPE& lhs, const TYPE& rhs));
    /// quick sort the array
    void QuickSortWithFunc(int (*func)(const void* lhs, const void* rhs));
    /// do a binary search, requires a sorted array
    IndexT BinarySearchIndex(const TYPE& elm) const;
    /// do a binary search using a specific key type
    template <typename KEYTYPE> IndexT BinarySearchIndex(typename std::enable_if<true, const KEYTYPE&>::type elm) const;
    
    /// Set size. Grows array if num is greater than capacity. Calls destroy on all objects at index > num!
    void Resize(SizeT num);
    /// Resize and fill new elements with arguments
    template <typename ...ARGS> void Resize(SizeT num, ARGS... args);
    /// Resize to fit the provided value, but don't shrink if the new size is smaller
    void Extend(SizeT num);
    /// Fit the size of the array to the amount of elements
    void Fit();

    /// Returns sizeof(TYPE)
    constexpr SizeT TypeSize() const;

    /// for range-based iteration
    Iterator begin() const;
    Iterator end() const;
    size_t size() const;
    void resize(size_t size);
    void clear() noexcept;
    void push_back(const TYPE& item);

    /// grow array with grow value
    void Grow();
protected:
    template<class T, bool S>
    friend class FixedArray;

    /// destroy an element (call destructor without freeing memory)
    void Destroy(TYPE* elm);
    /// copy content
    void Copy(const Array<TYPE, SMALL_VECTOR_SIZE>& src);
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

    _smallvector<TYPE, SMALL_VECTOR_SIZE> stackElements;
};

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, int SMALL_VECTOR_SIZE>
Array<TYPE, SMALL_VECTOR_SIZE>::Array() :
    grow(16),
    capacity(SMALL_VECTOR_SIZE),
    count(0),
    elements(this->stackElements.data())
{
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, int SMALL_VECTOR_SIZE>
Array<TYPE, SMALL_VECTOR_SIZE>::Array(SizeT _capacity, SizeT _grow) :
    grow(_grow),
    capacity(SMALL_VECTOR_SIZE),
    count(0),
    elements(this->stackElements.data())
{
    if (0 == this->grow)
    {
        this->grow = 16;
    }
    this->GrowTo(_capacity);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, int SMALL_VECTOR_SIZE>
Array<TYPE, SMALL_VECTOR_SIZE>::Array(SizeT initialSize, SizeT _grow, const TYPE& initialValue) :
    grow(_grow),
    capacity(SMALL_VECTOR_SIZE),
    count(0),
    elements(stackElements.data())
{
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
template<class TYPE, int SMALL_VECTOR_SIZE>
Array<TYPE, SMALL_VECTOR_SIZE>::Array(const TYPE* const buf, SizeT num) :
    grow(16),
    capacity(SMALL_VECTOR_SIZE),
    count(0),
    elements(stackElements.data())
{
    static_assert(std::is_trivially_copyable<TYPE>::value, "TYPE is not trivially copyable; Util::Array cannot be constructed from pointer of TYPE.");
    this->GrowTo(num);
    this->count = num;
    const SizeT bytes = num * sizeof(TYPE);
    Memory::Copy(buf, this->elements, bytes);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, int SMALL_VECTOR_SIZE>
Array<TYPE, SMALL_VECTOR_SIZE>::Array(std::initializer_list<TYPE> list) :
    grow(16),
    capacity(SMALL_VECTOR_SIZE),
    count(0),
    elements(stackElements.data())
{
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
template<class TYPE, int SMALL_VECTOR_SIZE>
Array<TYPE, SMALL_VECTOR_SIZE>::Array(std::nullptr_t) :
    grow(16),
    capacity(SMALL_VECTOR_SIZE),
    count(0),
    elements(stackElements.data())
{
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, int SMALL_VECTOR_SIZE>
Array<TYPE, SMALL_VECTOR_SIZE>::Array(const Array<TYPE, SMALL_VECTOR_SIZE>& rhs) :
    grow(16),
    capacity(SMALL_VECTOR_SIZE),
    count(0),
    elements(this->stackElements.data())
{
    this->Copy(rhs);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, int SMALL_VECTOR_SIZE>
Array<TYPE, SMALL_VECTOR_SIZE>::Array(Array<TYPE, SMALL_VECTOR_SIZE>&& rhs) noexcept :
    grow(rhs.grow),
    capacity(rhs.capacity),
    count(rhs.count),
    elements(this->stackElements.data())
{
    // If data is on the stack, copy data over
    if (rhs.capacity <= SMALL_VECTOR_SIZE)
    {
        for (IndexT i = 0; i < rhs.capacity; i++)
            this->elements[i] = rhs.elements[i];
    }
    else
    {
        // Otherwise, exchange pointers and invalidate
        this->elements = rhs.elements;
        rhs.elements = rhs.stackElements.data();
    }
    rhs.count = 0;
    rhs.capacity = SMALL_VECTOR_SIZE;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, int SMALL_VECTOR_SIZE> 
void
Array<TYPE, SMALL_VECTOR_SIZE>::Copy(const Array<TYPE, SMALL_VECTOR_SIZE>& src)
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
template<class TYPE, int SMALL_VECTOR_SIZE> 
void
Array<TYPE, SMALL_VECTOR_SIZE>::Delete()
{
    this->grow = 16;
    
    if (this->capacity > 0)
    {
        if (this->elements != this->stackElements.data())
        {
            ArrayFree(this->capacity, this->elements);
        }
        else
        {
            // If in small vector, run destructor
            this->DestroyRange(0, this->count);
        }   
    }
    this->elements = this->stackElements.data();
    this->count = 0;
    this->capacity = SMALL_VECTOR_SIZE;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, int SMALL_VECTOR_SIZE> void
Array<TYPE, SMALL_VECTOR_SIZE>::Destroy(TYPE* elm)
{
    elm->~TYPE();
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, int SMALL_VECTOR_SIZE>
Array<TYPE, SMALL_VECTOR_SIZE>::~Array()
{
    this->Delete();
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, int SMALL_VECTOR_SIZE> void
Array<TYPE, SMALL_VECTOR_SIZE>::Realloc(SizeT _capacity, SizeT _grow)
{
    this->Delete();
    this->grow = _grow;
    this->capacity = _capacity;
    this->count = _capacity;
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
template<class TYPE, int SMALL_VECTOR_SIZE> void 
Array<TYPE, SMALL_VECTOR_SIZE>::operator=(const Array<TYPE, SMALL_VECTOR_SIZE>& rhs)
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
template<class TYPE, int SMALL_VECTOR_SIZE> void
Array<TYPE, SMALL_VECTOR_SIZE>::operator=(Array<TYPE, SMALL_VECTOR_SIZE>&& rhs) noexcept
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
template<class TYPE, int SMALL_VECTOR_SIZE> void
Array<TYPE, SMALL_VECTOR_SIZE>::GrowTo(SizeT newCapacity)
{
    if (newCapacity > SMALL_VECTOR_SIZE)
    {
        TYPE* newArray = ArrayAlloc<TYPE>(newCapacity);
        if (this->elements)
        {
            this->MoveRange(newArray, this->elements, this->count);

            // discard old array if not the stack array
            if (this->elements != this->stackElements.data())
                ArrayFree(this->capacity, this->elements);
        }
        this->elements = newArray;
        this->capacity = newCapacity;
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, int SMALL_VECTOR_SIZE> 
void
Array<TYPE, SMALL_VECTOR_SIZE>::Grow()
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
template<class TYPE, int SMALL_VECTOR_SIZE> 
void
Array<TYPE, SMALL_VECTOR_SIZE>::Move(IndexT fromIndex, IndexT toIndex)
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
template<class TYPE, int SMALL_VECTOR_SIZE>
inline void 
Array<TYPE, SMALL_VECTOR_SIZE>::DestroyRange(IndexT fromIndex, IndexT toIndex)
{    
    if constexpr (!std::is_trivially_destructible<TYPE>::value)
    {
        for (IndexT i = fromIndex; i < toIndex; i++)
        {
            this->Destroy(&(this->elements[i]));
        }
    }
    else
    {
        Memory::Clear((void*)&this->elements[fromIndex], sizeof(TYPE) * (toIndex - fromIndex));        
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, int SMALL_VECTOR_SIZE>
inline void 
Array<TYPE, SMALL_VECTOR_SIZE>::CopyRange(TYPE* to, TYPE* from, SizeT num)
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
template<class TYPE, int SMALL_VECTOR_SIZE>
inline void 
Array<TYPE, SMALL_VECTOR_SIZE>::MoveRange(TYPE* to, TYPE* from, SizeT num)
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
template<class TYPE, int SMALL_VECTOR_SIZE>
inline TYPE& 
Array<TYPE, SMALL_VECTOR_SIZE>::Get(IndexT index) const
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
template<class TYPE, int SMALL_VECTOR_SIZE> 
void
Array<TYPE, SMALL_VECTOR_SIZE>::Append(const TYPE& elm)
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
template<class TYPE, int SMALL_VECTOR_SIZE> 
void 
Array<TYPE, SMALL_VECTOR_SIZE>::Append(TYPE&& elm)
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
template<class TYPE, int SMALL_VECTOR_SIZE> 
void
Array<TYPE, SMALL_VECTOR_SIZE>::AppendArray(const Array<TYPE, SMALL_VECTOR_SIZE>& rhs)
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
template<class TYPE, int SMALL_VECTOR_SIZE> 
void
Array<TYPE, SMALL_VECTOR_SIZE>::AppendArray(const TYPE* arr, const SizeT count)
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
template<class TYPE, int SMALL_VECTOR_SIZE>
TYPE& 
Array<TYPE, SMALL_VECTOR_SIZE>::Emplace()
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
template<class TYPE, int SMALL_VECTOR_SIZE>
TYPE* 
Array<TYPE, SMALL_VECTOR_SIZE>::EmplaceArray(const SizeT count)
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
template<class TYPE, int SMALL_VECTOR_SIZE> 
void
Array<TYPE, SMALL_VECTOR_SIZE>::Reserve(SizeT num)
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
template<class TYPE, int SMALL_VECTOR_SIZE> 
const SizeT
Array<TYPE, SMALL_VECTOR_SIZE>::Size() const
{
    return this->count;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, int SMALL_VECTOR_SIZE> 
const SizeT
Array<TYPE, SMALL_VECTOR_SIZE>::ByteSize() const
{
    return this->count * sizeof(TYPE);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, int SMALL_VECTOR_SIZE> 
const SizeT
Array<TYPE, SMALL_VECTOR_SIZE>::Capacity() const
{
    return this->capacity;
}

//------------------------------------------------------------------------------
/**
    Access an element. This method will NOT grow the array, and instead do
    a range check, which may throw an assertion.
*/
template<class TYPE, int SMALL_VECTOR_SIZE> 
TYPE&
Array<TYPE, SMALL_VECTOR_SIZE>::operator[](IndexT index) const
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
template<class TYPE, int SMALL_VECTOR_SIZE> 
TYPE&
Array<TYPE, SMALL_VECTOR_SIZE>::operator[](IndexT index) 
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
template<class TYPE, int SMALL_VECTOR_SIZE> 
bool
Array<TYPE, SMALL_VECTOR_SIZE>::operator==(const Array<TYPE, SMALL_VECTOR_SIZE>& rhs) const
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
template<class TYPE, int SMALL_VECTOR_SIZE> 
bool
Array<TYPE, SMALL_VECTOR_SIZE>::operator!=(const Array<TYPE, SMALL_VECTOR_SIZE>& rhs) const
{
    return !(*this == rhs);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, int SMALL_VECTOR_SIZE> 
TYPE&
Array<TYPE, SMALL_VECTOR_SIZE>::Front() const
{
    #if NEBULA_BOUNDSCHECKS
    n_assert(this->elements && (this->count > 0));
    #endif
    return this->elements[0];
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, int SMALL_VECTOR_SIZE> 
TYPE&
Array<TYPE, SMALL_VECTOR_SIZE>::Back() const
{
    #if NEBULA_BOUNDSCHECKS
    n_assert(this->elements && (this->count > 0));
    #endif
    return this->elements[this->count - 1];
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, int SMALL_VECTOR_SIZE>
void
Array<TYPE, SMALL_VECTOR_SIZE>::push_back(const TYPE& item)
{
    this->Append(item);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, int SMALL_VECTOR_SIZE> 
bool 
Array<TYPE, SMALL_VECTOR_SIZE>::IsEmpty() const
{
    return (this->count == 0);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, int SMALL_VECTOR_SIZE>
bool
Array<TYPE, SMALL_VECTOR_SIZE>::IsValidIndex(IndexT index) const
{
    return this->elements && (index < this->count) && (index >= 0);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, int SMALL_VECTOR_SIZE> 
void
Array<TYPE, SMALL_VECTOR_SIZE>::EraseIndex(IndexT index)
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
template<class TYPE, int SMALL_VECTOR_SIZE>
void
Array<TYPE, SMALL_VECTOR_SIZE>::EraseIndexSwap(IndexT index)
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
template<class TYPE, int SMALL_VECTOR_SIZE> 
typename Array<TYPE, SMALL_VECTOR_SIZE>::Iterator
Array<TYPE, SMALL_VECTOR_SIZE>::Erase(typename Array<TYPE, SMALL_VECTOR_SIZE>::Iterator iter)
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
template<class TYPE, int SMALL_VECTOR_SIZE> 
typename Array<TYPE, SMALL_VECTOR_SIZE>::Iterator
Array<TYPE, SMALL_VECTOR_SIZE>::EraseSwap(typename Array<TYPE, SMALL_VECTOR_SIZE>::Iterator iter)
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
template<class TYPE, int SMALL_VECTOR_SIZE> 
void 
Array<TYPE, SMALL_VECTOR_SIZE>::EraseRange(IndexT start, IndexT end)
{
    n_assert(end >= start);
    n_assert(end <= this->count);
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
template<class TYPE, int SMALL_VECTOR_SIZE> 
void
Array<TYPE, SMALL_VECTOR_SIZE>::EraseBack()
{
    n_assert(this->count > 0);
    if constexpr (!std::is_trivially_destructible<TYPE>::value)
        this->Destroy(&(this->elements[this->count - 1]));
    this->count--;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, int SMALL_VECTOR_SIZE> 
void
Array<TYPE, SMALL_VECTOR_SIZE>::EraseFront()
{
    this->EraseIndex(0);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, int SMALL_VECTOR_SIZE>
inline TYPE 
Array<TYPE, SMALL_VECTOR_SIZE>::PopFront()
{
#if NEBULA_BOUNDSCHECKS
    n_assert(this->count > 0);
#endif
    TYPE ret = std::move(this->elements[0]);
    this->EraseIndex(0);
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, int SMALL_VECTOR_SIZE>
inline TYPE 
Array<TYPE, SMALL_VECTOR_SIZE>::PopBack()
{
#if NEBULA_BOUNDSCHECKS
    n_assert(this->count > 0);
#endif
    this->count--;
    return std::move(this->elements[this->count]);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, int SMALL_VECTOR_SIZE> 
void
Array<TYPE, SMALL_VECTOR_SIZE>::Insert(IndexT index, const TYPE& elm)
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
template<class TYPE, int SMALL_VECTOR_SIZE> 
void
Array<TYPE, SMALL_VECTOR_SIZE>::Clear()
{
    if (this->count > 0)
    {
        this->DestroyRange(0, this->count);
        this->count = 0;
    }
}

//------------------------------------------------------------------------------
/**
    This is identical with Clear(), but does NOT call destructors (it just
    resets the _size member. USE WITH CARE!
*/
template<class TYPE, int SMALL_VECTOR_SIZE> 
void
Array<TYPE, SMALL_VECTOR_SIZE>::Reset()
{
    this->count = 0;
}

//------------------------------------------------------------------------------
/**
    Free up memory and reset the grow
*/
template<class TYPE, int SMALL_VECTOR_SIZE> 
void 
Array<TYPE, SMALL_VECTOR_SIZE>::Free()
{
    this->Delete();
    this->grow = 16;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, int SMALL_VECTOR_SIZE> 
typename Array<TYPE, SMALL_VECTOR_SIZE>::Iterator
Array<TYPE, SMALL_VECTOR_SIZE>::Begin() const
{
    return this->elements;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, int SMALL_VECTOR_SIZE> 
typename Array<TYPE, SMALL_VECTOR_SIZE>::ConstIterator
Array<TYPE, SMALL_VECTOR_SIZE>::ConstBegin() const
{
    return static_cast<Array<TYPE, SMALL_VECTOR_SIZE>::ConstIterator>(this->elements);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, int SMALL_VECTOR_SIZE> 
typename Array<TYPE, SMALL_VECTOR_SIZE>::Iterator
Array<TYPE, SMALL_VECTOR_SIZE>::End() const
{
    return this->elements + this->count;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, int SMALL_VECTOR_SIZE> 
typename Array<TYPE, SMALL_VECTOR_SIZE>::ConstIterator
Array<TYPE, SMALL_VECTOR_SIZE>::ConstEnd() const
{
    return static_cast<Array<TYPE, SMALL_VECTOR_SIZE>::ConstIterator>(this->elements + this->count);
}

//------------------------------------------------------------------------------
/**
    Find element in array, return iterator, or 0 if element not
    found.

    @param  elm     element to find
    @return         element iterator, or 0 if not found
*/
template<class TYPE, int SMALL_VECTOR_SIZE> 
typename Array<TYPE, SMALL_VECTOR_SIZE>::Iterator
Array<TYPE, SMALL_VECTOR_SIZE>::Find(const TYPE& elm, const IndexT start) const
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
template<class TYPE, int SMALL_VECTOR_SIZE> 
IndexT
Array<TYPE, SMALL_VECTOR_SIZE>::FindIndex(const TYPE& elm, const IndexT start) const
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
*/
template<class TYPE, int SMALL_VECTOR_SIZE>
template<typename ...ELEM_TYPE>
inline void 
Array<TYPE, SMALL_VECTOR_SIZE>::Append(const TYPE& first, const ELEM_TYPE&... elements)
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
template<class TYPE, int SMALL_VECTOR_SIZE>
template<typename KEYTYPE> 
inline IndexT
Array<TYPE, SMALL_VECTOR_SIZE>::FindIndex(typename std::enable_if<true, const KEYTYPE&>::type elm, const IndexT start) const
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
template<class TYPE, int SMALL_VECTOR_SIZE> 
void
Array<TYPE, SMALL_VECTOR_SIZE>::Fill(IndexT first, SizeT num, const TYPE& elm)
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
template<class TYPE, int SMALL_VECTOR_SIZE> 
Array<TYPE, SMALL_VECTOR_SIZE>
Array<TYPE, SMALL_VECTOR_SIZE>::Difference(const Array<TYPE, SMALL_VECTOR_SIZE>& rhs)
{
    Array<TYPE, SMALL_VECTOR_SIZE> diff;
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
template<class TYPE, int SMALL_VECTOR_SIZE> 
void
Array<TYPE, SMALL_VECTOR_SIZE>::Sort()
{
    std::sort(this->Begin(), this->End());
}

//------------------------------------------------------------------------------
/**
    Sorts the array using quick sort. This just calls the STL sort algorithm.
*/
template <class TYPE, int SMALL_VECTOR_SIZE>
void
Array<TYPE, SMALL_VECTOR_SIZE>::QuickSort()
{
    std::qsort(
        this->Begin(),
        this->Size(),
        sizeof(TYPE),
        [](const void* a, const void* b)
        {
            TYPE arg1 = *static_cast<const TYPE*>(a);
            TYPE arg2 = *static_cast<const TYPE*>(b);
            return (arg1 > arg2) - (arg1 < arg2);
        }
    );
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, int SMALL_VECTOR_SIZE> 
void
Util::Array<TYPE, SMALL_VECTOR_SIZE>::SortWithFunc(bool (*func)(const TYPE& lhs, const TYPE& rhs))
{
    std::sort(this->Begin(), this->End(), func);
}

//------------------------------------------------------------------------------
/**
*/
template <class TYPE, int SMALL_VECTOR_SIZE>
void
Array<TYPE, SMALL_VECTOR_SIZE>::QuickSortWithFunc(int (*func)(const void* lhs, const void* rhs))
{
    std::qsort(
        this->Begin(),
        this->Size(),
        sizeof(TYPE),
        func
    );
}

//------------------------------------------------------------------------------
/**
    Does a binary search on the array, returns the index of the identical
    element, or InvalidIndex if not found
*/
template<class TYPE, int SMALL_VECTOR_SIZE> 
IndexT
Array<TYPE, SMALL_VECTOR_SIZE>::BinarySearchIndex(const TYPE& elm) const
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
template<class TYPE, int SMALL_VECTOR_SIZE>
template<typename KEYTYPE> inline IndexT 
Array<TYPE, SMALL_VECTOR_SIZE>::BinarySearchIndex(typename std::enable_if<true, const KEYTYPE&>::type elm) const
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
template<class TYPE, int SMALL_VECTOR_SIZE> 
void
Array<TYPE, SMALL_VECTOR_SIZE>::Resize(SizeT num)
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
template<class TYPE, int SMALL_VECTOR_SIZE>
template<typename ...ARGS>
void Array<TYPE, SMALL_VECTOR_SIZE>::Resize(SizeT num, ARGS... args)
{
    if (num < this->count)
    {
        this->DestroyRange(num, this->count);
    }
    else if (num > this->capacity)
    {
        SizeT oldCapacity = this->capacity;
        this->GrowTo(num);
        for (IndexT i = oldCapacity; i < this->capacity; i++)
            this->elements[i] = TYPE(args...);
    }

    this->count = num;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, int SMALL_VECTOR_SIZE>
inline void Array<TYPE, SMALL_VECTOR_SIZE>::Extend(SizeT num)
{
    if (num > this->capacity)
    {
        this->GrowTo(num);
        this->count = num;
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, int SMALL_VECTOR_SIZE>
void
Array<TYPE, SMALL_VECTOR_SIZE>::clear() noexcept
{
    this->Clear();
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, int SMALL_VECTOR_SIZE>
inline void 
Array<TYPE, SMALL_VECTOR_SIZE>::Fit()
{
    TYPE* newArray = ArrayAlloc<TYPE>(this->count);
    if (this->elements)
    {
        this->MoveRange(newArray, this->elements, this->count);
        if (this->elements != this->stackElements.data())
            ArrayFree(this->capacity, this->elements);
    }
    this->elements = newArray;

    this->capacity = this->count;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, int SMALL_VECTOR_SIZE> 
inline constexpr SizeT 
Array<TYPE, SMALL_VECTOR_SIZE>::TypeSize() const
{
    return sizeof(TYPE);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, int SMALL_VECTOR_SIZE> 
size_t
Array<TYPE, SMALL_VECTOR_SIZE>::size() const
{
    return this->count;
}


//------------------------------------------------------------------------------
/**
*/
template<class TYPE, int SMALL_VECTOR_SIZE> 
typename Array<TYPE, SMALL_VECTOR_SIZE>::Iterator
Array<TYPE, SMALL_VECTOR_SIZE>::begin() const
{
    return this->elements;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, int SMALL_VECTOR_SIZE> 
typename Array<TYPE, SMALL_VECTOR_SIZE>::Iterator
Array<TYPE, SMALL_VECTOR_SIZE>::end() const
{
    return this->elements + this->count;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, int SMALL_VECTOR_SIZE> 
void
Array<TYPE, SMALL_VECTOR_SIZE>::resize(size_t s)
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
template<class TYPE, int SMALL_VECTOR_SIZE> 
bool
Array<TYPE, SMALL_VECTOR_SIZE>::IsSorted() const
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
template<class TYPE, int SMALL_VECTOR_SIZE> 
IndexT
Array<TYPE, SMALL_VECTOR_SIZE>::InsertAtEndOfIdenticalRange(IndexT startIndex, const TYPE& elm)
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
template<class TYPE, int SMALL_VECTOR_SIZE> 
IndexT
Array<TYPE, SMALL_VECTOR_SIZE>::InsertSorted(const TYPE& elm)
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

template<class TYPE, int STACK_SIZE>
using StackArray = Array<TYPE, STACK_SIZE>;

} // namespace Util
//------------------------------------------------------------------------------
