#pragma once
//------------------------------------------------------------------------------
/**
    @class Editor::TranslateTool

    Moves the selection by whatever the transform handle's drag reports. The handle itself is
    ImGuizmo's, drawn and read by the context's handle pass, so this tool only turns the drag's
    delta into previews and commits the result when the drag lets go.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "neweditor/viewtools/toolcontext.h"
#include "neweditor/viewtools/toolinterface.h"
#include "util/array.h"

namespace Editor
{

class TranslateTool final : public ToolInterface
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
        // the transforms as they were when the handle grabbed, and where the drag's preview put
        // them
        Util::Array<TransformEntry> originals;
        Util::Array<TransformEntry> current;
    } drag;
};

} // namespace Editor
