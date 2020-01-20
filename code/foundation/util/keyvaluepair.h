#pragma once
//------------------------------------------------------------------------------
/**
    @class Util::KeyValuePair
    
    Key/Value pair objects are used by most assiociative container classes,
    like Dictionary or HashTable. 
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file	
*/
#include "core/types.h"

//------------------------------------------------------------------------------
namespace Util
{
template<class KEYTYPE, class VALUETYPE> class KeyValuePair
{
public:
    /// default constructor
    KeyValuePair();
    /// constructor with key and value
    KeyValuePair(const KEYTYPE& k, const VALUETYPE& v);
    /// constructor with key and undefined value
    explicit KeyValuePair(const KEYTYPE& k);
    /// copy constructor
    KeyValuePair(const KeyValuePair<KEYTYPE, VALUETYPE>& rhs);
    /// move constructor
    KeyValuePair(KeyValuePair<KEYTYPE, VALUETYPE>&& rhs);
    /// assignment operator
    void operator=(const KeyValuePair<KEYTYPE, VALUETYPE>& rhs);
    /// move assignment operator
    void operator=(KeyValuePair<KEYTYPE, VALUETYPE>&& rhs);
    /// equality operator
    bool operator==(const KeyValuePair<KEYTYPE, VALUETYPE>& rhs) const;
    /// inequality operator
    bool operator!=(const KeyValuePair<KEYTYPE, VALUETYPE>& rhs) const;
    /// greater operator
    bool operator>(const KeyValuePair<KEYTYPE, VALUETYPE>& rhs) const;
    /// greater-or-equal operator
    bool operator>=(const KeyValuePair<KEYTYPE, VALUETYPE>& rhs) const;
    /// lesser operator
    bool operator<(const KeyValuePair<KEYTYPE, VALUETYPE>& rhs) const;
    /// lesser-or-equal operator
    bool operator<=(const KeyValuePair<KEYTYPE, VALUETYPE>& rhs) const;
	/// equality operator
	bool operator==(const KEYTYPE& rhs) const;
	/// inequality operator
	bool operator!=(const KEYTYPE& rhs) const;
	/// greater operator
	bool operator>(const KEYTYPE& rhs) const;
	/// greater-or-equal operator
	bool operator>=(const KEYTYPE& rhs) const;
	/// lesser operator
	bool operator<(const KEYTYPE& rhs) const;
	/// lesser-or-equal operator
	bool operator<=(const KEYTYPE& rhs) const;
    /// read/write access to value
    VALUETYPE& Value();
    /// read access to key
    const KEYTYPE& Key() const;
    /// read access to key
    const VALUETYPE& Value() const;

protected:
    KEYTYPE keyData;
    VALUETYPE valueData;
};

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
KeyValuePair<KEYTYPE, VALUETYPE>::KeyValuePair()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
KeyValuePair<KEYTYPE, VALUETYPE>::KeyValuePair(const KEYTYPE& k, const VALUETYPE& v) :
    keyData(k),
    valueData(v)
{
    // empty
}

//------------------------------------------------------------------------------
/**
    This strange constructor is useful for search-by-key if
    the key-value-pairs are stored in a Util::Array.
*/
template<class KEYTYPE, class VALUETYPE>
KeyValuePair<KEYTYPE, VALUETYPE>::KeyValuePair(const KEYTYPE& k) :
    keyData(k)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
KeyValuePair<KEYTYPE, VALUETYPE>::KeyValuePair(const KeyValuePair<KEYTYPE, VALUETYPE>& rhs) :
    keyData(rhs.keyData),
    valueData(rhs.valueData)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
KeyValuePair<KEYTYPE, VALUETYPE>::KeyValuePair(KeyValuePair<KEYTYPE, VALUETYPE>&& rhs) :
    keyData(std::move(rhs.keyData)),
    valueData(std::move(rhs.valueData))
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
void
KeyValuePair<KEYTYPE, VALUETYPE>::operator=(const KeyValuePair<KEYTYPE, VALUETYPE>& rhs)
{
    this->keyData = rhs.keyData;
    this->valueData = rhs.valueData;
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
void
KeyValuePair<KEYTYPE, VALUETYPE>::operator=(KeyValuePair<KEYTYPE, VALUETYPE>&& rhs)
{
    this->keyData = std::move(rhs.keyData);
    this->valueData = std::move(rhs.valueData);
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
bool
KeyValuePair<KEYTYPE, VALUETYPE>::operator==(const KeyValuePair<KEYTYPE, VALUETYPE>& rhs) const
{
    return (this->keyData == rhs.keyData);
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
bool
KeyValuePair<KEYTYPE, VALUETYPE>::operator!=(const KeyValuePair<KEYTYPE, VALUETYPE>& rhs) const
{
    return (this->keyData != rhs.keyData);
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
bool
KeyValuePair<KEYTYPE, VALUETYPE>::operator>(const KeyValuePair<KEYTYPE, VALUETYPE>& rhs) const
{
    return (this->keyData > rhs.keyData);
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
bool
KeyValuePair<KEYTYPE, VALUETYPE>::operator>=(const KeyValuePair<KEYTYPE, VALUETYPE>& rhs) const
{
    return (this->keyData >= rhs.keyData);
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
bool
KeyValuePair<KEYTYPE, VALUETYPE>::operator<(const KeyValuePair<KEYTYPE, VALUETYPE>& rhs) const
{
    return (this->keyData < rhs.keyData);
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
bool
KeyValuePair<KEYTYPE, VALUETYPE>::operator<=(const KeyValuePair<KEYTYPE, VALUETYPE>& rhs) const
{
    return (this->keyData <= rhs.keyData);
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
bool
KeyValuePair<KEYTYPE, VALUETYPE>::operator==(const KEYTYPE& rhs) const
{
	return (this->keyData == rhs);
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
bool
KeyValuePair<KEYTYPE, VALUETYPE>::operator!=(const KEYTYPE& rhs) const
{
	return (this->keyData != rhs);
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
bool
KeyValuePair<KEYTYPE, VALUETYPE>::operator>(const KEYTYPE& rhs) const
{
	return (this->keyData > rhs);
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
bool
KeyValuePair<KEYTYPE, VALUETYPE>::operator>=(const KEYTYPE& rhs) const
{
	return (this->keyData >= rhs);
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
bool
KeyValuePair<KEYTYPE, VALUETYPE>::operator<(const KEYTYPE& rhs) const
{
	return (this->keyData < rhs);
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
bool
KeyValuePair<KEYTYPE, VALUETYPE>::operator<=(const KEYTYPE& rhs) const
{
	return (this->keyData <= rhs);
}


//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
VALUETYPE&
KeyValuePair<KEYTYPE, VALUETYPE>::Value()
{
    return this->valueData;
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
const KEYTYPE&
KeyValuePair<KEYTYPE, VALUETYPE>::Key() const
{
    return this->keyData;
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
const VALUETYPE&
KeyValuePair<KEYTYPE, VALUETYPE>::Value() const
{
    return this->valueData;
}

} // namespace Util
//------------------------------------------------------------------------------
    