//------------------------------------------------------------------------------
//  @file rotatetool.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "neweditor/viewtools/rotatetool.h"
#include "neweditor/api/shell.h"
#include "neweditor/model/selectioncontext.h"
#include "math/mat4.h"
#include "math/vec3.h"

namespace Editor
{

//------------------------------------------------------------------------------
/**
    The handle pass has already read the drag and says what turn it has given the handle, so every
    entity takes part in that turn: its position goes around the handle's center and the turn comes
    in on its world side, which is what turns a group as one.
*/
void
RotateTool::Update(const ToolView& view)
{
    if (this->drag.dragging)
    {
        if (!view.gizmoUsed)
        {
            this->drag.dragging = false;

            bool changed = false;
            for (IndexT i = 0; i < this->drag.current.Size(); i++)
            {
                if (this->drag.current[i].position != this->drag.originals[i].position ||
                    this->drag.current[i].orientation != this->drag.originals[i].orientation)
                {
                    changed = true;
                    break;
                }
            }

            if (changed)
                this->context->CommitTransforms("Rotate entities", this->drag.current);
            else
                this->context->RestoreTransforms(this->drag.originals);

            this->drag.originals.Clear();
            this->drag.current.Clear();
        }
        else
        {
            const Math::mat4 rot = Math::rotationquat(view.gizmoRotation);
            const Math::vec3 pivot = this->context->Grabbed().pivot;

            this->drag.current = this->drag.originals;
            for (IndexT i = 0; i < this->drag.current.Size(); i++)
            {
                const TransformEntry& original = this->drag.originals[i];
                TransformEntry& entry = this->drag.current[i];

                const Math::vec3 offset = original.position - pivot;
                const Math::vec4 turned = rot * Math::vec4(offset, 0.0f);
                entry.position = pivot + Math::vec3(turned.x, turned.y, turned.z);
                entry.orientation = Math::normalize(view.gizmoRotation * original.orientation);

                this->context->PreviewTransform(entry.entity, entry.position, entry.orientation, entry.scale);
            }
        }
        return;
    }

    if (view.escape)
        return; // the context turns escape with no change into a switch back to select

    if (!view.gizmoUsed)
        return;

    const Util::Array<Game::Entity>& entities = this->context->GetShell()->GetSelection()->GetSelection();
    this->context->SnapshotTransforms(entities, this->drag.originals);
    if (this->drag.originals.IsEmpty())
        return;

    this->drag.current = this->drag.originals;
    this->drag.dragging = true;
}

//------------------------------------------------------------------------------
/**
*/
bool
RotateTool::IsModifying() const
{
    return this->drag.dragging;
}

//------------------------------------------------------------------------------
/**
*/
void
RotateTool::Abort()
{
    if (!this->drag.dragging)
        return;

    this->drag.dragging = false;
    this->context->RestoreTransforms(this->drag.originals);
    this->drag.originals.Clear();
    this->drag.current.Clear();
}

//------------------------------------------------------------------------------
/**
*/
const char*
RotateTool::Name() const
{
    return "rotate";
}

} // namespace Editor
