//------------------------------------------------------------------------------
//  backend/tbuiclipboard.cc
//  (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "tbuiclipboardinterface.h"

namespace TBUI
{

// == TBClipboard =====================================

void
TBUIClipboardInterface::Empty()
{
    clipboard.Clear();
}

bool
TBUIClipboardInterface::HasText()
{
    return !clipboard.IsEmpty();
}

bool
TBUIClipboardInterface::SetText(const char* text)
{
    return clipboard.Set(text);
}

bool
TBUIClipboardInterface::GetText(tb::TBStr& text)
{
    return text.Set(clipboard);
}

} // namespace tb
