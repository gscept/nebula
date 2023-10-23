#pragma once
//------------------------------------------------------------------------------
/**
    @class Base::Library
    
    Base class for loading and handling dynamic libraries.
        
    @copyright
    (C) 2023 Individual contributors, see AUTHORS file
*/
#include "util/string.h"
#include "io/uri.h"
#include "io/stream.h"

//------------------------------------------------------------------------------
namespace Base
{
class Library
{
public:
    /// constructor
    Library();

    /// set the executable path
    void SetPath(const IO::URI& uri);

    /// load library
    virtual bool Load() = 0;
    /// close the library
    virtual void Close() = 0;
    /// get address to exported function (ex. GetProcAddress)
    virtual void* GetExport(Util::String const& name) const = 0;
    /// gets the state of the library
    virtual bool IsLoaded() = 0;

protected:
    bool isLoaded;
    IO::URI path;
};

//------------------------------------------------------------------------------
/**
*/
inline Library::Library()
    : isLoaded(false)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline void
Library::SetPath(const IO::URI& uri)
{
    this->path = uri;
}

} // namespace Base
//------------------------------------------------------------------------------
