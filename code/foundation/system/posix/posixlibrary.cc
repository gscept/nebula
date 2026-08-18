//------------------------------------------------------------------------------
//  posixlibrary.cc
//  (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "posixlibrary.h"

#include <dlfcn.h>

namespace Posix
{
//------------------------------------------------------------------------------
/**
*/
PosixLibrary::PosixLibrary()
    : libraryHandle(nullptr)
{
}

//------------------------------------------------------------------------------
/**
*/
bool
PosixLibrary::Load()
{
    n_assert(this->path.IsValid());
    Util::String path = this->path.GetHostAndLocalPath();
    this->libraryHandle = dlopen(path.AsCharPtr(), RTLD_NOW | RTLD_LOCAL);
    this->isLoaded = this->libraryHandle != nullptr;
    return this->isLoaded;
}

//------------------------------------------------------------------------------
/**
*/
void
PosixLibrary::Close()
{
    if (this->libraryHandle != nullptr)
    {
        dlclose(this->libraryHandle);
        this->libraryHandle = nullptr;
    }
    this->isLoaded = false;
}

//------------------------------------------------------------------------------
/**
*/
void*
PosixLibrary::GetExport(Util::String const& name) const
{
    n_assert(this->libraryHandle != nullptr);
    return dlsym(this->libraryHandle, name.AsCharPtr());
}

//------------------------------------------------------------------------------
/**
*/
bool
PosixLibrary::IsLoaded()
{
    return this->isLoaded;
}
} // namespace Posix
