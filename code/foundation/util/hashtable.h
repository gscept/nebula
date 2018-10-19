#pragma once
//------------------------------------------------------------------------------
/**
	@class Util::HashTable
	
	Organizes key/value pairs by a hash code. Looks very similar
	to a Dictionary, but may provide better search times (up to O(1))
	by computing a (ideally unique) hash code on the key and using that as an
	index into an array. The flipside is that the key class must provide
	a hash code and the memory footprint may be larger then Dictionary.
	
	The default capacity is 128. Matching the capacity against the number
	of expected elements in the hash table is one key to get optimal 
	insertion and search times, the other is to provide a good (and fast) 
	hash code computation which produces as few collissions as possible 
	for the key type.

	The key class must implement the following method in order to
	work with the HashTable:    
	IndexT HashCode() const;

	The Util::String class implements this method as an example.
	Internally the hash table is implemented as a fixed array
	of sorted arrays. The fixed array is indexed by the hash code
	of the key, the sorted arrays contain all values with identical
	keys.

	(C) 2006 Radon Labs GmbH
	(C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "util/fixedarray.h"
#include "util/arraystack.h"
#include "util/keyvaluepair.h"
#include "math/scalar.h"
#include <type_traits>

//------------------------------------------------------------------------------
namespace Util
{
template<class KEYTYPE, class VALUETYPE, int STACK_SIZE = 1> class HashTable
{
public:
	/// default constructor
	HashTable();
	/// constructor with capacity
	HashTable(SizeT capacity);
	/// copy constructor
	HashTable(const HashTable<KEYTYPE, VALUETYPE, STACK_SIZE>& rhs);
    /// move constructor
    HashTable(HashTable<KEYTYPE, VALUETYPE, STACK_SIZE>&& rhs);
    /// assignment operator
	void operator=(const HashTable<KEYTYPE, VALUETYPE, STACK_SIZE>& rhs);
    /// move assignment operator
    void operator=(HashTable<KEYTYPE, VALUETYPE, STACK_SIZE>&& rhs);
	/// read/write [] operator, assertion if key not found
	VALUETYPE& operator[](const KEYTYPE& key) const;
	/// return current number of values in the hashtable
	SizeT Size() const;
	/// return fixed-size capacity of the hash table
	SizeT Capacity() const;
	/// clear the hashtable
	void Clear();
	/// return true if empty
	bool IsEmpty() const;
	/// begin bulk adding to the hashtable, which will amortize the cost of sorting the hash buckets upon end
	void BeginBulkAdd();
	/// add a key/value pair object to the hash table, returns index within hash array where item is stored
	IndexT Add(const KeyValuePair<KEYTYPE, VALUETYPE>& kvp);
	/// add a key and associated value
	IndexT Add(const KEYTYPE& key, const VALUETYPE& value);
	/// adds element only if it doesn't exist, and return reference to it
	VALUETYPE& AddUnique(const KEYTYPE& key);
	/// end bulk adding, which sorts all affected tables
	void EndBulkAdd();
	/// merge two dictionaries
	void Merge(const HashTable<KEYTYPE, VALUETYPE, STACK_SIZE>& rhs);
	/// erase an entry
	void Erase(const KEYTYPE& key);
	/// return true if key exists in the array
	bool Contains(const KEYTYPE& key) const;
	/// find index in bucket
	IndexT FindIndex(const KEYTYPE& key) const;
	/// get value from key and bucket
	VALUETYPE& ValueAtIndex(const KEYTYPE& key, IndexT i) const;
	/// return array of all key/value pairs in the table (slow)
	ArrayStack<KeyValuePair<KEYTYPE, VALUETYPE>, STACK_SIZE> Content() const;
	/// get all keys as an Util::Array (slow)
	ArrayStack<KEYTYPE, STACK_SIZE> KeysAsArray() const;
	/// get all keys as an Util::Array (slow)
	ArrayStack<VALUETYPE, STACK_SIZE> ValuesAsArray() const;

	class Iterator
	{
	public:

		/// progress to next item in the hash table
		Iterator& operator++(int);
		/// check if iterator is identical
		const bool operator==(const Iterator& rhs) const;
		/// check if iterator is identical
		const bool operator!=(const Iterator& rhs) const;

		/// the current value
		VALUETYPE* val;
		KEYTYPE const* key;
	private:
		friend class HashTable<KEYTYPE, VALUETYPE, STACK_SIZE>;
		FixedArray<ArrayStack<KeyValuePair<KEYTYPE, VALUETYPE>, STACK_SIZE> >* arr;
		IndexT hashIndex;
		IndexT bucketIndex;
	};

	/// get iterator to first element
	Iterator Begin();
	/// get iterator to last element
	Iterator End();
private:
	FixedArray<ArrayStack<KeyValuePair<KEYTYPE, VALUETYPE>, STACK_SIZE> > hashArray;
	int size;

	FixedArray<bool> bulkDirty;
	bool inBulkAdd;

	/// if type is integral, just use that value directly
	template <typename HASHKEY> const IndexT GetHashCode(const typename std::enable_if<std::is_integral<HASHKEY>::value, HASHKEY>::type& key) const { return IndexT(key % this->hashArray.Size()); };
	/// if type is pointer, convert using questionable method
	template <typename HASHKEY> const IndexT GetHashCode(const typename std::enable_if<std::is_pointer<HASHKEY>::value, HASHKEY>::type& key) const { return ((IndexT)(std::hash<unsigned long long>{}(PtrT(key)) & 0x7FFFFFFF)) % this->hashArray.Size(); }
	/// if not, call the function on HashCode on HASHKEY
	template <typename HASHKEY> const IndexT GetHashCode(const HASHKEY& key) const { return key.HashCode() % this->hashArray.Size();  };
};

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE, int STACK_SIZE>
ArrayStack<KEYTYPE, STACK_SIZE>
HashTable<KEYTYPE, VALUETYPE, STACK_SIZE>::KeysAsArray() const
{
	Util::ArrayStack<KEYTYPE, STACK_SIZE> keys;
	for (int i = 0; i < this->hashArray.Size(); i++)
	{
		for (int j = 0; j < this->hashArray[i].Size();j++)
		{ 
			keys.Append(this->hashArray[i][j].Key());
		}
	}
	return keys;
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE, int STACK_SIZE>
ArrayStack<VALUETYPE, STACK_SIZE>
HashTable<KEYTYPE, VALUETYPE, STACK_SIZE>::ValuesAsArray() const
{
	Util::ArrayStack<VALUETYPE, STACK_SIZE> vals;
	for (int i = 0; i < this->hashArray.Size(); i++)
	{
		for (int j = 0; j < this->hashArray[i].Size(); j++)
		{
			vals.Append(this->hashArray[i][j].Value());
		}
	}
	return vals;
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE, int STACK_SIZE>
typename HashTable<KEYTYPE, VALUETYPE, STACK_SIZE>::Iterator
HashTable<KEYTYPE, VALUETYPE, STACK_SIZE>::Begin()
{
	Iterator ret;
	ret.hashIndex = this->hashArray.Size();
	ret.bucketIndex = 0;
	ret.val = nullptr;
	ret.key = nullptr;
	ret.arr = &this->hashArray;
	IndexT i;
	for (i = 0; i < this->hashArray.Size(); i++)
	{
		if (this->hashArray[i].Size() != 0)
		{
			ret.hashIndex = i;
			ret.bucketIndex = 0;
			ret.val = &this->hashArray[i].Front().Value();
			ret.key = &this->hashArray[i].Front().Key();
			break;
		}
	}
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE, int STACK_SIZE>
typename HashTable<KEYTYPE, VALUETYPE, STACK_SIZE>::Iterator
HashTable<KEYTYPE, VALUETYPE, STACK_SIZE>::End()
{
	Iterator ret;
	ret.hashIndex = this->hashArray.Size();
	ret.bucketIndex = 0;
	ret.val = nullptr;
	ret.key = nullptr;
	ret.arr = &this->hashArray;
	IndexT i;
	for (i = this->hashArray.Size()-1; i >= 0; i--)
	{
		if (this->hashArray[i].Size() != 0)
		{
			ret.hashIndex = i;
			ret.bucketIndex = this->hashArray[i].Size();
			ret.val = &this->hashArray[i].Back().Value();
			ret.key = &this->hashArray[i].Back().Key();
			break;
		}
	}
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE, int STACK_SIZE>
HashTable<KEYTYPE, VALUETYPE, STACK_SIZE>::HashTable() :
	hashArray(128),
	bulkDirty(128),
	inBulkAdd(false),
	size(0)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE, int STACK_SIZE>
HashTable<KEYTYPE, VALUETYPE, STACK_SIZE>::HashTable(SizeT capacity) :
	hashArray(capacity),
	bulkDirty(capacity),
	inBulkAdd(false),
	size(0)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE, int STACK_SIZE>
HashTable<KEYTYPE, VALUETYPE, STACK_SIZE>::HashTable(const HashTable<KEYTYPE, VALUETYPE, STACK_SIZE>& rhs) :
	hashArray(rhs.hashArray),
	bulkDirty(rhs.bulkDirty),
	inBulkAdd(rhs.inBulkAdd),
	size(rhs.size)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE, int STACK_SIZE>
HashTable<KEYTYPE, VALUETYPE, STACK_SIZE>::HashTable(HashTable<KEYTYPE, VALUETYPE, STACK_SIZE>&& rhs) :
    hashArray(std::move(rhs.hashArray)),
    bulkDirty(rhs.bulkDirty),
    inBulkAdd(rhs.inBulkAdd),
    size(rhs.size)
{
#ifdef NEBULA_DEBUG
    rhs.size = 0;
#endif
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE, int STACK_SIZE>
void
HashTable<KEYTYPE, VALUETYPE, STACK_SIZE>::operator=(const HashTable<KEYTYPE, VALUETYPE, STACK_SIZE>& rhs)
{
	if (this != &rhs)
	{
		this->hashArray = rhs.hashArray;
		this->bulkDirty = rhs.bulkDirty;
		this->size = rhs.size;
	}
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE, int STACK_SIZE>
void
HashTable<KEYTYPE, VALUETYPE, STACK_SIZE>::operator=(HashTable<KEYTYPE, VALUETYPE, STACK_SIZE>&& rhs)
{
    if (this != &rhs)
    {
        this->hashArray = std::move(rhs.hashArray);
        this->bulkDirty = rhs.bulkDirty;
        this->size = rhs.size;
#ifdef NEBULA_DEBUG
        rhs.size = 0;
#endif
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE, int STACK_SIZE>
VALUETYPE&
HashTable<KEYTYPE, VALUETYPE, STACK_SIZE>::operator[](const KEYTYPE& key) const
{
	// get hash code from key, trim to capacity
	IndexT hashIndex = GetHashCode<KEYTYPE>(key);
	const ArrayStack<KeyValuePair<KEYTYPE, VALUETYPE>, STACK_SIZE>& hashElements = this->hashArray[hashIndex];
	int numHashElements = hashElements.Size();
	#if NEBULA_BOUNDSCHECKS    
	n_assert(0 != numHashElements); // element with key doesn't exist
	#endif
	if (1 == numHashElements)
	{
		// no hash collissions, just return the only existing element
		return hashElements[0].Value();
	}
	else
	{
		// here's a hash collision, find the right key
		// with a binary search
		IndexT hashElementIndex = hashElements.BinarySearchIndex(key);
		#if NEBULA_BOUNDSCHECKS
		n_assert(InvalidIndex != hashElementIndex);
		#endif
		return hashElements[hashElementIndex].Value();
	}
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE, int STACK_SIZE>
SizeT
HashTable<KEYTYPE, VALUETYPE, STACK_SIZE>::Size() const
{
	return this->size;
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE, int STACK_SIZE>
SizeT
HashTable<KEYTYPE, VALUETYPE, STACK_SIZE>::Capacity() const
{
	return this->hashArray.Size();
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE, int STACK_SIZE>
void
HashTable<KEYTYPE, VALUETYPE, STACK_SIZE>::Clear()
{
	IndexT i;
	SizeT num = this->hashArray.Size();
	for (i = 0; i < num; i++)
	{
		this->hashArray[i].Clear();
	}
	this->size = 0;
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE, int STACK_SIZE>
bool
HashTable<KEYTYPE, VALUETYPE, STACK_SIZE>::IsEmpty() const
{
	return (0 == this->size);
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE, int STACK_SIZE>
void
HashTable<KEYTYPE, VALUETYPE, STACK_SIZE>::BeginBulkAdd()
{
	n_assert(!this->inBulkAdd);
	this->inBulkAdd = true;
	this->bulkDirty.Fill(false);
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE, int STACK_SIZE>
IndexT
HashTable<KEYTYPE, VALUETYPE, STACK_SIZE>::Add(const KeyValuePair<KEYTYPE, VALUETYPE>& kvp)
{
	#if NEBULA_BOUNDSCHECKS
	n_assert(!this->Contains(kvp.Key()));
	#endif
	IndexT hashIndex = GetHashCode<KEYTYPE>(kvp.Key());
	n_assert(hashIndex >= 0);
	IndexT ret;
	if (this->inBulkAdd)
	{
		ret = this->hashArray[hashIndex].Size() - 1;
		this->hashArray[hashIndex].Append(kvp);
		this->bulkDirty[hashIndex] = true;
	}
	else
		ret = this->hashArray[hashIndex].InsertSorted(kvp);
	this->size++;
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE, int STACK_SIZE>
IndexT
HashTable<KEYTYPE, VALUETYPE, STACK_SIZE>::Add(const KEYTYPE& key, const VALUETYPE& value)
{
	KeyValuePair<KEYTYPE, VALUETYPE> kvp(key, value);
	return this->Add(kvp);
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE, int STACK_SIZE>
VALUETYPE&
HashTable<KEYTYPE, VALUETYPE, STACK_SIZE>::AddUnique(const KEYTYPE& key)
{
	// get hash code from key, trim to capacity
	IndexT hashIndex = GetHashCode<KEYTYPE>(key);
	ArrayStack<KeyValuePair<KEYTYPE, VALUETYPE>, STACK_SIZE>& hashElements = this->hashArray[hashIndex];
	if (hashElements.Size() == 0)
	{
		this->Add(key, VALUETYPE());
	}
	else if (hashElements.Size() > 1)
	{
		IndexT hashElementIndex = hashElements.BinarySearchIndex(key);
		if (hashElementIndex == InvalidIndex)
		{
			hashElementIndex = this->Add(key, VALUETYPE());
		}
		return hashElements[hashElementIndex].Value();
	}
	return hashElements[0].Value();

}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE, int STACK_SIZE>
void
HashTable<KEYTYPE, VALUETYPE, STACK_SIZE>::EndBulkAdd()
{
	n_assert(this->inBulkAdd);
	IndexT i;
	for (i = 0; i < this->bulkDirty.Size(); i++)
	{
		if (this->bulkDirty[i])
		{
			this->hashArray[i].Sort();
			this->bulkDirty[i] = false;
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE, int STACK_SIZE>
void
HashTable<KEYTYPE, VALUETYPE, STACK_SIZE>::Merge(const HashTable<KEYTYPE, VALUETYPE, STACK_SIZE>& rhs)
{
	n_assert(this->hashArray.Size() == rhs.hashArray.Size());
	IndexT i;
	for (i = 0; i < rhs.hashArray.Size(); i++)
	{
		IndexT j;
		for (j = 0; j < rhs.hashArray[i].Size(); j++)
			this->Add(rhs.hashArray[i][j]);
	}
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE, int STACK_SIZE>
void
HashTable<KEYTYPE, VALUETYPE, STACK_SIZE>::Erase(const KEYTYPE& key)
{
	#if NEBULA_BOUNDSCHECKS
	n_assert(this->size > 0);
	#endif
	IndexT hashIndex = GetHashCode<KEYTYPE>(key);
	ArrayStack<KeyValuePair<KEYTYPE, VALUETYPE>, STACK_SIZE>& hashElements = this->hashArray[hashIndex];
	IndexT hashElementIndex = hashElements.BinarySearchIndex(key);
	#if NEBULA_BOUNDSCHECKS
	n_assert(InvalidIndex != hashElementIndex); // key doesn't exist
	#endif
	hashElements.EraseIndex(hashElementIndex);
	this->size--;
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE, int STACK_SIZE>
bool
HashTable<KEYTYPE, VALUETYPE, STACK_SIZE>::Contains(const KEYTYPE& key) const
{
	if (this->size > 0)
	{
		IndexT hashIndex = GetHashCode<KEYTYPE>(key);
		ArrayStack<KeyValuePair<KEYTYPE, VALUETYPE>, STACK_SIZE>& hashElements = this->hashArray[hashIndex];
		if (hashElements.Size() == 0) return false;
		else
		{
			IndexT hashElementIndex = hashElements.BinarySearchIndex(key);
			return (InvalidIndex != hashElementIndex);
		}
	}
	else
	{
		return false;
	}
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE, int STACK_SIZE>
IndexT
HashTable<KEYTYPE, VALUETYPE, STACK_SIZE>::FindIndex(const KEYTYPE& key) const
{
	IndexT hashIndex = GetHashCode<KEYTYPE>(key);
	ArrayStack<KeyValuePair<KEYTYPE, VALUETYPE>, STACK_SIZE>& hashElements = this->hashArray[hashIndex];
	IndexT hashElementIndex = hashElements.BinarySearchIndex(key);
	return hashElementIndex;
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE, int STACK_SIZE>
VALUETYPE&
HashTable<KEYTYPE, VALUETYPE, STACK_SIZE>::ValueAtIndex(const KEYTYPE& key, IndexT i) const
{
	IndexT hashIndex = GetHashCode<KEYTYPE>(key);
	ArrayStack<KeyValuePair<KEYTYPE, VALUETYPE>, STACK_SIZE>& hashElements = this->hashArray[hashIndex];
	return hashElements[i].Value();
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE, int STACK_SIZE>
ArrayStack<KeyValuePair<KEYTYPE, VALUETYPE>, STACK_SIZE>
HashTable<KEYTYPE, VALUETYPE, STACK_SIZE>::Content() const
{
	ArrayStack<KeyValuePair<KEYTYPE, VALUETYPE>, STACK_SIZE> result;
	int i;
	int num = this->hashArray.Size();
	for (i = 0; i < num; i++)
	{
		if (this->hashArray[i].Size() > 0)
		{
			result.AppendArray(this->hashArray[i]);
		}
	}
	return result;
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE, int STACK_SIZE>
typename HashTable<KEYTYPE, VALUETYPE, STACK_SIZE>::Iterator&
HashTable<KEYTYPE, VALUETYPE, STACK_SIZE>::Iterator::operator++(int)
{
	const FixedArray<ArrayStack<KeyValuePair<KEYTYPE, VALUETYPE>, STACK_SIZE> >& arr = *this->arr;

	// always increment bucket index
	this->bucketIndex++;
	if (arr[this->hashIndex].Size() < this->bucketIndex)
	{
		this->val = &arr[this->hashIndex][this->bucketIndex].Value();
		this->key = &arr[this->hashIndex][this->bucketIndex].Key();
	}
	else
	{
		// go through remainding hash slots and update iterator if an array is empty
		IndexT i;
		for (i = this->hashIndex + 1; i < arr.Size(); i++)
		{
			if (arr[i].Size() > 0)
			{
				this->hashIndex = i;
				this->bucketIndex = 0;
				this->val = &arr[i].Front().Value();
				this->key = &arr[i].Front().Key();
				break;
			}
		}
	}
	return *this;
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE, int STACK_SIZE>
const bool
HashTable<KEYTYPE, VALUETYPE, STACK_SIZE>::Iterator::operator==(const Iterator& rhs) const
{
	return this->hashIndex == rhs.hashIndex && this->bucketIndex == rhs.bucketIndex;
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE, int STACK_SIZE>
const bool
HashTable<KEYTYPE, VALUETYPE, STACK_SIZE>::Iterator::operator!=(const Iterator& rhs) const
{
	return !(this->hashIndex == rhs.hashIndex && this->bucketIndex == rhs.bucketIndex);
}
} // namespace Util
//------------------------------------------------------------------------------
