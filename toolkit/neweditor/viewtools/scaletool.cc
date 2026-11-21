//------------------------------------------------------------------------------
//  @file scaletool.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "neweditor/viewtools/scaletool.h"
#include "neweditor/api/shell.h"
#include "neweditor/model/selectioncontext.h"
#include "math/mat4.h"
#include "math/vec3.h"

namespace Editor
{

// A scale component is never allowed through zero, so the handle cannot flip an entity inside out
static const float MinScale = 0.01f;

//------------------------------------------------------------------------------
/**
    The handle pass has already read the drag and says what ratio it has stretched the handle by, so
    each entity's scale takes that ratio and the entity is carried away from the handle's center
    along the same axes, which is what scales a selection as one block.
*/
void
ScaleTool::Update(const ToolView& view)
{
    if (this->drag.dragging)
    {
        if (!view.gizmoUsed)
        {
            this->drag.dragging = false;

            bool changed = false;
            for (IndexT i = 0; i < this->drag.current.Size(); i++)
            {
                if (this->drag.current[i].scale != this->drag.originals[i].scale)
                {
                    changed = true;
                    break;
                }
            }

            if (changed)
                this->context->CommitTransforms("Scale entities", this->drag.current);
            else
                this->context->RestoreTransforms(this->drag.originals);

            this->drag.originals.Clear();
            this->drag.current.Clear();
        }
        else
        {
            const Math::vec3 ratio = view.gizmoScaleRatio;

            // An entity's scale stretches that entity's own axes, so the ratio the handle came back
            // with
            // goes straight onto them - the same axes when the handle runs in the entity's basis,
            // and
            // the entity's own axes otherwise. The positions go by the handle's axes instead, which
            // is
            // what spreads a group out as one block
            const ToolContext::Handle& grabbed = this->context->Grabbed();
            const Math::mat4 toLocal = Math::rotationquat(Math::inverse(grabbed.orientation));
            const Math::mat4 toWorld = Math::rotationquat(grabbed.orientation);

            this->drag.current = this->drag.originals;
            for (IndexT i = 0; i < this->drag.current.Size(); i++)
            {
                const TransformEntry& original = this->drag.originals[i];
                TransformEntry& entry = this->drag.current[i];

                entry.scale = original.scale * ratio;
                if (entry.scale.x < MinScale) entry.scale.x = MinScale;
                if (entry.scale.y < MinScale) entry.scale.y = MinScale;
                if (entry.scale.z < MinScale) entry.scale.z = MinScale;

                const Math::vec3 offset = original.position - grabbed.pivot;
                const Math::vec4 local = toLocal * Math::vec4(offset, 0.0f);
                const Math::vec4 carried = toWorld * Math::vec4(local.x * ratio.x, local.y * ratio.y, local.z * ratio.z, 0.0f);
                entry.position = grabbed.pivot + Math::vec3(carried.x, carried.y, carried.z);

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
ScaleTool::IsModifying() const
{
    return this->drag.dragging;
}

//------------------------------------------------------------------------------
/**
*/
void
ScaleTool::Abort()
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
ScaleTool::Name() const
{
    return "scale";
}

} // namespace Editor
