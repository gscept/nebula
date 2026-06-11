//------------------------------------------------------------------------------
//  posixlibrary.cc
//  (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "posixlibrary.h"
#include <dlfcn.h>
#include <cstdio>

namespace Posix
{
using namespace Util;

//------------------------------------------------------------------------------
/**
*/
PosixLibrary::PosixLibrary() :
    handle(nullptr)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
bool
PosixLibrary::Load()
{
    n_assert(this->path.IsValid());
    Util::String p = this->path.GetHostAndLocalPath();

    // Clear previous dlerror state before trying to load.
    dlerror();
    this->handle = dlopen(p.AsCharPtr(), RTLD_NOW | RTLD_LOCAL);
    this->isLoaded = this->handle != nullptr;
    if (!this->isLoaded)
    {
        const char* err = dlerror();
        std::fprintf(stderr, "PosixLibrary::Load(): failed to load '%s' (%s)\n", p.AsCharPtr(), err != nullptr ? err : "unknown error");
    }
    return this->isLoaded;
}

//------------------------------------------------------------------------------
/**
*/
void
PosixLibrary::Close()
{
    if (!this->isLoaded || this->handle == nullptr)
        return;

    int result = dlclose(this->handle);
    if (result != 0)
    {
        const char* err = dlerror();
        std::fprintf(stderr, "PosixLibrary::Close(): failed to close '%s' (%s)\n", this->path.GetHostAndLocalPath().AsCharPtr(), err != nullptr ? err : "unknown error");
    }

    this->handle = nullptr;
    this->isLoaded = false;
}

//------------------------------------------------------------------------------
/**
*/
void*
PosixLibrary::GetExport(Util::String const& name) const
{
    if (this->handle == nullptr)
        return nullptr;

    dlerror();
    void* sym = dlsym(this->handle, name.AsCharPtr());
    const char* err = dlerror();
    if (err != nullptr)
    {
        std::fprintf(stderr, "PosixLibrary::GetExport(): symbol '%s' not found in '%s' (%s)\n", name.AsCharPtr(), this->path.GetHostAndLocalPath().AsCharPtr(), err);
        return nullptr;
    }
    return sym;
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
