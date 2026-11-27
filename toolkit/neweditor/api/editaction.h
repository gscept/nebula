#pragma once
//------------------------------------------------------------------------------
/**
    @file editaction.h
    The shared edit actions. The keys and menus that run them are global, so Ctrl+C or F2 means the
    same thing whichever panel is in front, and what it acts on is the panel last worked in. A panel
    that edits something of its own takes the action for it. Otherwise it goes to the scene.
    @see
    Editor::EditorPanel::OnEditAction
    Editor::Shell::OfferEditAction
    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------

namespace Editor
{

enum class EditAction
{
    Save,
    Copy,
    Paste,
    Duplicate,
    Delete,
    Rename,
};

} // namespace Editor
//------------------------------------------------------------------------------
