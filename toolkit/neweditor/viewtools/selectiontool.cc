//------------------------------------------------------------------------------
//  @file selectiontool.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "neweditor/viewtools/selectiontool.h"
#include "neweditor/api/shell.h"
#include "neweditor/model/scene.h"
#include "neweditor/model/selectioncontext.h"
#include "imgui.h"
#include "neweditor/imguizmo/imguizmo.h"
#include "basegamefeature/components/orientation.h"
#include "basegamefeature/components/position.h"
#include "basegamefeature/components/scale.h"
#include "game/entity.h"
#include "game/world.h"
#include "math/bbox.h"
#include "math/line.h"
#include "math/scalar.h"

namespace Editor
{

// The marquee is drawn in the color of the mode the drag direction put it in, because the box is
// the
// only thing saying which mode it is. Window is the accent the UI uses, crossing is the same family
// but reads as the other one
static const Math::vec4 WindowMarqueeColor(0.30f, 0.55f, 0.90f, 1.0f);
static const Math::vec4 CrossingMarqueeColor(0.30f, 0.72f, 0.55f, 1.0f);
static const float MarqueeFillAlpha = 0.16f;
static const float MarqueeLineWidth = 1.0f;

// A press that moved less than this is still the click it was, and the release that ends it is
// still
// a pick. Without this a click that drifted a pixel would clear the selection
static const float MinMarqueeDistance = 4.0f;

//------------------------------------------------------------------------------
/**
    The drag attempt starts on the move away from the press, and it is one of two things depending
    on what the press landed on. A selected entity under the cursor is the drag's: it moves the
    whole selection on the plane that cuts that entity, and commits when it released after moving
    long enough. Anything else is the marquee, which is the box from the press to the cursor.

    The click pick runs on the release, on a frame that is not the end of a drag attempt. The
    marquee commits on that same release, and escape or the right button takes the box back instead,
    on a frame where the button is not holding the gesture anymore.
*/
void
SelectionTool::Update(const ToolView& view)
{
    // The frame belongs to a gizmo handle's drag. The press that took the handle and the release that
    // lets it go are the same ones the tool would read as its own gesture, so the tool reads neither
    if (view.gizmoUsed)
        return;

    // The box lives for the length of the gesture that made it, so a release the panel could not
    // report still
    // ends it: the frame the look drag takes the cursor on, and the frames the panel returned early
    // on. Without
    // this the box keeps following the cursor after the button is up
    const bool boxWasActive = this->marquee.active;

    // the pause the old selection context kept over a drag ends with the attempt
    if (!this->drag.dragTimer.Running() && this->drag.paused)
        this->drag.paused = false;

    if (boxWasActive && !view.held && !view.released)
    {
        // the button is up and the release never reached the tool, so the gesture is over
        this->Abort();
        this->marquee.cancelled = true;
    }

    if (view.pressed)
    {
        this->drag.mousePosOnStart = view.mouse;
        this->marquee.start = view.mouse;
        this->marquee.current = view.mouse;
        this->marquee.active = false;
        this->marquee.cancelled = false;
    }

    // Only start translating if the mouse has moved more that a small distance
    if (!this->drag.dragTimer.Running())
    {
        float mouseDistance = (view.mouse - this->drag.mousePosOnStart).length();
        if (mouseDistance > 0.0015f && view.held && !this->marquee.cancelled)
        {
            this->drag.dragTimer.Reset();
            this->drag.dragTimer.Start();
            this->drag.paused = true;
        }
    }

    Math::line ray;
    if (!view.CanvasRay(view.mouse, ray))
        return;

    Game::World* editorWorld = this->context->GetShell()->GetScene()->GetAuthoringWorld();
    const Util::Array<Game::Entity>& selection = this->context->GetShell()->GetSelection()->GetSelection();

    // The frame the attempt arms is the frame it is decided on, and it is decided once
    if (this->drag.dragTimer.Running() && !this->drag.isDirty && !this->marquee.active)
    {
        this->drag.originEntity = this->PickSelected(view);
        if (this->drag.originEntity == Game::Entity::Invalid())
        {
            // Nothing of the selection is under the cursor, so this drag has nothing to move. It is
            // the box instead
            this->marquee.active = true;
        }
        else
        {
            Game::Position entityPos = editorWorld->GetComponent<Game::Position>(this->drag.originEntity);
            this->drag.plane = Math::plane(entityPos, Math::vector(0, 1, 0));
            this->drag.originStartPos = entityPos;
            Math::point startPos;
            if (this->drag.plane.intersect(ray, startPos))
            {
                this->drag.startPos = startPos.vec;
                this->drag.delta = Math::vec3(0);
                this->drag.isDirty = true;
            }
            else
            {
                // failed to find plane. Cancel translation attempt
                this->drag.dragTimer.Stop();
            }
        }
    }

    if (view.released && this->drag.dragTimer.Running())
    {
        this->drag.dragTimer.Stop();
    }

    if (this->marquee.active)
    {
        this->UpdateMarquee(view);
    }
    else if (this->drag.dragTimer.Running())
    {
        bool const xzAxis = !view.alt;

        if (xzAxis)
        {
            this->drag.altDown = false;

            Math::point mousePosOnWorldPlane;
            if (this->drag.plane.intersect(ray, mousePosOnWorldPlane))
            {
                this->drag.delta = mousePosOnWorldPlane.vec - this->drag.startPos;
            }
        }
        else // y axis translation
        {
            // find a good plane. The entity stands where the preview put it, authored plus delta
            Game::Position gameEntityPos = editorWorld->GetComponent<Game::Position>(this->drag.originEntity);
            gameEntityPos += this->drag.delta;

            Math::vector planeNormal = Math::cross(Math::normalize(ray.m), Math::vector(0, 1, 0));
            planeNormal = Math::cross(planeNormal, Math::vector(0, 1, 0));

            Math::plane plane(gameEntityPos, planeNormal);
            Math::point mousePosOnWorldPlane;
            if (plane.intersect(ray, mousePosOnWorldPlane))
            {
                if (!this->drag.altDown)
                {
                    // entering the y axis mode. The measurement starts from the cursor's height
                    // right here, so pressing alt alone never moves the entity
                    this->drag.yBase = mousePosOnWorldPlane.y - this->drag.delta.y;
                    this->drag.altDown = true;
                }

                this->drag.delta.y = mousePosOnWorldPlane.y - this->drag.yBase;
                this->drag.plane =
                    Math::plane(this->drag.startPos + this->drag.delta, Math::vector(0, 1, 0));
            }
        }

        // Snapping. Same behavior as the ImGuizmo tools.
        const Math::vec3 lattice = this->context->SnapValues(TranslateToolId, view.ctrl);
        if (lattice.x > 0.0f || lattice.y > 0.0f || lattice.z > 0.0f)
        {
            const Math::vec3 snapped =
                ToolContext::SnapToLattice(this->drag.originStartPos + this->drag.delta, lattice);
            this->drag.delta = snapped - this->drag.originStartPos;
        }

        Math::vec2 fromScreen, toScreen;
        if (view.Project(this->drag.originStartPos, fromScreen) &&
            view.Project(this->drag.originStartPos + this->drag.delta, toScreen))
        {
            Util::String info;
            if (xzAxis)
                info.Format("X : %5.3f Z : %5.3f", this->drag.delta.x, this->drag.delta.z);
            else
                info.Format("Y : %5.3f", this->drag.delta.y);
            ImGuizmo::DrawTranslationFeedback(
                ImVec2(fromScreen.x, fromScreen.y), ImVec2(toScreen.x, toScreen.y), info.AsCharPtr());
        }

        for (IndexT i = 0; i < selection.Size(); i++)
        {
            const Game::Entity entity = selection[i];

            Game::Position pos = editorWorld->GetComponent<Game::Position>(entity);
            pos += this->drag.delta;

            this->context->PreviewTransform(
                entity,
                pos,
                editorWorld->GetComponent<Game::Orientation>(entity),
                editorWorld->GetComponent<Game::Scale>(entity));
        }
    }
    else if (this->drag.isDirty)
    {
        // Only apply the translation if the translation has happened for more than some threshold
        // duration
        constexpr float minDragTime = 0.1f;
        if (this->drag.delta != Math::vec3(0) && this->drag.dragTimer.GetTime() > minDragTime)
        {
            // User has release gizmo, we can set real transform and add to undo queue
            Util::Array<TransformEntry> entries;
            this->context->SnapshotTransforms(selection, entries);
            for (IndexT i = 0; i < entries.Size(); i++)
                entries[i].position += this->drag.delta;

            this->context->CommitTransforms("Translate entities", entries);
        }
        else
        {
            // Reset game entity position to be same as editors
            Util::Array<TransformEntry> entries;
            this->context->SnapshotTransforms(selection, entries);
            this->context->RestoreTransforms(entries);

            this->drag.paused = false;
        }
        this->drag.isDirty = false;
        this->drag.delta = Math::vec3(0);
    }

    if (view.escape || view.rightPressed)
    {
        if (boxWasActive || this->marquee.active)
        {
            this->Abort();
            this->marquee.cancelled = true;
        }
        else if (view.escape && !selection.IsEmpty())
            this->context->GetShell()->GetSelection()->Clear();
    }

    // The click pick, on the release, unless the frame belongs to a drag attempt the way it did in
    // the old editor when its selection context was paused
    if (view.released && !this->drag.paused)
    {
        const Ptr<SelectionContext> selectionContext = this->context->GetShell()->GetSelection();
        const Game::Entity picked = this->context->Pick(view);

        if (picked != Game::Entity::Invalid())
        {
            if (view.ctrl)
                selectionContext->Toggle(picked);
            else
            {
                Util::Array<Game::Entity> single;
                single.Append(picked);
                selectionContext->SetSelection(single);
            }
        }
        else if (!view.ctrl)
            selectionContext->Clear();
    }
}

//------------------------------------------------------------------------------
/**
    The closest entity of the selection under the cursor, which is what the old tool used to pick
    the plane cutting entity.
*/
Game::Entity
SelectionTool::PickSelected(const ToolView& view) const
{
    Math::line ray;
    if (!view.CanvasRay(view.mouse, ray))
        return Game::Entity::Invalid();

    Scene* scene = this->context->GetShell()->GetScene();
    const Util::Array<Game::Entity>& selection = this->context->GetShell()->GetSelection()->GetSelection();

    float closestDistance = 1e30f;
    Game::Entity closestEntity = Game::Entity::Invalid();

    for (IndexT i = 0; i < selection.Size(); i++)
    {
        Math::bbox box;
        if (!ToolContext::WorldBounds(scene, selection[i], box))
            continue;

        float t;
        if (box.intersects(ray, t) && t < closestDistance)
        {
            closestEntity = selection[i];
            closestDistance = t;
        }
    }

    return closestEntity;
}

//------------------------------------------------------------------------------
/**
    The box from the press to the cursor. The way it is dragged is the mode: to the right it is the
    window, which takes the entities it holds, and to the left it is the crossing, which takes the
    ones it touches. The entities it takes are tested on the bounds the canvas draws them on, which
    is the same test a click on them would have answered with.

    The box is drawn, the entities it would take are outlined, and the release puts the selection on
    them. Ctrl keeps what is already selected and adds the box to it.
*/
void
SelectionTool::UpdateMarquee(const ToolView& view)
{
    this->marquee.current = view.mouse;

    const float distance = (this->marquee.current - this->marquee.start).length();
    if (distance < MinMarqueeDistance)
    {
        // A box this small is the click the press was, so the release that ends it is still a pick.
        // The gesture is over on that release, and a box that never grew is not a gesture: leaving
        // it
        // active here is what made it follow the cursor after the button came up
        this->drag.paused = false;
        if (view.released)
            this->marquee.active = false;
        return;
    }

    // The frame the box ends on is the box's, not a pick
    this->drag.paused = true;

    const Math::vec2 boxMin(
        Math::min(this->marquee.start.x, this->marquee.current.x),
        Math::min(this->marquee.start.y, this->marquee.current.y));
    const Math::vec2 boxMax(
        Math::max(this->marquee.start.x, this->marquee.current.x),
        Math::max(this->marquee.start.y, this->marquee.current.y));

    const bool crossing = this->marquee.current.x < this->marquee.start.x;

    Scene* scene = this->context->GetShell()->GetScene();
    const Util::Array<Editable>& editables = scene->GetEditables();

    Util::Array<Game::Entity> taken;
    for (IndexT i = 0; i < editables.Size(); i++)
    {
        if (!editables[i].guid.IsValid())
            continue;

        Math::bbox box;
        if (!ToolContext::WorldBounds(scene, editables[i].entity, box))
            continue;

        // The bounds as the canvas draws them. An entity with a corner behind the camera has no box
        // on the canvas, and the marquee cannot hold or touch it
        Math::vec2 screenMin(1e30f, 1e30f);
        Math::vec2 screenMax(-1e30f, -1e30f);
        bool onCanvas = true;
        for (int corner = 0; corner < 8; corner++)
        {
            Math::vec2 projected;
            if (!view.Project(box.corner_point(corner).vec, projected))
            {
                onCanvas = false;
                break;
            }

            screenMin.x = Math::min(screenMin.x, projected.x);
            screenMin.y = Math::min(screenMin.y, projected.y);
            screenMax.x = Math::max(screenMax.x, projected.x);
            screenMax.y = Math::max(screenMax.y, projected.y);
        }

        if (!onCanvas)
            continue;

        const bool held =
            screenMin.x >= boxMin.x && screenMax.x <= boxMax.x &&
            screenMin.y >= boxMin.y && screenMax.y <= boxMax.y;
        const bool touched =
            screenMax.x >= boxMin.x && screenMin.x <= boxMax.x &&
            screenMax.y >= boxMin.y && screenMin.y <= boxMax.y;

        if (crossing ? touched : held)
        {
            taken.Append(editables[i].entity);
            this->context->DrawHighlight(editables[i].entity, Game::Entity::Invalid());
        }
    }

    const Math::vec4& color = crossing ? CrossingMarqueeColor : WindowMarqueeColor;
    const ImVec2 a(boxMin.x, boxMin.y);
    const ImVec2 b(boxMax.x, boxMax.y);

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(a, b, ImGui::ColorConvertFloat4ToU32(ImVec4(color.x, color.y, color.z, MarqueeFillAlpha)));
    drawList->AddRect(a, b, ImGui::ColorConvertFloat4ToU32(ImVec4(color.x, color.y, color.z, 1.0f)),
        0.0f, MarqueeLineWidth, ImDrawFlags_RoundCornersNone);

    Util::String info;
    info.Format("Marquee: %s (%d)", crossing ? "crossing" : "window", (int) taken.Size());
    this->context->GetShell()->SetStatus(info.AsCharPtr(), 0.5f);

    if (view.released)
    {
        const Ptr<SelectionContext> selectionContext = this->context->GetShell()->GetSelection();

        // One SetSelection is one undo entry, which is what adding the box a toggle at a time would
        // not
        // be. Ctrl keeps what is already selected and adds the box to it, and without ctrl the box
        // is
        // the selection
        Util::Array<Game::Entity> result;
        if (view.ctrl)
            result = selectionContext->GetSelection();

        for (IndexT i = 0; i < taken.Size(); i++)
            result.InsertSorted(taken[i]);

        selectionContext->SetSelection(result);

        this->marquee.active = false;
    }
}

//------------------------------------------------------------------------------
/**
*/
bool
SelectionTool::IsModifying() const
{
    return this->drag.dragTimer.Running() || this->marquee.active;
}

//------------------------------------------------------------------------------
/**
*/
void
SelectionTool::Abort()
{
    if (this->drag.dragTimer.Running())
        this->drag.dragTimer.Stop();

    if (this->drag.isDirty)
    {
        Util::Array<TransformEntry> entries;
        this->context->SnapshotTransforms(this->context->GetShell()->GetSelection()->GetSelection(), entries);
        this->context->RestoreTransforms(entries);
    }

    this->drag.isDirty = false;
    this->drag.paused = false;
    this->drag.altDown = false;
    this->drag.delta = Math::vec3(0);
    this->drag.originEntity = Game::Entity::Invalid();

    // The box has not written anything yet, so letting go of it is only letting go of the box
    this->marquee.active = false;
}

//------------------------------------------------------------------------------
/**
*/
const char*
SelectionTool::Name() const
{
    return "select";
}

} // namespace Editor
