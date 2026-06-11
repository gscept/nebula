#pragma once
//------------------------------------------------------------------------------
/**
    @class Posix::PosixLibrary

    Load and handle a shared object using the POSIX dynamic loader API.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
#include "util/string.h"
#include "io/uri.h"
#include "io/stream.h"
#include "system/base/librarybase.h"

//------------------------------------------------------------------------------
namespace Posix
{
class PosixLibrary : public Base::Library
{
public:
    /// constructor
    PosixLibrary();

    /// load shared library
    bool Load() override;
    /// close shared library
    void Close() override;
    /// get exported function address (dlsym)
    void* GetExport(Util::String const& name) const override;
    /// gets the state of the library
    bool IsLoaded() override;

private:
    void* handle;
};

} // namespace Posix
//------------------------------------------------------------------------------
