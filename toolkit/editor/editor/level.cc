//------------------------------------------------------------------------------
//  @file level.cc
//  @copyright (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "level.h"
#include "editor/editor.h"
#include "flat/game/level.h"

namespace Editor
{
__ImplementClass(Editor::Level, 'ELVL', Core::RefCounted);
//------------------------------------------------------------------------------
/**
*/
Level::Level()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
Level::~Level()
{
    this->Clear();
}

//------------------------------------------------------------------------------
/**
*/
void
Level::Clear()
{
}

//------------------------------------------------------------------------------
/**
*/
bool
Level::LoadLevel(const Util::String& name)
{
    return false;
}

//------------------------------------------------------------------------------
/**
*/
bool
Level::SaveLevelAs(const Util::String& name)
{
    return false;
}

//------------------------------------------------------------------------------
/**
*/
bool
Level::Export(const Util::String& name)
{
    Editor::state.editorWorld->ExportLevel(name);
    return true;
}

} // namespace Editor
