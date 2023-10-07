//------------------------------------------------------------------------------
//  win32library.cc
//  (C) 2023 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "win32library.h"

namespace Win32
{
using namespace Util;
using namespace IO;

//------------------------------------------------------------------------------
/**
*/
Win32Library::Win32Library() :
    hInstance(NULL)
{ 
    // empty
}

//------------------------------------------------------------------------------
/**
*/
bool
Win32Library::Load()
{
    n_assert(this->path.IsValid());
    Util::String p = this->path.GetHostAndLocalPath();
    this->hInstance = LoadLibrary(p.AsCharPtr());
    this->isLoaded = this->hInstance != NULL;
    return this->isLoaded;
}

//------------------------------------------------------------------------------
/**
*/
void
Win32Library::Close()
{
    n_assert(this->isLoaded);
    bool freeResult = FreeLibrary(this->hInstance);

    // Not sure if the following assert is necessary.
    // Might be that the free only returns true if the library is ACTUALLY 
    // unloaded from memory. It might return false (documentation is unclear)
    // if some other application is still referencing the dll
    n_assert(freeResult); 
}

//------------------------------------------------------------------------------
/**
*/
void*
Win32Library::GetExport(Util::String const& name) const
{
    FARPROC adr = GetProcAddress(this->hInstance, name.AsCharPtr());
    return (void*)adr;
}

//------------------------------------------------------------------------------
/**
*/
bool
Win32Library::IsLoaded()
{
    return this->isLoaded;
}

} // namespace Win32
