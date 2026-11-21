//------------------------------------------------------------------------------
//  @file editcommand.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "neweditor/model/editcommand.h"

namespace Editor
{

__ImplementAbstractClass(Editor::EditCommand, 'EDCM', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
EditCommand::~EditCommand()
{
    // empty
}

} // namespace Editor
