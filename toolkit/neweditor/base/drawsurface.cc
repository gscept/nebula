//------------------------------------------------------------------------------
//  @file drawsurface.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "neweditor/base/drawsurface.h"
#include "neweditor/base/iconregistry.h"
#include "imgui.h"

namespace Editor
{

// The flags the inspector's rows are drawn with. They are baked in here rather than passed, because
// the
// managed side has no way to name ImGui's constants
static const ImGuiTableFlags TableFlags = ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoSavedSettings |
    ImGuiTableFlags_NoHostExtendX;

// The width the key column costs, which is what makes the values line up with the component rows
static const float KeyWidth = 92.0f;

// The widget label. It is only an id to ImGui, because the name is already in the key column, and
// the id
// comes from the key that was pushed for the row
static const char* ValueLabel = "##value";

//------------------------------------------------------------------------------
/**
    The key column of a row, and the value column left open for whatever the caller draws into it.
*/
static void
DrawRowKey(const char* key, const char* description)
{
    ImGui::TableNextRow();

    ImGui::TableSetColumnIndex(0);
    ImGui::AlignTextToFramePadding();

    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    ImGui::TextUnformatted(key);
    ImGui::PopStyleColor();

    if (description != nullptr)
        ImGui::SetItemTooltip("%s", description);

    ImGui::TableSetColumnIndex(1);
}

//------------------------------------------------------------------------------
/**
    The row the panel draws for a section: the title, the override marker and the remove button on
    the right, and the separator under it. The buttons are drawn when the caller names the property
    the row belongs to, because a managed section has nothing else to be identified by. The marker
    is drawn when the caller says the property has keys this instance authored.
*/
int
DrawSection(const char* title, const char* propertyName, bool authored)
{
    ImGui::Spacing();

    // The text is aligned to the frame padding, because a button on the same row is a frame
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(title);

    int clicked = 0;

    if (propertyName != nullptr)
    {
        ImGui::SameLine();

        // The labels are the same on every property section, so the id has to come from the
        // property rather than
        // from the label. Without every button in the panel would be the same item to ImGui
        ImGui::PushID(propertyName);

        ImGuiStyle& style = ImGui::GetStyle();

        // The buttons are icons, so the width they cost is the width of the glyphs rather than the
        // width of the
        // words. The labels are what they fall back to when a name is not in the table
        const char* markerGlyph = IconGlyph("rotate-ccw");
        const char* markerTip = "Revert property to prefab";
        const char* removeGlyph = IconGlyph("trash-2");
        const char* removeTip = "Remove";

        float width = ImGui::CalcTextSize(removeGlyph != nullptr ? removeGlyph : removeTip).x + style.FramePadding.x * 2.0f;
        if (authored)
            width += ImGui::CalcTextSize(markerGlyph != nullptr ? markerGlyph : markerTip).x + style.FramePadding.x * 2.0f + style.ItemInnerSpacing.x;

        float off = ImGui::GetContentRegionAvail().x - width;
        if (off > 0.0f)
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);

        if (authored)
        {
            if (IconButton(markerGlyph, markerTip))
                clicked = 2;

            ImGui::SameLine();
        }

        if (IconButton(removeGlyph, removeTip))
            clicked = 1;

        ImGui::PopID();
    }

    ImGui::Separator();

    return clicked;
}

//------------------------------------------------------------------------------
/**
*/
bool
DrawBeginTable(const char* id)
{
    if (!ImGui::BeginTable(id, 2, TableFlags))
        return false;

    // The columns are set up here rather than by the caller, because the key column is the same on
    // every
    // row of the panel and the value column is the one that takes what is left
    ImGui::TableSetupColumn("key", ImGuiTableColumnFlags_WidthFixed, KeyWidth);
    ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthStretch);

    return true;
}

//------------------------------------------------------------------------------
/**
    EndTable is called even when BeginTable answered false, because ImGui still wants the table
    ended for a table that was not drawn
*/
void
DrawEndTable()
{
    ImGui::EndTable();
}

//------------------------------------------------------------------------------
/**
*/
void
DrawRowText(const char* key, const char* value, const char* description)
{
    DrawRowKey(key, description);
    ImGui::TextUnformatted(value);
}

//------------------------------------------------------------------------------
/**
*/
void
DrawRowTextDisabled(const char* key, const char* value, const char* description)
{
    DrawRowKey(key, description);
    ImGui::TextDisabled("%s", value);
}

//------------------------------------------------------------------------------
/**
*/
bool
DrawRowLink(const char* key, const char* value, const char* description)
{
    DrawRowKey(key, description);
    return ImGui::TextLink(value);
}

//------------------------------------------------------------------------------
/**
*/
bool
DrawRowInt(const char* key, int value, int* out, const char* description)
{
    DrawRowKey(key, description);

    ImGui::PushID(key);
    int current = value;
    const bool changed = ImGui::InputInt(ValueLabel, &current);
    ImGui::PopID();

    if (out != nullptr)
        *out = current;

    return changed;
}

//------------------------------------------------------------------------------
/**
*/
bool
DrawRowFloat(const char* key, float value, float* out, const char* description)
{
    DrawRowKey(key, description);

    ImGui::PushID(key);
    float current = value;
    const bool changed = ImGui::DragFloat(ValueLabel, &current, 0.01f);
    ImGui::PopID();

    if (out != nullptr)
        *out = current;

    return changed;
}

//------------------------------------------------------------------------------
/**
*/
bool
DrawRowBool(const char* key, bool value, bool* out, const char* description)
{
    DrawRowKey(key, description);

    ImGui::PushID(key);
    bool current = value;
    const bool changed = ImGui::Checkbox(ValueLabel, &current);
    ImGui::PopID();

    if (out != nullptr)
        *out = current;

    return changed;
}

//------------------------------------------------------------------------------
/**
    The value the user typed is written into the buffer the managed side gave, because the surface
    does not own memory the managed side can read afterwards.
*/
bool
DrawRowString(const char* key, const char* value, char* out, int outLen, const char* description)
{
    DrawRowKey(key, description);

    if (out == nullptr || outLen <= 1)
        return false;

    // Copied by hand rather than through a CRT string function, and stopped at the source's length
    int i = 0;
    while (i < outLen - 1 && value != nullptr && value[i] != 0)
    {
        out[i] = value[i];
        i++;
    }
    out[i] = 0;

    ImGui::PushID(key);
    const bool changed = ImGui::InputText(ValueLabel, out, outLen);
    ImGui::PopID();

    return changed;
}

//------------------------------------------------------------------------------
/**
*/
bool
DrawRowCombo(const char* key, const char* options, int index, int* out, const char* description)
{
    DrawRowKey(key, description);

    if (options == nullptr)
        return false;

    ImGui::PushID(key);
    int current = index;
    const bool changed = ImGui::Combo(ValueLabel, &current, options);
    ImGui::PopID();

    if (out != nullptr)
        *out = current;

    return changed;
}

//------------------------------------------------------------------------------
/**
    The components of one field as separate drags on the same row. Each one gets its own id, because
    the row only pushes one
*/
bool
DrawRowFloats(const char* key, float* values, int count, const char* description)
{
    DrawRowKey(key, description);

    if (values == nullptr || count <= 0)
        return false;

    ImGui::PushID(key);

    bool changed = false;
    for (int i = 0; i < count; i++)
    {
        if (i > 0)
            ImGui::SameLine();

        ImGui::PushID(i);
        if (ImGui::DragFloat(ValueLabel, &values[i], 0.01f))
            changed = true;
        ImGui::PopID();
    }

    ImGui::PopID();

    return changed;
}

//------------------------------------------------------------------------------
/**
    The override marker on a row, drawn after the widget the value column has. It is only drawn when
    the caller says the row is the instance's own, and the answer is true when it was pressed. What
    a press means is not the surface's decision: it reports the press and lets the caller decide.

    The key is pushed for the button, because every marker is the same glyph and ImGui would
    otherwise see them as one item.
*/
bool
DrawRowMarker(const char* key, bool authored, const char* tip)
{
    if (!authored)
        return false;

    ImGui::SameLine();
    ImGui::PushID(key);

    const bool pressed = IconButton(IconGlyph("rotate-ccw"), tip != nullptr ? tip : "Revert to prefab");

    ImGui::PopID();
    return pressed;
}

//------------------------------------------------------------------------------
/**
*/
void
DrawPushId(const char* id)
{
    ImGui::PushID(id);
}

//------------------------------------------------------------------------------
/**
*/
void
DrawPopId()
{
    ImGui::PopID();
}

//------------------------------------------------------------------------------
/**
*/
void
DrawTextDisabled(const char* text)
{
    ImGui::TextDisabled("%s", text);
}

//------------------------------------------------------------------------------
/**
*/
bool
DrawIsAnyItemActive()
{
    return ImGui::IsAnyItemActive();
}

//------------------------------------------------------------------------------
/**
    An icon button. The glyph is what the icon table has for a name, and the label is what the button
    falls back to when the name is not in the table. The label is also the tooltip, because an
    icon-only button has nothing else to say.

    The tooltip is only worth showing when the button is actually an icon. When it fell back to the
    label there is nothing to add, because the label is already on screen.
*/
bool
IconButton(const char* glyph, const char* label)
{
    const bool clicked = ImGui::Button(glyph != nullptr ? glyph : label);
    if (glyph != nullptr)
        ImGui::SetItemTooltip("%s", label);
    return clicked;
}

} // namespace Editor
//------------------------------------------------------------------------------