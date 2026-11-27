#pragma once
//------------------------------------------------------------------------------
/**
    @file assetfield.h
    The field an asset reference is edited with, wherever something refers to an asset: a material's
    texture, a component's model. It shows the asset by its folder and file, and changes it the ways
    an asset is passed around in the editor:

    - a double click opens the asset in its editor
    - an asset of the type dragged from the browser can be dropped on it
    - right click copies its URN, middle click pastes one
    - the button next to it picks another in the asset picker

    The field does not change anything itself. It hands the new asset to onChange, which may be
    called while the field draws, or later, after the frame, when the picker answers. The caller
    decides how the change is made, usually as an undoable command.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "io/path.h"
#include "util/string.h"
#include <functional>

namespace Editor
{

/// draw an asset field. Type is the URN type the field takes, like 'tex', nullptr takes any asset.
/// An asset exported as the type is taken as it, a model for a field that takes a skeleton gives
/// the model's skeleton. The width is the whole field's, the name and the button, 0 fills the line
void AssetField(const char* id, const IO::Path& asset, const char* type, const std::function<void(const IO::Path&)>& onChange, float width = 0.0f);

} // namespace Editor
//------------------------------------------------------------------------------
