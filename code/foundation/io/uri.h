#pragma once
#ifndef IO_URI_H
#define IO_URI_H
//------------------------------------------------------------------------------
/** 
    @class IO::URI
    
    An URI object can split a Uniform Resource Identifier string into
    its components or build a string from URI components. Please note
    that the memory footprint of an URI object is always bigger then
    a pure String object, so if memory usage is of concern, it is advised
    to keep paths as String objects around, and only use URI objects
    to encode and decode them.

    An URI is made of the following components, where most of them
    are optional:
    
    Scheme://UserInfo@Host:Port/LocalPath#Fragment?Query
  
    Example URIs:

    http://user:password@www.myserver.com:8080/index.html#main
    http://www.myserver.com/query?user=bla
    ftp://ftp.myserver.com/pub/bla.zip
    file:///c:/temp/bla.txt
    file://SambaServer/temp/blub.txt

    Note that assigns will be resolved before splitting a URI into its
    components, for instance the assign "textures" could be defined
    as:

    Assign("textures", "http://www.dataserv.com/myapp/textures/");

    So a path to a texture URI could be defined as:

    URI("textures:mytex.dds")

    Which would actually resolve into:

    http://www.dataserv.com/myapp/textures/mytex.dds
        
    Decoding into components happens in the init constructor or the 
    Set() method in the following steps:

    - resolve any assigns in the original string
    - split into Scheme, Host and Path blocks
    - resolve Host and Path blocks further

    Enconding from components into string happens in the AsString()
    method in the following steps:

    - concatenate URI string from components
    - convert part of the string back into an existing assign

    @copyright
    (C) 2006 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "util/string.h"
#include "util/dictionary.h"

//------------------------------------------------------------------------------
namespace IO
{
class URI
{
public:
    /// default constructor
    URI();
    /// init constructor
    URI(const Util::String& s);
    /// init constructor
    URI(const char* s);
    /// copy constructor
    URI(const URI& rhs);
    /// assignmnent operator
    void operator=(const URI& rhs);
    /// equality operator
    bool operator==(const URI& rhs) const;
    /// inequality operator
    bool operator!=(const URI& rhs) const;
    
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
    /// set Scheme component (ftp, http, etc...)
    void SetScheme(const Util::String& s);
    /// get Scheme component (default is file)
    const Util::String& Scheme() const;
    /// set UserInfo component
    void SetUserInfo(const Util::String& s);
    /// get UserInfo component (can be empty)
    const Util::String& UserInfo() const;
    /// set Host component
    void SetHost(const Util::String& s);
    /// get Host component (can be empty)
    const Util::String& Host() const;
    /// set Port component
    void SetPort(const Util::String& s);
    /// get Port component (can be empty)
    const Util::String& Port() const;
    /// set LocalPath component
    void SetLocalPath(const Util::String& s);
    /// get LocalPath component (can be empty)
    const Util::String& LocalPath() const;
    /// append an element to the local path component
    void AppendLocalPath(const Util::String& pathComponent);
    /// set Fragment component
    void SetFragment(const Util::String& s);
    /// get Fragment component (can be empty)
    const Util::String& Fragment() const;
    /// set Query component
    void SetQuery(const Util::String& s);
    /// get Query component (can be empty)
    const Util::String& Query() const;
    /// parse query parameters into a dictionary
    Util::Dictionary<Util::String,Util::String> ParseQuery() const;
    /// get the "tail" (path, query and fragment)
    Util::String GetTail() const;
    /// get the host and path without scheme
    Util::String GetHostAndLocalPath() const;

private:
    /// split string into components
    bool Split(const Util::String& s);
    /// build string from components
    Util::String Build() const;

    bool isEmpty;
    Util::String scheme;
    Util::String userInfo;
    Util::String host;
    Util::String port;
    Util::String localPath;
    Util::String fragment;
    Util::String query;
};

//------------------------------------------------------------------------------
/**
*/
inline
URI::URI() :
    isEmpty(true)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
URI::URI(const Util::String& s) :
    isEmpty(true)
{
    bool validUri = this->Split(s);
    n_assert2(validUri, s.AsCharPtr());
}

//------------------------------------------------------------------------------
/**
*/
inline
URI::URI(const char* s) :
    isEmpty(true)
{
    bool validUri = this->Split(s);
    n_assert2(validUri, s);
}

//------------------------------------------------------------------------------
/**
*/
inline
URI::URI(const URI& rhs) :
    isEmpty(rhs.isEmpty),
    scheme(rhs.scheme),
    userInfo(rhs.userInfo),
    host(rhs.host),
    port(rhs.port),
    localPath(rhs.localPath),
    fragment(rhs.fragment),
    query(rhs.query)
{
    // empty
}    

//------------------------------------------------------------------------------
/**
*/
inline
void
URI::operator=(const URI& rhs)
{
    this->isEmpty = rhs.isEmpty;
    this->scheme = rhs.scheme;
    this->userInfo = rhs.userInfo;
    this->host = rhs.host;
    this->port = rhs.port;
    this->localPath = rhs.localPath;
    this->fragment = rhs.fragment;
    this->query = rhs.query;
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
URI::operator==(const URI& rhs) const
{
    if (this->isEmpty && rhs.isEmpty)
    {
        return true;
    }
    else
    {
        return ((this->scheme == rhs.scheme) &&
                (this->userInfo == rhs.userInfo) &&
                (this->host == rhs.host) &&
                (this->port == rhs.port) &&
                (this->localPath == rhs.localPath) &&
                (this->fragment == rhs.fragment) &&
                (this->query == rhs.query));
    }
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
URI::operator!=(const URI& rhs) const
{
    return !(*this == rhs);
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
URI::IsEmpty() const
{
    return this->isEmpty;
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
URI::IsValid() const
{
    return !(this->isEmpty);
}

//------------------------------------------------------------------------------
/**
*/
inline
void
URI::Clear() 
{
    this->isEmpty = true;
    this->scheme.Clear();
    this->userInfo.Clear();
    this->host.Clear();
    this->port.Clear();
    this->localPath.Clear();
    this->fragment.Clear();
    this->query.Clear();
}

//------------------------------------------------------------------------------
/**
*/
inline
void
URI::Set(const Util::String& s)
{
    this->Split(s);
}

//------------------------------------------------------------------------------
/**
*/
inline
Util::String
URI::AsString() const
{
    return this->Build();
}

//------------------------------------------------------------------------------
/**
*/
inline
void
URI::SetScheme(const Util::String& s)
{
    this->isEmpty = false;
    this->scheme = s;
}

//------------------------------------------------------------------------------
/**
*/
inline
const Util::String&
URI::Scheme() const
{
    return this->scheme;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
URI::SetUserInfo(const Util::String& s)
{
    this->isEmpty = false;
    this->userInfo = s;
}

//------------------------------------------------------------------------------
/**
*/
inline
const Util::String&
URI::UserInfo() const
{
    return this->userInfo;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
URI::SetHost(const Util::String& s)
{
    this->isEmpty = false;
    this->host = s;
}

//------------------------------------------------------------------------------
/**
*/
inline
const Util::String&
URI::Host() const
{
    return this->host;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
URI::SetPort(const Util::String& s)
{
    this->isEmpty = false;
    this->port = s;
}

//------------------------------------------------------------------------------
/**
*/
inline
const Util::String&
URI::Port() const
{
    return this->port;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
URI::SetLocalPath(const Util::String& s)
{
    this->isEmpty = false;
    this->localPath = s;
}

//------------------------------------------------------------------------------
/**
*/
inline
const Util::String&
URI::LocalPath() const
{
    return this->localPath;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
URI::SetFragment(const Util::String& s)
{
    this->isEmpty = false;
    this->fragment = s;
}

//------------------------------------------------------------------------------
/**
*/
inline
const Util::String&
URI::Fragment() const
{
    return this->fragment;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
URI::SetQuery(const Util::String& s)
{
    this->isEmpty = false;
    this->query = s;
}

//------------------------------------------------------------------------------
/**
*/
inline
const Util::String&
URI::Query() const
{
    return this->query;
}

} // namespace IO

//------------------------------------------------------------------------------
/**
*/
IO::URI operator ""_uri(const char* c, std::size_t s);

//------------------------------------------------------------------------------
#endif
