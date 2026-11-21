#pragma once
//------------------------------------------------------------------------------
/**
    @class Editor::ScaleTool

    Resizes the selection by whatever the transform handle's drag reports.

    The handle is ImGuizmo's and always scales in its own basis, so the tool multiplies the ratio it
    reports into each entity's scale and carries the entities away from the handle's center through
    that basis by the same ratio, which is what makes a group scale as one block rather than each
    about its own origin.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "neweditor/viewtools/toolcontext.h"
#include "neweditor/viewtools/toolinterface.h"
#include "util/array.h"

namespace Editor
{

class ScaleTool final : public ToolInterface
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
