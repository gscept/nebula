#pragma once
//------------------------------------------------------------------------------
/** 
    @class IO::URN
    
    A URN (Uniform Resource Name) is a URI (Uniform Resource Identifier) that uses the "urn" scheme.
    URNs are intended to serve as persistent, location-independent resource identifiers and are defined in RFC 8141.
    A URN is typically used to identify a resource by name rather than by location, and is often used in contexts where the resource may not be directly accessible, such as in metadata or as
    a unique identifier for an asset.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "util/string.h"
#include "util/dictionary.h"

//------------------------------------------------------------------------------
namespace IO
{
class URN
{
public:
    /// default constructor
    URN();
    /// init constructor
    URN(const Util::String& s);
    /// init constructor
    URN(const char* s);
    /// copy constructor
    URN(const URN& rhs);
    /// assignmnent operator
    void operator=(const URN& rhs);
    /// equality operator
    bool operator==(const URN& rhs) const;
    /// inequality operator
    bool operator!=(const URN& rhs) const;
    
    /// set complete URI string
    void Set(const Util::String& s);
    /// return as concatenated string
    Util::String AsString() const;

    /// return true if the URI is empty
    bool IsEmpty() const;
    /// return true if the URI is not empty
    bool IsValid() const;
    /// clear the URI
    void Clear();
    /// set Namespace component
    void SetNamespace(const Util::String& s);
    /// get Namespace component
    const Util::String& GetNamespace() const;
    /// set Specific component
    void SetSpecific(const Util::String& s);
    /// get Specific component (can be empty)
    const Util::String& GetSpecific() const;
    /// set query component
    void SetQuery(const Util::String& s);
    /// get query component (can be empty)
    const Util::String& GetQuery() const;
    /// set fragment component
    void SetFragment(const Util::String& s);
    /// get fragment component (can be empty)
    const Util::String& GetFragment() const;
private:
    /// split string into components
    bool Split(const Util::String& s);
    /// build string from components
    Util::String Build() const;

    bool isEmpty;
    Util::String nid;
    Util::String nss;
    Util::String query;
    Util::String fragment;
};

//------------------------------------------------------------------------------
/**
*/
inline
URN::URN() :
    isEmpty(true)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
URN::URN(const Util::String& s) :
    isEmpty(true)
{
    bool validUrn = this->Split(s);
    n_assert2(validUrn, s.AsCharPtr());
}

//------------------------------------------------------------------------------
/**
*/
inline
URN::URN(const char* s) :
    isEmpty(true)
{
    bool validUrn = this->Split(s);
    n_assert2(validUrn, s);
}

//------------------------------------------------------------------------------
/**
*/
inline
URN::URN(const URN& rhs) :
    isEmpty(rhs.isEmpty),
    nid(rhs.nid),
    nss(rhs.nss),
    query(rhs.query),
    fragment(rhs.fragment)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline 
void
URN::operator=(const URN& rhs)
{
    this->isEmpty = rhs.isEmpty;
    this->nid = rhs.nid;
    this->nss = rhs.nss;
    this->query = rhs.query;
    this->fragment = rhs.fragment;
}

//------------------------------------------------------------------------------
/**
*/
inline 
bool
URN::operator==(const URN& rhs) const
{
    if (this->isEmpty && rhs.isEmpty)
    {
        return true;
    }
    return ((this->nid == rhs.nid) &&
            (this->nss == rhs.nss) &&
            (this->query == rhs.query) &&
            (this->fragment == rhs.fragment));
}

//------------------------------------------------------------------------------
/**
*/
inline 
bool
URN::operator!=(const URN& rhs) const
{
    return !(*this == rhs);
}

//------------------------------------------------------------------------------
/**
*/
inline 
bool
URN::IsEmpty() const
{
    return this->isEmpty;
}

//------------------------------------------------------------------------------
/**
*/
inline 
bool
URN::IsValid() const
{
    return !this->isEmpty;
}

//------------------------------------------------------------------------------
/**
*/
inline 
void
URN::Clear()
{
    this->isEmpty = true;
    this->nid.Clear();
    this->nss.Clear();
    this->query.Clear();
    this->fragment.Clear();
}

//------------------------------------------------------------------------------
/**
*/
inline 
Util::String
URN::AsString() const
{
    return this->Build();
}

//------------------------------------------------------------------------------
/**
*/
inline
void
IO::URN::Set(const Util::String& s)
{
    this->Split(s);
}

//------------------------------------------------------------------------------
/**
*/
inline
void
IO::URN::SetNamespace(const Util::String& s)
{
    this->isEmpty = false;
    this->nid = s;
}

//------------------------------------------------------------------------------
/**
*/
inline
const Util::String&
IO::URN::GetNamespace() const
{
    return this->nid;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
IO::URN::SetSpecific(const Util::String& s)
{
    this->isEmpty = false;
    this->nss = s;
}

//------------------------------------------------------------------------------
/**
*/
inline
const Util::String&
IO::URN::GetSpecific() const
{
    return this->nss;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
IO::URN::SetQuery(const Util::String& s)
{
    this->isEmpty = false;
    this->query = s;
}

//------------------------------------------------------------------------------
/**
*/
inline
const Util::String&
IO::URN::GetQuery() const
{
    return this->query;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
IO::URN::SetFragment(const Util::String& s)
{
    this->isEmpty = false;
    this->fragment = s;
}

//------------------------------------------------------------------------------
/**
*/
inline
const Util::String&
IO::URN::GetFragment() const
{
    return this->fragment;
}

} // namespace IO
//------------------------------------------------------------------------------
/**
*/
IO::URN operator ""_urn(const char* c, std::size_t s);

//------------------------------------------------------------------------------

