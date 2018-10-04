#pragma once
//------------------------------------------------------------------------------
/**
    @class Util::StringAtom
    
    A StringAtom. See StringAtomTableBase for details about the
    StringAtom system in N3.

    TODO: WARNING/STATISTICS for creation from char* or String and 
    converting back to String!

    (C) 2007 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "util/string.h"
#include <functional>
//------------------------------------------------------------------------------
namespace Util
{
class StringAtom
{
public:
    /// default constructor
    StringAtom();
    /// copy constructor
    StringAtom(const StringAtom& rhs);
    /// construct from char ptr
    StringAtom(char* ptr);
    /// construct from char ptr
    StringAtom(const char* ptr);
    /// construct from char ptr
    StringAtom(unsigned char* ptr);
    /// construct from char ptr
    StringAtom(const unsigned char* ptr);
    /// construct from string object
    StringAtom(const String& str);

    /// assignment
    void operator=(const StringAtom& rhs);
    /// assignment from char ptr
    void operator=(const char* ptr);
    /// assignment from string object
    void operator=(const String& str);

    /// equality operator
    bool operator==(const StringAtom& rhs) const;
    /// inequality operator
    bool operator!=(const StringAtom& rhs) const;
    /// greater-then operator
    bool operator>(const StringAtom& rhs) const;
    /// less-then operator
    bool operator<(const StringAtom& rhs) const;
    /// greater-or-equal operator
    bool operator>=(const StringAtom& rhs) const;
    /// less-or-equal operator
    bool operator<=(const StringAtom& rhs) const;

    /// equality with char* (SLOW!)
    bool operator==(const char* rhs) const;
    /// inequality with char* (SLOW!)
    bool operator!=(const char* rhs) const;
    /// equality with string object (SLOW!)
    bool operator==(const String& rhs) const;
    /// inequality with string object (SLOW!)
    bool operator!=(const String& rhs) const;

    /// clear content (becomes invalid)
    void Clear();
    /// return true if valid (contains a non-empty string)
    bool IsValid() const;
    /// get contained string as char ptr (fast)
    const char* Value() const;
    /// get containted string as string object (SLOW!!!)
    String AsString() const;

	/// calculate hash code for Util::HashTable (basically just the adress)
	IndexT HashCode() const;
private:
    /// setup the string atom from a string pointer
    void Setup(const char* str);

    const char* content;
};

//------------------------------------------------------------------------------
/**
*/
__forceinline
StringAtom::StringAtom() :
    content(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
StringAtom::StringAtom(const StringAtom& rhs) :
    content(rhs.content)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
StringAtom::StringAtom(char* str)
{
    if (0 != str)
    {
        this->Setup(str);
    }
    else
    {
        this->content = 0;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline
StringAtom::StringAtom(const char* str)
{
    if (0 != str)
    {
        this->Setup(str);
    }
    else
    {
        this->content = 0;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline
StringAtom::StringAtom(unsigned char* str)
{
    if (0 != str)
    {
        this->Setup((const char*)str);
    }
    else
    {
        this->content = 0;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline
StringAtom::StringAtom(const unsigned char* str)
{
    if (0 != str)
    {
        this->Setup((const char*)str);
    }
    else
    {
        this->content = 0;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline
StringAtom::StringAtom(const String& str)
{
    this->Setup(str.AsCharPtr());   
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
StringAtom::operator=(const StringAtom& rhs)
{
    this->content = rhs.content;
}

//------------------------------------------------------------------------------
/**
*/
inline void
StringAtom::operator=(const char* str)
{
    if (0 != str)
    {
        this->Setup(str);
    }
    else
    {
        this->content = 0;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline void
StringAtom::operator=(const String& str)
{
    this->Setup(str.AsCharPtr());    
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
StringAtom::operator==(const StringAtom& rhs) const
{
    return this->content == rhs.content;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
StringAtom::operator!=(const StringAtom& rhs) const
{
    return this->content != rhs.content;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
StringAtom::operator>(const StringAtom& rhs) const
{
    return this->content > rhs.content;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
StringAtom::operator<(const StringAtom& rhs) const
{
    return this->content < rhs.content;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
StringAtom::operator>=(const StringAtom& rhs) const
{
    return this->content >= rhs.content;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
StringAtom::operator<=(const StringAtom& rhs) const
{
    return this->content <= rhs.content;
}

//------------------------------------------------------------------------------
/**
    Compare with String object. Careful, slow!
*/
inline bool
StringAtom::operator==(const String& rhs) const
{
    if (0 == this->content)
    {
        return false;
    }
    else
    {
        return (rhs == this->content);
    }
}

//------------------------------------------------------------------------------
/**
    Compare with String object. Careful, slow!
*/
inline bool
StringAtom::operator!=(const String& rhs) const
{
    if (0 == this->content)
    {
        return false;
    }
    else
    {
        return (rhs != this->content);
    }
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
StringAtom::Clear()
{
    this->content = 0;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
StringAtom::IsValid() const
{
    return (0 != this->content) && (0 != this->content[0]);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline const char*
StringAtom::Value() const
{
    return this->content;
}

//------------------------------------------------------------------------------
/**
    SLOW!!!
*/
inline String
StringAtom::AsString() const
{
    return String(this->content);
}

//------------------------------------------------------------------------------
/**
*/
inline IndexT
StringAtom::HashCode() const
{
	// FIXME, test this :D
	PtrT key = PtrT(this->content);
	return (IndexT)(std::hash<unsigned long long>{}(key) & 0x7FFFFFFF);
}

} // namespace Util

//------------------------------------------------------------------------------
/**
*/
Util::StringAtom operator ""_atm(const char* c, std::size_t s);

//------------------------------------------------------------------------------
