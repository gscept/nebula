//------------------------------------------------------------------------------
//  @file shell.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "neweditor/api/shell.h"

namespace Editor
{

__ImplementAbstractClass(Editor::Shell, 'SHEL', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
Shell::~Shell()
{
    // empty
}

} // namespace Editor
//------------------------------------------------------------------------------
