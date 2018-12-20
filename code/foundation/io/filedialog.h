#pragma once
//------------------------------------------------------------------------------
/**
    @class IO::FileDialog

    Wrapper around NFD for showing file dialogs

    (C) 2018 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "util/string.h"
#include "io/uri.h"


//------------------------------------------------------------------------------
namespace IO
{
namespace FileDialog
{
    /// Shows open file dialog     
    bool OpenFile(Util::String const& title, const IO::URI & start, std::initializer_list<const char*>const& filters, Util::String& path);
    /// Folder selection dialog
    bool OpenFolder(Util::String const& title, const IO::URI & start, Util::String& path);
    /// Save file dialog
    bool SaveFile(Util::String const& title, const IO::URI & start, std::initializer_list<const char*>const& filters, Util::String& path);
}
}