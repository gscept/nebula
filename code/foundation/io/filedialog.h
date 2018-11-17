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
    bool OpenFile(const IO::URI & start, const Util::String& filters, Util::String& path);
    /// Folder selection dialog
    bool OpenFolder(const IO::URI & start, Util::String& path);
    /// Save file dialog
    bool SaveFile(const IO::URI & start, const Util::String& filters, Util::String& path);
}
}