#pragma once
//------------------------------------------------------------------------------
/**
    @class Editor::RotateTool

    Turns the selection by whatever the transform handle's drag reports.

    The handle is ImGuizmo's, and the handle pass says what turn the drag has given it, so the tool
    carries every selected entity around the handle's center by that turn.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "neweditor/viewtools/toolcontext.h"
#include "neweditor/viewtools/toolinterface.h"
#include "util/array.h"

namespace Editor
{

class RotateTool final : public ToolInterface
{
public:
    void Update(const ToolView& view) override;
    bool IsModifying() const override;
    void Abort() override;
    const char* Name() const override;

private:
    struct
    {
        bool dragging = false;
        Util::Array<TransformEntry> originals;
        Util::Array<TransformEntry> current;
    } drag;
};

} // namespace Editor
