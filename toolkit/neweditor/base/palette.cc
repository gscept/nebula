//------------------------------------------------------------------------------
//  @file palette.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "neweditor/base/palette.h"
#include "imgui.h"

namespace Editor
{

//------------------------------------------------------------------------------
/**
    The palette in the roles ImGui has for it, which is what makes the panels look the way they
    always did.

    ImGui has one style per context, so this is the whole editor's look at once. A host that owns a
    context sets its own style for its own UI, and this goes on top of it, because the roles here
    are the ones the editor's windows use.
*/
void
ApplyStyle()
{
    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowPadding = ImVec2(10.0f, 10.0f);
    style.FramePadding = ImVec2(8.0f, 5.0f);
    style.ItemSpacing = ImVec2(6.0f, 6.0f);
    style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);
    style.CellPadding = ImVec2(6.0f, 3.0f);
    style.ScrollbarSize = 10.0f;
    style.GrabMinSize = 10.0f;

    style.WindowRounding = 4.0f;
    style.FrameRounding = 4.0f;
    style.ChildRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 5.0f;
    style.TabRounding = 3.0f;

    // The sheet drew a border on the panels, the menus, the buttons and the input fields. ImGui
    // only draws one
    // for frames when the size is set, so the buttons would otherwise be borderless
    style.WindowBorderSize = 1.0f;
    style.ChildBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;
    style.FrameBorderSize = 1.0f;

    style.WindowTitleAlign = ImVec2(0.0f, 0.5f);
    style.ButtonTextAlign = ImVec2(0.0f, 0.5f);
    style.WindowMenuButtonPosition = ImGuiDir_Right;

    // The panel. A docked window takes ChildBg and an undocked one takes WindowBg, so both are the
    // panel color.
    // DockingEmptyBg is what is left between them, which is where the darker body color shows
    style.Colors[ImGuiCol_Text] = ColorFromHex(PaletteText);
    style.Colors[ImGuiCol_TextDisabled] = ColorFromHex(PaletteMuted);
    style.Colors[ImGuiCol_WindowBg] = ColorFromHex(PalettePanel);
    style.Colors[ImGuiCol_ChildBg] = ColorFromHex(PalettePanel);
    style.Colors[ImGuiCol_PopupBg] = ColorFromHex(PalettePanelAlt);
    style.Colors[ImGuiCol_Border] = ColorFromHex(PaletteBorder);
    style.Colors[ImGuiCol_BorderShadow] = ColorFromHex(0x000000, 0.0f);

    // An input is a recess in the panel rather than a raised button, so it keeps the body color and
    // the border
    style.Colors[ImGuiCol_FrameBg] = ColorFromHex(PaletteBg);
    style.Colors[ImGuiCol_FrameBgHovered] = ColorFromHex(PalettePanelAlt);
    style.Colors[ImGuiCol_FrameBgActive] = ColorFromHex(PalettePanelAlt);
    style.Colors[ImGuiCol_CheckboxSelectedBg] = ColorFromHex(PaletteBg);
    style.Colors[ImGuiCol_InputTextCursor] = ColorFromHex(PaletteAccent);

    // The header. The sheet had it on panel-alt for every panel, so the focused one only gets the
    // border color,
    // which is enough to tell which panel has the keyboard focus
    style.Colors[ImGuiCol_TitleBg] = ColorFromHex(PalettePanelAlt);
    style.Colors[ImGuiCol_TitleBgActive] = ColorFromHex(PaletteBorder);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ColorFromHex(PalettePanelAlt);
    style.Colors[ImGuiCol_MenuBarBg] = ColorFromHex(PalettePanelAlt);

    // The scrollbar had no track, so the grab is the border color on the panel
    style.Colors[ImGuiCol_ScrollbarBg] = ColorFromHex(PalettePanel);
    style.Colors[ImGuiCol_ScrollbarGrab] = ColorFromHex(PaletteBorder);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ColorFromHex(PaletteBorder, 0.5f);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ColorFromHex(PaletteAccent);

    style.Colors[ImGuiCol_CheckMark] = ColorFromHex(PaletteAccent);
    style.Colors[ImGuiCol_SliderGrab] = ColorFromHex(PaletteAccent);
    style.Colors[ImGuiCol_SliderGrabActive] = ColorFromHex(PaletteAccent);

    // A button is panel-alt with the border, and the hover is the border color
    style.Colors[ImGuiCol_Button] = ColorFromHex(PalettePanelAlt);
    style.Colors[ImGuiCol_ButtonHovered] = ColorFromHex(PaletteBorder);
    style.Colors[ImGuiCol_ButtonActive] = ColorFromHex(PaletteAccentSoft);

    // A row is selected with accent-soft and hovered with the border color
    style.Colors[ImGuiCol_Header] = ColorFromHex(PaletteAccentSoft);
    style.Colors[ImGuiCol_HeaderHovered] = ColorFromHex(PaletteBorder);
    style.Colors[ImGuiCol_HeaderActive] = ColorFromHex(PaletteAccentSoft);

    style.Colors[ImGuiCol_Separator] = ColorFromHex(PaletteBorder);
    style.Colors[ImGuiCol_SeparatorHovered] = ColorFromHex(PaletteAccent);
    style.Colors[ImGuiCol_SeparatorActive] = ColorFromHex(PaletteAccent);
    style.Colors[ImGuiCol_ResizeGrip] = ColorFromHex(PaletteBorder);
    style.Colors[ImGuiCol_ResizeGripHovered] = ColorFromHex(PaletteBorder, 0.5);
    style.Colors[ImGuiCol_ResizeGripActive] = ColorFromHex(PaletteAccent);

    // The tabs were text on the panel with the active one in the accent. A tab in ImGui has a
    // background, so it
    // keeps the panel color and only the selected one takes the header color, with the accent as
    // its marker
    style.Colors[ImGuiCol_Tab] = ColorFromHex(PalettePanel);
    style.Colors[ImGuiCol_TabHovered] = ColorFromHex(PaletteBorder);
    style.Colors[ImGuiCol_TabSelected] = ColorFromHex(PalettePanelAlt);
    style.Colors[ImGuiCol_TabSelectedOverline] = ColorFromHex(PaletteAccent);
    style.Colors[ImGuiCol_TabDimmed] = ColorFromHex(PalettePanel);
    style.Colors[ImGuiCol_TabDimmedSelected] = ColorFromHex(PalettePanelAlt);
    style.Colors[ImGuiCol_TabDimmedSelectedOverline] = ColorFromHex(PaletteAccent);

    style.Colors[ImGuiCol_DockingPreview] = ColorFromHex(PaletteAccent, 0.3f);
    style.Colors[ImGuiCol_DockingEmptyBg] = ColorFromHex(PaletteBg);

    // The outline table is drawn without borders, but the profiler windows are
    style.Colors[ImGuiCol_TableHeaderBg] = ColorFromHex(PalettePanelAlt);
    style.Colors[ImGuiCol_TableBorderStrong] = ColorFromHex(PaletteBorder);
    style.Colors[ImGuiCol_TableBorderLight] = ColorFromHex(PaletteBorder, 0.5f);
    style.Colors[ImGuiCol_TableRowBg] = ColorFromHex(PalettePanel);
    style.Colors[ImGuiCol_TableRowBgAlt] = ColorFromHex(PalettePanel);

    style.Colors[ImGuiCol_TextLink] = ColorFromHex(PaletteAccent);
    style.Colors[ImGuiCol_TextSelectedBg] = ColorFromHex(PaletteAccentSoft);
    style.Colors[ImGuiCol_TreeLines] = ColorFromHex(PaletteBorder);
    style.Colors[ImGuiCol_UnsavedMarker] = ColorFromHex(PaletteWarning);

    // A drop target was accent-soft with the border in the accent
    style.Colors[ImGuiCol_DragDropTarget] = ColorFromHex(PaletteAccent);
    style.Colors[ImGuiCol_DragDropTargetBg] = ColorFromHex(PaletteAccentSoft);

    style.Colors[ImGuiCol_NavCursor] = ColorFromHex(PaletteAccent);
    style.Colors[ImGuiCol_NavWindowingHighlight] = ColorFromHex(PaletteAccent);
    style.Colors[ImGuiCol_NavWindowingDimBg] = ColorFromHex(0x000000, 0.7f);
    style.Colors[ImGuiCol_ModalWindowDimBg] = ColorFromHex(0x000000, 0.35f);
}

} // namespace Editor
//------------------------------------------------------------------------------
