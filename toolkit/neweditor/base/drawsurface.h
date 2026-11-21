#pragma once
//------------------------------------------------------------------------------
/**
    The row layout a feature draws through. A feature that draws content the editor cannot read uses
    these functions instead of building its own layout, so its section looks like the rest of the
    inspector. The panel draws its own rows through the same functions.

    A row is the key in column 0 and one widget in column 1. The widget returns true when the value
    changed and writes the new value through a pointer the caller gave. Nothing here runs in a build
    with no editor. The names are C linkage, because the managed side resolves them by name.

    @see
    Editor::PropertyRecorder

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "imgui.h"

namespace Editor
{

extern "C"
{

//------------------------------------------------------------------------------
/**
    Draw a section heading: the title, the separator, the remove button when a name is given, and
    the mark when the caller says this instance wrote the keys. Return 0 when nothing was clicked, 1
    for the remove button and 2 for the mark, because a row the managed side draws can only answer
    with a number.

    The name is the id the buttons are pushed under, so it has to identify this section. The title
    is the same word on every section.
*/
int DrawSection(const char* title, const char* name, bool authored);

// The table a section's rows go into. EndTable is called even when BeginTable answers false,
// because ImGui still wants the table ended.
bool DrawBeginTable(const char* id);
void DrawEndTable();

// One row: the key and one widget. The description is optional and shows as a tooltip on the key.
// The value is utf8. A widget returns true when the value changed, and writes the new value through
// the pointer given.
void DrawRowText(const char* key, const char* value, const char* description);
void DrawRowTextDisabled(const char* key, const char* value, const char* description);
bool DrawRowInt(const char* key, int value, int* out, const char* description);
bool DrawRowFloat(const char* key, float value, float* out, const char* description);
bool DrawRowBool(const char* key, bool value, bool* out, const char* description);
bool DrawRowString(const char* key, const char* value, char* out, int outLen, const char* description);

// The combo takes the names as one buffer. Each name ends with a zero byte, and the buffer ends
// with two.
bool DrawRowCombo(const char* key, const char* options, int index, int* out, const char* description);

// A field with several parts, drawn as separate drags on one row. values is the caller's buffer and
// stays valid for the call, and the parts that changed are written back into it.
bool DrawRowFloats(const char* key, float* values, int count, const char* description);

// A row whose value is a link. Return true when it was clicked. What a click means is the caller's.
bool DrawRowLink(const char* key, const char* value, const char* description);

// The override mark, drawn on the row after the value. It is drawn only when the caller says the
// row is the instance's own. Return true when it was pressed.
bool DrawRowMarker(const char* key, bool authored, const char* tip);

// Push and pop the id buttons are drawn under, so two buttons with the same glyph do not clash.
void DrawPushId(const char* id);
void DrawPopId();

// A line of its own, for a row with no key.
void DrawTextDisabled(const char* text);

// Return true when a widget is still working on its value. This is how a caller knows a drag is not
// done.
bool DrawIsAnyItemActive();

} // extern "C"

// An icon button. The glyph is what the icon table has for a name, and the label is what the button
// falls back to when the name is not in the table. The label is also the tooltip, because an
// icon-only button has nothing else to say. The managed side does not call this, so it is not C
// linkage.
bool IconButton(const char* glyph, const char* label);

} // namespace Editor
//------------------------------------------------------------------------------
