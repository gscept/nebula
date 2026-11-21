//------------------------------------------------------------------------------
//  @file translatetool.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "neweditor/viewtools/translatetool.h"
#include "neweditor/api/shell.h"
#include "neweditor/model/selectioncontext.h"

namespace Editor
{

//------------------------------------------------------------------------------
/**
    The handle pass has already read the drag and says how far the handle has moved since the drag
    took hold of it, so the preview is the transforms the drag started from moved by that offset.
    The release commits the result, or puts the originals back when nothing moved.
*/
void
TranslateTool::Update(const ToolView& view)
{
    if (this->drag.dragging)
    {
        if (!view.gizmoUsed)
        {
            this->drag.dragging = false;

            bool changed = false;
            for (IndexT i = 0; i < this->drag.current.Size(); i++)
            {
                if (this->drag.current[i].position != this->drag.originals[i].position)
                {
                    changed = true;
                    break;
                }
            }

            if (changed)
                this->context->CommitTransforms("Translate entities", this->drag.current);
            else
                this->context->RestoreTransforms(this->drag.originals);

            this->drag.originals.Clear();
            this->drag.current.Clear();
        }
        else
        {
            this->drag.current = this->drag.originals;
            for (IndexT i = 0; i < this->drag.current.Size(); i++)
            {
                TransformEntry& entry = this->drag.current[i];
                entry.position += view.gizmoOffset;

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
TranslateTool::IsModifying() const
{
    return this->drag.dragging;
}

//------------------------------------------------------------------------------
/**
*/
void
TranslateTool::Abort()
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
TranslateTool::Name() const
{
    return "translate";
}

} // namespace Editor
