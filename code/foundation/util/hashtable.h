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
	(C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "util/fixedarray.h"
#include "util/array.h"
#include "util/keyvaluepair.h"
#include "math/scalar.h"
#include <type_traits>

//------------------------------------------------------------------------------
namespace Util
{
template<class KEYTYPE, class VALUETYPE> class HashTable
{
public:
	/// default constructor
	HashTable();
	/// constructor with capacity
	HashTable(SizeT capacity);
	/// copy constructor
	HashTable(const HashTable<KEYTYPE, VALUETYPE>& rhs);
	/// assignment operator
	void operator=(const HashTable<KEYTYPE, VALUETYPE>& rhs);
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
	/// add a key/value pair object to the hash table
	void Add(const KeyValuePair<KEYTYPE, VALUETYPE>& kvp);
	/// add a key and associated value
	void Add(const KEYTYPE& key, const VALUETYPE& value);
	/// adds element only if it doesn't exist, and return reference to it
	VALUETYPE& AddUnique(const KEYTYPE& key);
	/// merge two dictionaries
	void Merge(const HashTable<KEYTYPE, VALUETYPE>& rhs);
	/// erase an entry
	void Erase(const KEYTYPE& key);
	/// return true if key exists in the array
	bool Contains(const KEYTYPE& key) const;
	/// find index in bucket
	IndexT FindIndex(const KEYTYPE& key) const;
	/// get value from key and bucket
	VALUETYPE& ValueAtIndex(const KEYTYPE& key, IndexT i) const;
	/// return array of all key/value pairs in the table (slow)
	Array<KeyValuePair<KEYTYPE, VALUETYPE> > Content() const;
	/// get all keys as an Util::Array (slow)
	Array<KEYTYPE> KeysAsArray() const;
	/// get all keys as an Util::Array (slow)
	Array<VALUETYPE> ValuesAsArray() const;
private:
	FixedArray<Array<KeyValuePair<KEYTYPE, VALUETYPE> > > hashArray;
	int size;

	/// if type is integral, just use that value directly
	template<typename HASHTYPE> const IndexT GetHashCode(const typename std::enable_if<std::is_integral<HASHTYPE>::value, HASHTYPE>::type& key) const { return IndexT(key % this->hashArray.Size()); };
	/// if not, call the function on HashCode on KEYTPYE
	template<typename HASHTYPE> const IndexT GetHashCode(const HASHTYPE& key) const { return key.HashCode() % this->hashArray.Size();  };
};

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
Array<KEYTYPE>
Util::HashTable<KEYTYPE, VALUETYPE>::KeysAsArray() const
{
	Util::Array<KEYTYPE> keys;
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
template<class KEYTYPE, class VALUETYPE>
Array<VALUETYPE>
Util::HashTable<KEYTYPE, VALUETYPE>::ValuesAsArray() const
{
	Util::Array<VALUETYPE> vals;
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
template<class KEYTYPE, class VALUETYPE>
HashTable<KEYTYPE, VALUETYPE>::HashTable() :
	hashArray(128),
	size(0)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
HashTable<KEYTYPE, VALUETYPE>::HashTable(SizeT capacity) :
	hashArray(capacity),
	size(0)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
HashTable<KEYTYPE, VALUETYPE>::HashTable(const HashTable<KEYTYPE, VALUETYPE>& rhs) :
	hashArray(rhs.hashArray),
	size(rhs.size)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
void
HashTable<KEYTYPE, VALUETYPE>::operator=(const HashTable<KEYTYPE, VALUETYPE>& rhs)
{
	if (this != &rhs)
	{
		this->hashArray = rhs.hashArray;
		this->size = rhs.size;
	}
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
VALUETYPE&
HashTable<KEYTYPE, VALUETYPE>::operator[](const KEYTYPE& key) const
{
	// get hash code from key, trim to capacity
	IndexT hashIndex = GetHashCode<KEYTYPE>(key);
	const Array<KeyValuePair<KEYTYPE, VALUETYPE> >& hashElements = this->hashArray[hashIndex];
	int numHashElements = hashElements.Size();
	#if NEBULA3_BOUNDSCHECKS    
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
		#if NEBULA3_BOUNDSCHECKS
		n_assert(InvalidIndex != hashElementIndex);
		#endif
		return hashElements[hashElementIndex].Value();
	}
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
SizeT
HashTable<KEYTYPE, VALUETYPE>::Size() const
{
	return this->size;
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
SizeT
HashTable<KEYTYPE, VALUETYPE>::Capacity() const
{
	return this->hashArray.Size();
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
void
HashTable<KEYTYPE, VALUETYPE>::Clear()
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
template<class KEYTYPE, class VALUETYPE>
bool
HashTable<KEYTYPE, VALUETYPE>::IsEmpty() const
{
	return (0 == this->size);
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
void
HashTable<KEYTYPE, VALUETYPE>::Add(const KeyValuePair<KEYTYPE, VALUETYPE>& kvp)
{
	#if NEBULA3_BOUNDSCHECKS
	n_assert(!this->Contains(kvp.Key()));
	#endif
	IndexT hashIndex = GetHashCode(kvp.Key());
	n_assert(hashIndex >= 0);
	this->hashArray[hashIndex].InsertSorted(kvp);
	this->size++;
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
void
HashTable<KEYTYPE, VALUETYPE>::Add(const KEYTYPE& key, const VALUETYPE& value)
{
	KeyValuePair<KEYTYPE, VALUETYPE> kvp(key, value);
	this->Add(kvp);
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
inline VALUETYPE&
HashTable<KEYTYPE, VALUETYPE>::AddUnique(const KEYTYPE& key)
{
	// get hash code from key, trim to capacity
	IndexT hashIndex = GetHashCode(key);
	Array<KeyValuePair<KEYTYPE, VALUETYPE> >& hashElements = this->hashArray[hashIndex];
	if (hashElements.Size() == 0)
	{
		this->Add(key, VALUETYPE());
	}
	else if (hashElements.Size() > 1)
	{
		IndexT hashElementIndex = hashElements.BinarySearchIndex(key);
		if (hashElementIndex == InvalidIndex)
		{
			this->Add(key, VALUETYPE());
			hashElementIndex = hashElements.BinarySearchIndex(key);
		}
		return hashElements[hashElementIndex].Value();
	}
	return hashElements[0].Value();

}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
void
HashTable<KEYTYPE, VALUETYPE>::Merge(const HashTable<KEYTYPE, VALUETYPE>& rhs)
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
template<class KEYTYPE, class VALUETYPE>
void
HashTable<KEYTYPE, VALUETYPE>::Erase(const KEYTYPE& key)
{
	#if NEBULA3_BOUNDSCHECKS
	n_assert(this->size > 0);
	#endif
	IndexT hashIndex = GetHashCode(key);
	Array<KeyValuePair<KEYTYPE, VALUETYPE> >& hashElements = this->hashArray[hashIndex];
	IndexT hashElementIndex = hashElements.BinarySearchIndex(key);
	#if NEBULA3_BOUNDSCHECKS
	n_assert(InvalidIndex != hashElementIndex); // key doesn't exist
	#endif
	hashElements.EraseIndex(hashElementIndex);
	this->size--;
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
bool
HashTable<KEYTYPE, VALUETYPE>::Contains(const KEYTYPE& key) const
{
	if (this->size > 0)
	{
		IndexT hashIndex = GetHashCode(key);
		Array<KeyValuePair<KEYTYPE, VALUETYPE> >& hashElements = this->hashArray[hashIndex];
		IndexT hashElementIndex = hashElements.BinarySearchIndex(key);
		return (InvalidIndex != hashElementIndex);
	}
	else
	{
		return false;
	}
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
inline IndexT
HashTable<KEYTYPE, VALUETYPE>::FindIndex(const KEYTYPE& key) const
{
	IndexT hashIndex = GetHashCode(key);
	Array<KeyValuePair<KEYTYPE, VALUETYPE> >& hashElements = this->hashArray[hashIndex];
	IndexT hashElementIndex = hashElements.BinarySearchIndex(key);
	return hashElementIndex;
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
inline VALUETYPE&
HashTable<KEYTYPE, VALUETYPE>::ValueAtIndex(const KEYTYPE& key, IndexT i) const
{
	IndexT hashIndex = GetHashCode(key);
	Array<KeyValuePair<KEYTYPE, VALUETYPE> >& hashElements = this->hashArray[hashIndex];
	return hashElements[i].Value();
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
Array<KeyValuePair<KEYTYPE, VALUETYPE> >
HashTable<KEYTYPE, VALUETYPE>::Content() const
{
	Array<KeyValuePair<KEYTYPE, VALUETYPE> > result;
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

} // namespace Util
//------------------------------------------------------------------------------
