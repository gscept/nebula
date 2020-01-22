#pragma once
//------------------------------------------------------------------------------
/**
    @class Util::ArrayStack

    Nebula's small vector optimized array. Identical to Array in every way except
	that it keeps a stack allocated small buffer (much like String) and can allow
	for arrays to be tightly packed in memory.

	Presented at CppCon 2016:
	https://www.youtube.com/watch?v=vElZc6zSIXM

    (C) 2018-2020 Individual contributors, see AUTHORS file
*/
#include "core/types.h"

//------------------------------------------------------------------------------
namespace Util
{
template<class TYPE, int STACK_SIZE> class ArrayStack
{
public:
    /// define iterator
    typedef TYPE* Iterator;

    /// constructor with default parameters
	ArrayStack();
    /// constuctor with initial size and grow size
	ArrayStack(SizeT initialCapacity, SizeT initialGrow);
    /// constructor with initial size, grow size and initial values
	ArrayStack(SizeT initialSize, SizeT initialGrow, const TYPE& initialValue);
    /// copy constructor
	ArrayStack(const ArrayStack<TYPE, STACK_SIZE>& rhs);
	/// move constructor
	ArrayStack(ArrayStack<TYPE, STACK_SIZE>&& rhs);
	/// constructor from initializer list
	ArrayStack(std::initializer_list<TYPE> list);
    /// destructor
    ~ArrayStack();

    /// assignment operator
    void operator=(const ArrayStack<TYPE, STACK_SIZE>& rhs);
	/// move operator
	void operator=(ArrayStack<TYPE, STACK_SIZE>&& rhs);
    /// [] operator
    TYPE& operator[](IndexT index) const;
    /// equality operator
    bool operator==(const ArrayStack<TYPE, STACK_SIZE>& rhs) const;
    /// inequality operator
    bool operator!=(const ArrayStack<TYPE, STACK_SIZE>& rhs) const;
    /// convert to "anything"
    template<typename T> T As() const;

    /// append element to end of array
    void Append(const TYPE& elm);
    /// append the contents of an array to this array
    void AppendArray(const ArrayStack<TYPE, STACK_SIZE>& rhs);
    /// increase capacity to fit N more elements into the array
    void Reserve(SizeT num);
    /// get number of elements in array
    SizeT Size() const;
    /// get overall allocated size of array in number of elements
    SizeT Capacity() const;
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
	/// erase last element
	void EraseBack();
	/// erase first element
	void EraseFront();
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
    Iterator Find(const TYPE& elm) const;
    /// find identical element in array, return index, InvalidIndex if not found
    IndexT FindIndex(const TYPE& elm) const;
    /// find identical element using a specific key type
    template <typename KEYTYPE> IndexT FindIndex(typename std::enable_if<true, const KEYTYPE&>::type elm) const;
    /// fill array range with element
    void Fill(IndexT first, SizeT num, const TYPE& elm);
    /// clear contents and preallocate with new attributes
    void Realloc(SizeT capacity, SizeT grow);
    /// returns new array with elements which are not in rhs (slow!)
    ArrayStack<TYPE, STACK_SIZE> Difference(const ArrayStack<TYPE, STACK_SIZE>& rhs);
    /// sort the array
    void Sort();
	/// sort with custom function
	void SortWithFunc(bool (*func)(const TYPE& lhs, const TYPE& rhs));
    /// do a binary search, requires a sorted array
    IndexT BinarySearchIndex(const TYPE& elm) const;
	/// do binary search with explicit typed element
	template <typename KEYTYPE> IndexT BinarySearchIndex(typename std::enable_if<true, const KEYTYPE&>::type elm) const;

	/// returns true if the stack is used
	const bool IsStackUsed() const;

	/// for range-based iteration
	Iterator begin() const;
	Iterator end() const;
private:
    /// destroy an element (call destructor without freeing memory)
    void Destroy(TYPE* elm);
    /// copy content
    void Copy(const ArrayStack<TYPE, STACK_SIZE>& src);
    /// delete content
    void Delete();
    /// grow array
    void Grow();
    /// grow array to target size
    void GrowTo(SizeT newCapacity);
    /// move elements, grows array if needed
    void Move(IndexT fromIndex, IndexT toIndex);

    static const SizeT MinGrowSize = 16;
    static const SizeT MaxGrowSize = 65536; // FIXME: big grow size needed for mesh tools
    SizeT grow;                             // grow by this number of elements if array exhausted
    SizeT capacity;                         // number of elements allocated
    SizeT count;                             // number of elements in array
	TYPE smallVector[STACK_SIZE];
    TYPE* elements;                         // pointer to element array
};

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, int STACK_SIZE>
ArrayStack<TYPE, STACK_SIZE>::ArrayStack() :
    grow(8),
    capacity(STACK_SIZE),
    count(0),
    elements(smallVector)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, int STACK_SIZE>
ArrayStack<TYPE, STACK_SIZE>::ArrayStack(SizeT _capacity, SizeT _grow) :
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
		if (this->capacity > STACK_SIZE)
			this->elements = n_new_array(TYPE, this->capacity);
        else
        {
            this->capacity = STACK_SIZE;
			this->elements = smallVector;
        }
    }
    else
    {
        this->capacity = STACK_SIZE;
        this->elements = smallVector;
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, int STACK_SIZE>
ArrayStack<TYPE, STACK_SIZE>::ArrayStack(SizeT initialSize, SizeT _grow, const TYPE& initialValue) :
    grow(_grow),
    capacity(initialSize),
    count(initialSize)
{
    if (0 == this->grow)
    {
        this->grow = 16;
    }
    if (this->capacity > 0)
    {
		if (this->capacity > STACK_SIZE)
			this->elements = n_new_array(TYPE, this->capacity);
        else
        {
            this->capacity = STACK_SIZE;
			this->elements = smallVector;
        }

        IndexT i;
        for (i = 0; i < this->capacity; i++)
        {
            this->elements[i] = initialValue;
        }
    }
    else
    {
        this->capacity = STACK_SIZE;
        this->elements = smallVector;
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, int STACK_SIZE>
ArrayStack<TYPE, STACK_SIZE>::ArrayStack(std::initializer_list<TYPE> list) :
	grow(16),
	capacity((SizeT)list.size()),
	count((SizeT)list.size())
{
	if (this->capacity > 0)
	{
		if (this->capacity > STACK_SIZE)
			this->elements = n_new_array(TYPE, this->capacity);
        else
        {
            this->capacity = STACK_SIZE;
			this->elements = smallVector;
        }

		IndexT i;
		for (i = 0; i < this->count; i++)
		{
			this->elements[i] = list.begin()[i];
		}
	}
	else
	{
        this->capacity = STACK_SIZE;
		this->elements = smallVector;
	}
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, int STACK_SIZE>
ArrayStack<TYPE, STACK_SIZE>::ArrayStack(const ArrayStack<TYPE, STACK_SIZE>& rhs) :
	grow(0),
	capacity(0),
	count(0),
	elements(smallVector)
{
	this->Copy(rhs);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, int STACK_SIZE>
inline ArrayStack<TYPE, STACK_SIZE>::ArrayStack(ArrayStack<TYPE, STACK_SIZE>&& rhs)
{
	this->capacity = rhs.capacity;
	this->count = rhs.count;
	this->grow = rhs.grow;
	if (this->capacity > STACK_SIZE)
	{
		this->elements = rhs.elements;
	}
	else
	{
		for (IndexT i = 0; i < rhs.count; ++i)
		{
			this->smallVector[i] = rhs.smallVector[i];
		}

		this->elements = this->smallVector;
	}
	rhs.count = 0;
	rhs.capacity = 0;
	rhs.elements = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, int STACK_SIZE> void
ArrayStack<TYPE, STACK_SIZE>::Copy(const ArrayStack<TYPE, STACK_SIZE>& src)
{
    #if NEBULA_BOUNDSCHECKS
	if (this->elements != this->smallVector)
		n_assert(0 == this->elements);
    #endif

    this->grow = src.grow;
    this->capacity = src.capacity;
    this->count = src.count;
    if (this->capacity > 0)
    {
		if (this->capacity > STACK_SIZE)
			this->elements = n_new_array(TYPE, this->capacity);
		else
			this->elements = smallVector;

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
template<class TYPE, int STACK_SIZE> void
ArrayStack<TYPE, STACK_SIZE>::Delete()
{
    this->grow = 0;
    this->capacity = 0;
    this->count = 0;
    if (this->elements)
    {
		if (this->elements != this->smallVector)
			n_delete_array(this->elements);

        this->elements = this->smallVector;
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, int STACK_SIZE> void
ArrayStack<TYPE, STACK_SIZE>::Destroy(TYPE* elm)
{
    elm->~TYPE();
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, int STACK_SIZE>
ArrayStack<TYPE, STACK_SIZE>::~ArrayStack()
{
    this->Delete();
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, int STACK_SIZE> void
ArrayStack<TYPE, STACK_SIZE>::Realloc(SizeT _capacity, SizeT _grow)
{
    this->Delete();
    this->grow = _grow;
    this->capacity = _capacity;
    this->count = 0;
    if (this->capacity > 0)
    {
		if (this->capacity > STACK_SIZE)
			this->elements = n_new_array(TYPE, this->capacity);
		else
			this->elements = smallVector;
    }
    else
    {
        this->elements = smallVector;
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, int STACK_SIZE> void 
ArrayStack<TYPE, STACK_SIZE>::operator=(const ArrayStack<TYPE, STACK_SIZE>& rhs)
{
    if (this != &rhs)
    {
        if ((this->capacity > 0) && (rhs.count <= this->capacity))
        {
            // source array fits into our capacity, copy in place
            n_assert(0 != this->elements);
            IndexT i;
            for (i = 0; i < rhs.count; i++)
            {
                this->elements[i] = rhs.elements[i];
            }

            // properly destroy remaining original elements
            for (; i < this->count; i++)
            {
                this->Destroy(&(this->elements[i]));
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
template<class TYPE, int STACK_SIZE>
inline void ArrayStack<TYPE, STACK_SIZE>::operator=(ArrayStack<TYPE, STACK_SIZE>&& rhs)
{
	this->capacity = rhs.capacity;
	this->count = rhs.count;
	this->grow = rhs.grow;
	if (this->capacity > STACK_SIZE)
	{
		this->elements = rhs.elements;
	}
	else
	{
		memcpy(this->smallVector, rhs.smallVector, sizeof(TYPE) * rhs.count);
	}
	rhs.count = 0;
	rhs.capacity = 0;
	rhs.elements = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, int STACK_SIZE> void
ArrayStack<TYPE, STACK_SIZE>::GrowTo(SizeT newCapacity)
{
	if (newCapacity > STACK_SIZE)
	{
		TYPE* newArray = n_new_array(TYPE, newCapacity);
		if (this->elements)
		{
			// copy over contents
			IndexT i;
			for (i = 0; i < this->count; i++)
			{
				newArray[i] = this->elements[i];
			}

			// discard old array
			if (this->elements != this->smallVector)
				n_delete_array(this->elements);
		}
		this->elements = newArray;
	}
    this->capacity = newCapacity;

}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, int STACK_SIZE> void
ArrayStack<TYPE, STACK_SIZE>::Grow()
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
	07-Dec-04	jo		bugfix: neededSize >= this->capacity => neededSize > capacity	
*/
template<class TYPE, int STACK_SIZE> void
ArrayStack<TYPE, STACK_SIZE>::Move(IndexT fromIndex, IndexT toIndex)
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
        IndexT i;
        for (i = 0; i < num; i++)
        {
            this->elements[toIndex + i] = this->elements[fromIndex + i];
        }

        // destroy remaining elements
        for (i = (fromIndex + i) - 1; i < this->count; i++)
        {
            this->Destroy(&(this->elements[i]));
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

        // destroy freed elements
        for (i = int(fromIndex); i < int(toIndex); i++)
        {
            this->Destroy(&(this->elements[i]));
        }
    }

    // adjust array size
    this->count = toIndex + num;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, int STACK_SIZE> void
ArrayStack<TYPE, STACK_SIZE>::Append(const TYPE& elm)
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
template<class TYPE, int STACK_SIZE> void
ArrayStack<TYPE, STACK_SIZE>::AppendArray(const ArrayStack<TYPE, STACK_SIZE>& rhs)
{
    IndexT i;
    SizeT num = rhs.Size();
    for (i = 0; i < num; i++)
    {
        this->Append(rhs[i]);
    }
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
template<class TYPE, int STACK_SIZE> void
ArrayStack<TYPE, STACK_SIZE>::Reserve(SizeT num)
{
    #if NEBULA_BOUNDSCHECKS
    n_assert(num > 0);
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
template<class TYPE, int STACK_SIZE> SizeT
ArrayStack<TYPE, STACK_SIZE>::Size() const
{
    return this->count;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, int STACK_SIZE> SizeT
ArrayStack<TYPE, STACK_SIZE>::Capacity() const
{
    return this->capacity;
}

//------------------------------------------------------------------------------
/**
    Access an element. This method will NOT grow the array, and instead do
    a range check, which may throw an assertion.
*/
template<class TYPE, int STACK_SIZE> TYPE&
ArrayStack<TYPE, STACK_SIZE>::operator[](IndexT index) const
{
    #if NEBULA_BOUNDSCHECKS
    n_assert(this->elements && (index < this->count));
    #endif
    return this->elements[index];
}

//------------------------------------------------------------------------------
/**
    The equality operator returns true if all elements are identical. The
    TYPE class must support the equality operator.
*/
template<class TYPE, int STACK_SIZE> bool
ArrayStack<TYPE, STACK_SIZE>::operator==(const ArrayStack<TYPE, STACK_SIZE>& rhs) const
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
template<class TYPE, int STACK_SIZE> bool
ArrayStack<TYPE, STACK_SIZE>::operator!=(const ArrayStack<TYPE, STACK_SIZE>& rhs) const
{
    return !(*this == rhs);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, int STACK_SIZE> TYPE&
ArrayStack<TYPE, STACK_SIZE>::Front() const
{
    #if NEBULA_BOUNDSCHECKS
    n_assert(this->elements && (this->count > 0));
    #endif
    return this->elements[0];
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, int STACK_SIZE> TYPE&
ArrayStack<TYPE, STACK_SIZE>::Back() const
{
    #if NEBULA_BOUNDSCHECKS
    n_assert(this->elements && (this->count > 0));
    #endif
    return this->elements[this->count - 1];
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, int STACK_SIZE> bool 
ArrayStack<TYPE, STACK_SIZE>::IsEmpty() const
{
    return (this->count == 0);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, int STACK_SIZE> void
ArrayStack<TYPE, STACK_SIZE>::EraseIndex(IndexT index)
{
    #if NEBULA_BOUNDSCHECKS
    n_assert(this->elements && (index < this->count));
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
template<class TYPE, int STACK_SIZE> void
ArrayStack<TYPE, STACK_SIZE>::EraseIndexSwap(IndexT index)
{
    #if NEBULA_BOUNDSCHECKS
    n_assert(this->elements && (index < this->count));
    #endif

    // swap with last element, and destroy last element
    IndexT lastElementIndex = this->count - 1;
    if (index < lastElementIndex)
    {
        this->elements[index] = this->elements[lastElementIndex];
    }
    this->Destroy(&(this->elements[lastElementIndex]));
    this->count--;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, int STACK_SIZE> typename ArrayStack<TYPE, STACK_SIZE>::Iterator
ArrayStack<TYPE, STACK_SIZE>::Erase(typename ArrayStack<TYPE, STACK_SIZE>::Iterator iter)
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
template<class TYPE, int STACK_SIZE> typename ArrayStack<TYPE, STACK_SIZE>::Iterator
ArrayStack<TYPE, STACK_SIZE>::EraseSwap(typename ArrayStack<TYPE, STACK_SIZE>::Iterator iter)
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
template<class TYPE, int STACK_SIZE> void
ArrayStack<TYPE, STACK_SIZE>::EraseBack()
{
	n_assert(this->count > 0);
	this->Destroy(&(this->elements[this->count - 1]));
	this->count--;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, int STACK_SIZE> void 
ArrayStack<TYPE, STACK_SIZE>::EraseFront()
{
	this->Erase(0);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, int STACK_SIZE> void
ArrayStack<TYPE, STACK_SIZE>::Insert(IndexT index, const TYPE& elm)
{
    #if NEBULA_BOUNDSCHECKS
    n_assert(index <= this->count);
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
    preallocated space. It simply sets the array size to 0.
*/
template<class TYPE, int STACK_SIZE> void
ArrayStack<TYPE, STACK_SIZE>::Clear()
{
    IndexT i;
    for (i = 0; i < this->count; i++)
    {
        this->Destroy(&(this->elements[i]));
    }
    this->count = 0;
}

//------------------------------------------------------------------------------
/**
    This is identical with Clear(), but does NOT call destructors (it just
    resets the size member. USE WITH CARE!
*/
template<class TYPE, int STACK_SIZE> void
ArrayStack<TYPE, STACK_SIZE>::Reset()
{
    this->count = 0;
}

//------------------------------------------------------------------------------
/**
	Free up memory and reset the grow
*/
template<class TYPE, int STACK_SIZE> void 
ArrayStack<TYPE, STACK_SIZE>::Free()
{
	this->Delete();
	this->grow = 8;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, int STACK_SIZE> typename ArrayStack<TYPE, STACK_SIZE>::Iterator
ArrayStack<TYPE, STACK_SIZE>::Begin() const
{
    return this->elements;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, int STACK_SIZE> typename ArrayStack<TYPE, STACK_SIZE>::Iterator
ArrayStack<TYPE, STACK_SIZE>::End() const
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
template<class TYPE, int STACK_SIZE> typename ArrayStack<TYPE, STACK_SIZE>::Iterator
ArrayStack<TYPE, STACK_SIZE>::Find(const TYPE& elm) const
{
    IndexT index;
    for (index = 0; index < this->count; index++)
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
template<class TYPE, int STACK_SIZE> IndexT
ArrayStack<TYPE, STACK_SIZE>::FindIndex(const TYPE& elm) const
{
    IndexT index;
    for (index = 0; index < this->count; index++)
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

	@param  elm     element to find
	@return         index to element, or InvalidIndex if not found
*/
template<class TYPE, int STACK_SIZE>
template<typename KEYTYPE> inline IndexT
ArrayStack<TYPE, STACK_SIZE>::FindIndex(typename std::enable_if<true, const KEYTYPE&>::type elm) const
{
	IndexT index;
	for (index = 0; index < this->count; index++)
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
template<class TYPE, int STACK_SIZE> void
ArrayStack<TYPE, STACK_SIZE>::Fill(IndexT first, SizeT num, const TYPE& elm)
{
    if ((first + num) > this->count)
    {
        this->GrowTo(first + num);
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
template<class TYPE, int STACK_SIZE> ArrayStack<TYPE, STACK_SIZE>
ArrayStack<TYPE, STACK_SIZE>::Difference(const ArrayStack<TYPE, STACK_SIZE>& rhs)
{
    ArrayStack<TYPE, STACK_SIZE> diff;
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
template<class TYPE, int STACK_SIZE> void
ArrayStack<TYPE, STACK_SIZE>::Sort()
{
    std::sort(this->Begin(), this->End());
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, int STACK_SIZE> void
Util::ArrayStack<TYPE, STACK_SIZE>::SortWithFunc(bool (*func)(const TYPE& lhs, const TYPE& rhs))
{
	std::sort(this->Begin(), this->End(), func);
}

//------------------------------------------------------------------------------
/**
    Does a binary search on the array, returns the index of the identical
    element, or InvalidIndex if not found
*/
template<class TYPE, int STACK_SIZE> IndexT
ArrayStack<TYPE, STACK_SIZE>::BinarySearchIndex(const TYPE& elm) const
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
*/
template<class TYPE, int STACK_SIZE>
template<typename KEYTYPE> inline IndexT
ArrayStack<TYPE, STACK_SIZE>::BinarySearchIndex(typename std::enable_if<true, const KEYTYPE&>::type elm) const
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
template<class TYPE, int STACK_SIZE>
const bool ArrayStack<TYPE, STACK_SIZE>::IsStackUsed() const
{
	return this->elements == this->smallVector;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, int STACK_SIZE> typename ArrayStack<TYPE, STACK_SIZE>::Iterator
ArrayStack<TYPE, STACK_SIZE>::begin() const
{
	return this->elements;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE, int STACK_SIZE> typename ArrayStack<TYPE, STACK_SIZE>::Iterator
ArrayStack<TYPE, STACK_SIZE>::end() const
{
	return this->elements + this->count;
}

//------------------------------------------------------------------------------
/**
    This tests, whether the array is sorted. This is a slow operation
    O(n).
*/
template<class TYPE, int STACK_SIZE> bool
ArrayStack<TYPE, STACK_SIZE>::IsSorted() const
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
template<class TYPE, int STACK_SIZE> IndexT
ArrayStack<TYPE, STACK_SIZE>::InsertAtEndOfIdenticalRange(IndexT startIndex, const TYPE& elm)
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
template<class TYPE, int STACK_SIZE> IndexT
ArrayStack<TYPE, STACK_SIZE>::InsertSorted(const TYPE& elm)
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
