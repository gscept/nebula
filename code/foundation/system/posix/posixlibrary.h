#pragma once
//------------------------------------------------------------------------------
/**
    @class Posix::PosixLibrary

    Load and handle an external shared library on POSIX platforms.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "system/base/librarybase.h"

namespace Posix
{
class PosixLibrary : public Base::Library
{
public:
    /// constructor
    PosixLibrary();

    /// load library
    bool Load() override;
    /// close library
    void Close() override;
    /// get exported function address
    void* GetExport(Util::String const& name) const override;
    /// get library state
    bool IsLoaded() override;

private:
    void* libraryHandle;
};
} // namespace Posix
