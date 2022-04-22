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
void SetComponent(Editor::Entity entity, Game::ComponentId component, void* value);
void AddComponent(Editor::Entity entity, Game::ComponentId component);
void RemoveComponent(Editor::Entity entity, Game::ComponentId component);
void SetEntityName(Editor::Entity entity, Util::String const& name);

} // namespace Edit
