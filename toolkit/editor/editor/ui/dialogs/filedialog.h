#pragma once
//------------------------------------------------------------------------------
/**
    FileDialog

    (C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "util/string.h"

namespace Presentation
{
namespace Dialogs
{

enum FileResult
{
    NOINPUT = -1,
    CANCEL = 0,
    OKAY = 1,
    ERR = 2,
};

/// Opens a file dialog using imgui
FileResult OpenFileDialog(const Util::String& title, Util::String& outpath, const char* pattern, bool& open);

/// @todo   SaveFileDialog

} // namespace Dialogs
} // namespace Presentation