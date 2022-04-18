#pragma once
//------------------------------------------------------------------------------
/**
    @file cmds.h

    contains the api for all undo/redo commands

    (C) 2021 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "editor.h"

namespace Edit
{

Editor::Entity CreateEntity(Util::StringAtom templateName);
void DeleteEntity(Editor::Entity entity);
void SetSelection(Util::Array<Editor::Entity> const& selection);
void SetProperty(Editor::Entity entity, Game::PropertyId pid, void* value);
void AddProperty(Editor::Entity entity, Game::PropertyId pid);
void RemoveProperty(Editor::Entity entity, Game::PropertyId pid);
void SetEntityName(Editor::Entity entity, Util::String const& name);

} // namespace Edit
