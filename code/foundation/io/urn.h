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
#include "util/stringatom.h"
#include "util/dictionary.h"
#include "io/uri.h"

//------------------------------------------------------------------------------
namespace IO
{
class URN
{
public:
    /// default constructor
    URN();
    /// init constructor
    explicit URN(const Util::String& s);
    /// init constructor
    explicit URN(const char* s);
    /// Construct from components
    explicit URN(const char* nid, const Util::String& nss);
    /// Construct from components
    explicit URN(const char* nid, const char* nss);
    /// copy constructor
    URN(const URN& rhs);
    /// assignmnent operator
    void operator=(const URN& rhs);
    /// equality operator
    bool operator==(const URN& rhs) const;
    /// inequality operator
    bool operator!=(const URN& rhs) const;

    /// append to URN operator with const char
    IO::URN operator/(const char* path);
    /// append to URN operator with string
    IO::URN operator/(const Util::String& path);
    
    /// set complete URI string
    void Set(const Util::String& s);
    /// return as concatenated string
    const Util::String& AsString();

    /// return true if the URN is empty
    bool IsEmpty() const;
    /// return true if the URN is not empty
    bool IsValid() const;
    /// Returns true if the URN is a folder
    bool IsFolder() const;
    /// clear the URN
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
    /// build string from components
    void Build();
private:
    /// split string into components
    bool Split(const Util::String& s);

    bool isFolder;
    bool isEmpty;
    bool isDirty;
    Util::String nid;
    Util::String nss;
    Util::String query;
    Util::String fragment;
    Util::String string;

    
};

//------------------------------------------------------------------------------
/**
*/
inline
URN::URN() :
    isEmpty(true),
    isFolder(false)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
URN::URN(const Util::String& s) :
    isEmpty(true),
    isFolder(false)
{
    bool validUrn = this->Split(s);
}

//------------------------------------------------------------------------------
/**
*/
inline
URN::URN(const char* s) :
    isEmpty(true),
    isFolder(false)
{
    bool validUrn = this->Split(s);
    n_assert2(validUrn, s);
}

//------------------------------------------------------------------------------
/**
*/
inline 
URN::URN(const char* nid, const Util::String& nss)
{
    this->isFolder = strlen(nid) == 0;
    this->isEmpty = this->isFolder && nss.Length() == 0;
    this->nid = nid;
    this->nss = nss;
    this->Build();
}

//------------------------------------------------------------------------------
/**
*/
inline 
URN::URN(const char* nid, const char* nss)
{
    this->isFolder = strlen(nid) == 0;
    this->isEmpty = this->isFolder && strlen(nss) == 0;
    this->nid = nid;
    this->nss = nss;
    this->Build();
}

//------------------------------------------------------------------------------
/**
*/
inline
URN::URN(const URN& rhs) :
    isEmpty(rhs.isEmpty),
    isFolder(rhs.isFolder),
    nid(rhs.nid),
    nss(rhs.nss),
    query(rhs.query),
    fragment(rhs.fragment),
    string(rhs.string)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline void
URN::operator=(const URN& rhs)
{
    this->isEmpty = rhs.isEmpty;
    this->isFolder = rhs.isFolder;
    this->nid = rhs.nid;
    this->nss = rhs.nss;
    this->query = rhs.query;
    this->fragment = rhs.fragment;
    this->string = rhs.string;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
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
inline bool
URN::operator!=(const URN& rhs) const
{
    return !(*this == rhs);
}

//------------------------------------------------------------------------------
/**
*/
inline IO::URN 
URN::operator/(const char* path)
{
    this->nss += Util::Format("/%s", path);
}

//------------------------------------------------------------------------------
/**
*/
inline IO::URN 
URN::operator/(const Util::String& path)
{
    this->nss += Util::Format("/%s", path.AsCharPtr());
}

//------------------------------------------------------------------------------
/**
*/
inline bool
URN::IsEmpty() const
{
    return this->isEmpty;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
URN::IsValid() const
{
    return !this->isEmpty;
}

//------------------------------------------------------------------------------
/**
*/
inline bool 
URN::IsFolder() const
{
    return this->isFolder;
}

//------------------------------------------------------------------------------
/**
*/
inline void
URN::Clear()
{
    this->isEmpty = true;
    this->nid.Clear();
    this->nss.Clear();
    this->query.Clear();
    this->fragment.Clear();
    this->string.Clear();
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
URN::AsString()
{
    if (this->isDirty)
        this->Build();
    this->isDirty = false;
    return this->string;
}

//------------------------------------------------------------------------------
/**
*/
inline void
URN::Set(const Util::String& s)
{
    this->Split(s);
}

//------------------------------------------------------------------------------
/**
*/
inline void
URN::SetNamespace(const Util::String& s)
{
    this->isEmpty = false;
    this->isDirty = true;
    this->nid = s;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
URN::GetNamespace() const
{
    return this->nid;
}

//------------------------------------------------------------------------------
/**
*/
inline void
URN::SetSpecific(const Util::String& s)
{
    this->isEmpty = false;
    this->isDirty = true;
    this->nss = s;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
URN::GetSpecific() const
{
    return this->nss;
}

//------------------------------------------------------------------------------
/**
*/
inline void
URN::SetQuery(const Util::String& s)
{
    this->isEmpty = false;
    this->isDirty = true;
    this->query = s;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
URN::GetQuery() const
{
    return this->query;
}

//------------------------------------------------------------------------------
/**
*/
inline void
URN::SetFragment(const Util::String& s)
{
    this->isEmpty = false;
    this->isDirty = true;
    this->fragment = s;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
URN::GetFragment() const
{
    return this->fragment;
}

} // namespace IO

//------------------------------------------------------------------------------
/**
*/
IO::URN operator ""_urn(const char* c, std::size_t s);

//------------------------------------------------------------------------------

