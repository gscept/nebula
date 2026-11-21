#pragma once
//------------------------------------------------------------------------------
/**
    The colors the editor UI is drawn with, and the style that puts them in ImGui's roles.

    The look is the editor's, so it lives with the editor rather than with the module that sets the
    context up. A host that owns an ImGui context has its own look for its own UI; the editor puts
    its own on the context it draws into, which is the only thing it asks of a host besides the keys
    no text field took.

    The colors are authored as sRGB, which is what ImGui uses them as, so a panel that wants one of
    them resolves it with ColorFromHex() rather than storing an ImVec4. The names are the ones the
    sheet had.

    @see
    Editor::Shell

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "imgui.h"

namespace Editor
{

static const uint32_t PaletteBg = 0x14161a;           // the body behind the panels, and the input fields
static const uint32_t PalettePanel = 0x1b1e23;        // a panel
static const uint32_t PalettePanelAlt = 0x23272e;     // headers, the toolbar, buttons, menus
static const uint32_t PaletteBorder = 0x2f343c;       // borders, and the hover state
static const uint32_t PaletteText = 0xd7dce3;
static const uint32_t PaletteMuted = 0x868f9b;        // the secondary text on a row
static const uint32_t PaletteAccent = 0x4c8dff;
static const uint32_t PaletteAccentSoft = 0x2b3f5c;   // the selected row and the drop target
static const uint32_t PaletteWarning = 0xd9a441;
static const uint32_t PaletteError = 0xd9534f;

//------------------------------------------------------------------------------
/**
    A palette entry as ImGui wants it
*/
inline ImVec4
ColorFromHex(uint32_t hex, float alpha = 1.0f)
{
    return ImVec4(
        float((hex >> 16) & 0xFF) / 255.0f,
        float((hex >> 8) & 0xFF) / 255.0f,
        float(hex & 0xFF) / 255.0f,
        alpha);
}

//------------------------------------------------------------------------------
/**
    The palette in the roles ImGui has for it. Called once, when there is a context to put it on.
*/
void ApplyStyle();

} // namespace Editor
//------------------------------------------------------------------------------
