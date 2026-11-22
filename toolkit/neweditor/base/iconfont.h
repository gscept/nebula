#pragma once
//------------------------------------------------------------------------------
/**
    The icon face the editor adds to the ImGui font atlas.

    The editor draws the glyphs, so the editor loads the face the table was built from. It is merged
    into the font ImGui draws with, so a glyph draws inline with a label without a PushFont.

    @see
    Editor::IconGlyph

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"

namespace Editor
{

/// Add the icon face to the font ImGui draws with. It needs an ImGui context, so this runs at
/// activation rather than at attach
void LoadIconFont();

} // namespace Editor
