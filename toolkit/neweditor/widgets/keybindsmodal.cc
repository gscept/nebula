//------------------------------------------------------------------------------
//  @file keybindsmodal.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "neweditor/widgets/keybindsmodal.h"
#include "neweditor/api/shell.h"
#include "neweditor/console/editorcommands.h"
#include "neweditor/input/editorkeybinds.h"
#include "neweditor/base/drawsurface.h"
#include "neweditor/base/iconregistry.h"
#include "neweditor/base/palette.h"
#include "imgui.h"

namespace Editor
{

__ImplementClass(Editor::KeybindsModal, 'KBMO', Core::RefCounted);

// The popup is named by this, so the open and the begin have to say the same thing
static const char* KeybindsPopup = "keybinds";

// The size the popup takes on appearing. The rows are the part that scrolls, so the height is what
// makes the
// whole registry worth listing
static const float ModalWidth = 640.0f;
static const float ModalHeight = 420.0f;

// The widths the action and the combo cost, which is what makes the command column line up between
// rows. The
// two button columns take the width of the glyph they draw
static const float ActionColumnWidth = 170.0f;
static const float ComboColumnWidth = 140.0f;

//------------------------------------------------------------------------------
/**
*/
KeybindsModal::KeybindsModal() :
    core(nullptr),
    visible(false),
    openRequested(false),
    closeRequested(false)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
KeybindsModal::~KeybindsModal()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    The modal is built with the core, which is what draws it every frame. It is never visible until
    asked.

    The command and the keybind are registered here, which is what makes Ctrl+K the same route as
    typing 'keybinds' in the console or pressing the button on the toolbar.
*/
bool
KeybindsModal::Setup(Shell* core)
{
    this->core = core;

    core->GetCommands().Register("keybinds", "open the keybinds",
        Util::Delegate<void(const Util::Array<Util::String>&)>::FromMethod<KeybindsModal, &KeybindsModal::HandleShow>(this));
    core->GetKeybinds().Register("keybinds.open", Input::Key::K, Input::Modifier::ControlModifier, "keybinds");

    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
KeybindsModal::Discard()
{
    if (this->core != nullptr)
        this->core->GetKeybinds().ClearCapture();

    this->core = nullptr;
    this->visible = false;
    this->openRequested = false;
    this->closeRequested = false;
}

//------------------------------------------------------------------------------
/**
*/
void
KeybindsModal::Show()
{
    this->visible = true;
    this->openRequested = true;
    this->closeRequested = false;
}

//------------------------------------------------------------------------------
/**
*/
void
KeybindsModal::HandleShow(const Util::Array<Util::String>& args)
{
    (void)args;
    this->Show();
}

//------------------------------------------------------------------------------
/**
    true when another action has the same combo. Two equal binds are settled by the order they are
    listed in, so the row says there is a conflict rather than which one wins.
*/
bool
KeybindsModal::Conflicts(const EditorKeyBind& keybind)
{
    if (keybind.key == Input::Key::InvalidKey)
        return false;

    const Util::Array<EditorKeyBind>& binds = this->core->GetKeybinds().GetKeybinds();
    for (IndexT i = 0; i < binds.Size(); i++)
    {
        if (binds[i].action == keybind.action)
            continue;

        if (binds[i].key == keybind.key && binds[i].mods == keybind.mods)
            return true;
    }

    return false;
}

//------------------------------------------------------------------------------
/**
    The row. The combo is the accent color while the row is the one listening, and the warning color
    when another action already has it, because those are the two states a row cannot show by its
    text alone.
*/
void
KeybindsModal::DrawRow(const EditorKeyBind& keybind)
{
    EditorKeybinds& keybinds = this->core->GetKeybinds();

    ImGui::TableNextRow();

    // Every row draws the same two glyphs, so the glyph alone is the same id on every row and a
    // button would
    // share its hover and active state with the rest of the table. The action is what the row is id
    // by, because
    // the registry is ordered by the action name, so a rebind leaves the row where it is
    ImGui::PushID(keybind.action.AsCharPtr());

    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted(keybind.action.AsCharPtr());

    ImGui::TableSetColumnIndex(1);

    const bool listening = keybinds.GetCapture() == keybind.action;
    const bool conflict = this->Conflicts(keybind);

    if (listening)
        ImGui::PushStyleColor(ImGuiCol_Text, ColorFromHex(PaletteAccent));
    else if (conflict)
        ImGui::PushStyleColor(ImGuiCol_Text, ColorFromHex(PaletteWarning));

    const Util::String combo = EditorKeybinds::ComboString(keybind.key, keybind.mods);
    ImGui::TextUnformatted(combo.IsEmpty() ? "(none)" : combo.AsCharPtr());

    if (listening || conflict)
        ImGui::PopStyleColor();

    ImGui::TableSetColumnIndex(2);
    ImGui::PushStyleColor(ImGuiCol_Text, ColorFromHex(PaletteMuted));
    ImGui::TextUnformatted(keybind.command.AsCharPtr());
    ImGui::PopStyleColor();

    ImGui::TableSetColumnIndex(3);
    if (IconButton(IconGlyph("pencil-line"), "Press a key to rebind this action"))
        keybinds.SetCapture(keybind.action);

    ImGui::TableSetColumnIndex(4);
    if (IconButton(IconGlyph("refresh-cw"), "Reset action to its default keybinding"))
    {
        keybinds.Reset(keybind.action);
        keybinds.Save();
    }

    ImGui::PopID();
}

//------------------------------------------------------------------------------
/**
    The popup centers on the viewport and takes its size on appearing, so a modal the user resized
    keeps that shape until the layout is reset. The file line is at the top, the hint at the bottom,
    and the rows are the scroll region in between.
*/
void
KeybindsModal::Draw()
{
    if (!this->visible)
        return;

    if (this->core == nullptr)
        return;

    EditorKeybinds& keybinds = this->core->GetKeybinds();

    if (this->openRequested)
    {
        ImGui::OpenPopup(KeybindsPopup);
        this->openRequested = false;
    }

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + viewport->WorkSize.x * 0.5f,
        viewport->WorkPos.y + viewport->WorkSize.y * 0.5f), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(ModalWidth, ModalHeight), ImGuiCond_Appearing);

    bool open = true;
    if (ImGui::BeginPopupModal(KeybindsPopup, &open))
    {
        // Escape closes. ImGui only closes a popup on Escape when keyboard navigation is enabled,
        // which this
        // editor does not enable, so the modal closes it itself. The route is global, so that the
        // key reaches the
        // popup whether or not its window is the focused one
        if (ImGui::Shortcut(ImGuiKey_Escape, ImGuiInputFlags_RouteGlobal))
        {
            keybinds.ClearCapture();
            this->closeRequested = true;
        }

        // The file the bindings were read from. A bind from a row writes on its own, so the button
        // is for a change
        // made somewhere else, and for a file that is not the one the registry reads at startup
        ImGui::TextDisabled("Read from %s", keybinds.GetPath().AsCharPtr());
        ImGui::SameLine();

        // The rows draw the same glyphs as these, so each is id by what it does rather than by the
        // glyph
        ImGui::PushID("save");
        if (IconButton(IconGlyph("save"), "Write the keybind file"))
            keybinds.Save();
        ImGui::PopID();

        ImGui::SameLine();
        ImGui::PushID("reset_all");
        if (IconButton(IconGlyph("refresh-cw"), "Reset all actions to their default keybindings"))
        {
            keybinds.Reset(Util::String());
            keybinds.Save();
        }
        ImGui::PopID();

        ImGui::Separator();

        const ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH |
            ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_SizingFixedFit;

        if (ImGui::BeginTable("keybinds", 5, flags))
        {
            ImGui::TableSetupColumn("action", ImGuiTableColumnFlags_WidthFixed, ActionColumnWidth);
            ImGui::TableSetupColumn("combo", ImGuiTableColumnFlags_WidthFixed, ComboColumnWidth);
            ImGui::TableSetupColumn("command", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("bind", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("reset", ImGuiTableColumnFlags_WidthFixed);

            ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted("Action");
            ImGui::TableSetColumnIndex(1);
            ImGui::TextUnformatted("Combo");
            ImGui::TableSetColumnIndex(2);
            ImGui::TextUnformatted("Command");
            ImGui::TableSetColumnIndex(3);
            ImGui::TextUnformatted("Bind");
            ImGui::TableSetColumnIndex(4);
            ImGui::TextUnformatted("Reset");

            const Util::Array<EditorKeyBind>& binds = keybinds.GetKeybinds();
            for (IndexT i = 0; i < binds.Size(); i++)
                this->DrawRow(binds[i]);

            if (binds.IsEmpty())
                ImGui::TextDisabled("Nothing registered");

            ImGui::EndTable();
        }

        // The hint says what a row that is listening means, because a listening row looks like a
        // selected row.
        // It also says which modifiers are held, because a modifier on its own is not a bind and
        // the row does not
        // change until the key that goes with it is pressed
        ImGui::PushStyleColor(ImGuiCol_Text, ColorFromHex(keybinds.IsCapturing() ? PaletteAccent : PaletteMuted));
        if (keybinds.IsCapturing())
        {
            Util::String hint = Util::String("Listening for ") + keybinds.GetCapture();

            const Util::String held = Input::Modifier::ToString(keybinds.GetCaptureMods());
            if (!held.IsEmpty())
            {
                hint.Append(" with ");
                hint.Append(held);
            }

            hint.Append(". Press a key to bind it, Escape to stop");
            ImGui::TextUnformatted(hint.AsCharPtr());
        }
        else
            ImGui::TextUnformatted("Click the pencil on a row and press a key to rebind it. Escape closes");
        ImGui::PopStyleColor();

        if (this->closeRequested)
        {
            ImGui::CloseCurrentPopup();
            this->closeRequested = false;
            this->visible = false;
            keybinds.ClearCapture();
        }

        ImGui::EndPopup();
    }

    // ImGui itself closing the popup, Escape through the nav, lands here too
    if (!open)
    {
        this->visible = false;
        keybinds.ClearCapture();
    }
}

} // namespace Editor
