#pragma once
//------------------------------------------------------------------------------
/**
    @class Win32::Win32Library
    
    Load and handle an external DLL.
    
    @copyright
    (C) 2023 Individual contributors, see AUTHORS file
*/
#include "util/string.h"
#include "io/uri.h"
#include "io/stream.h"
#include "system/base/librarybase.h"

//------------------------------------------------------------------------------
namespace Win32
{
class Win32Library : public Base::Library
{
public:
    
    /// constructor
    Win32Library();
   
    /// load library
    bool Load() override;
    /// close the library
    void Close() override;
    /// get exported function address (GetProcAddress).
    void* GetExport(Util::String const& name) const override;
    /// gets the state of the library.
    bool IsLoaded() override;

private:
    HINSTANCE hInstance;
};

} // namespace Win32
//------------------------------------------------------------------------------
