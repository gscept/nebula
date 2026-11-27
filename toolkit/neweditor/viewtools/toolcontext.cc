//------------------------------------------------------------------------------
//  @file toolcontext.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "neweditor/viewtools/toolcontext.h"
#include "neweditor/viewtools/selectiontool.h"
#include "neweditor/viewtools/translatetool.h"
#include "neweditor/viewtools/rotatetool.h"
#include "neweditor/viewtools/scaletool.h"
#include "neweditor/commands/setcomponentcommand.h"
#include "neweditor/model/editcommandmanager.h"
#include "neweditor/console/editorcommands.h"
#include "neweditor/api/shell.h"
#include "neweditor/base/editorlog.h"
#include "neweditor/input/editorkeybinds.h"
#include "neweditor/model/scene.h"
#include "neweditor/model/selectioncontext.h"
#include "neweditor/api/gizmo.h"
#include "imgui.h"
#include "neweditor/imguizmo/imguizmo.h"
#include "basegamefeature/components/basegamefeature.h"
#include "basegamefeature/components/orientation.h"
#include "basegamefeature/components/position.h"
#include "basegamefeature/components/scale.h"
#include "game/world.h"
#include "math/line.h"
#include "math/mat4.h"
#include "math/scalar.h"

namespace Editor
{

__ImplementClass(Editor::ToolContext, 'TLCC', Core::RefCounted);

// The names the tool command and the status bar use, in ToolId order
static const char* const ToolNames[NumToolIds] = { "select", "translate", "rotate", "scale" };

// The names the frame dropdown, the gizmo command and the canvas' status line use, in
// FrameOfReference order.
// A frame added to the enum needs its name here too, which is what makes the dropdown list the
// frames from
// the enum rather than from a fixed pair of items
static const char* const FrameNames[NumFrames] = { "world", "local" };

// The outlines follow the old editor's colors: orange for the selection, dimmer for the hover
static const Math::vec4 SelectionOutlineColor(1.0f, 0.30f, 0.0f, 1.0f);
static const Math::vec4 HoverOutlineColor(0.5f, 0.15f, 0.0f, 1.0f);

// The line between an entity and the one a panel is pointing at. The same family as the hover
// outline, but
// stronger, because a line is thinner than the box it is drawn with
static const Math::vec4 HighlightLineColor(0.85f, 0.28f, 0.05f, 1.0f);

static const Math::vec4 GridXAxisColor(0.72f, 0.24f, 0.20f, 1.0f);
static const Math::vec4 GridZAxisColor(0.20f, 0.36f, 0.74f, 1.0f);

// An outline is the bounds drawn as its edges, over what the frame has already drawn. The grid
// lines are not:
// they are the plane the scene is dropped onto, so they can be behind geometry
static const Gizmo::DrawFlag OutlineFlags = (Gizmo::DrawFlag)(Gizmo::DrawFlag_Wireframe | Gizmo::DrawFlag_AlwaysOnTop);

static const float MinColorComponent = 0.0f;
static const float MaxColorComponent = 1.0f;

static const float MinGridStep = 0.01f;
static const float MaxGridStep = 100.0f;
static const float MinGridExtent = 1.0f;
static const float MaxGridExtent = 1000.0f;
static const int MinGridSubdivisions = 1;
static const int MaxGridSubdivisions = 10;

static const int MaxGridLines = 256;

static const float MaxTranslateSnap = 1000.0f;
static const float MaxRotateSnap = 360.0f;
static const float MaxScaleSnap = 10.0f;

//------------------------------------------------------------------------------
/**
*/
void
ToolContext::Setup(Shell* core)
{
    this->core = core;

    this->tools[SelectToolId] = new SelectionTool;
    this->tools[TranslateToolId] = new TranslateTool;
    this->tools[RotateToolId] = new RotateTool;
    this->tools[ScaleToolId] = new ScaleTool;
    for (IndexT i = 0; i < NumToolIds; i++)
        this->tools[i]->SetContext(this);

    this->activeTool = SelectToolId;

    core->GetCommands().Register("tool", "tool [select|translate|rotate|scale] - print or set the active viewport tool",
        Util::Delegate<void(const Util::Array<Util::String>&)>::FromMethod<ToolContext, &ToolContext::CommandTool>(this));

    core->GetCommands().Register("gizmo", "gizmo [world|local|toggle|list] - cycle or set the frame the gizmo works in, or print the components that have one",
        Util::Delegate<void(const Util::Array<Util::String>&)>::FromMethod<ToolContext, &ToolContext::CommandGizmo>(this));

    core->GetCommands().Register("grid", "grid [on|off|toggle|step <units>|subdiv <n>|size <units>|color <r> <g> <b> [a]|minor <r> <g> <b> [a>] - print or set the canvas' grid",
        Util::Delegate<void(const Util::Array<Util::String>&)>::FromMethod<ToolContext, &ToolContext::CommandGrid>(this));
    core->GetCommands().Register("snap", "snap [on|off|toggle|grid|increments|translate <units>|rotate <degrees>|scale <ratio>] - print or set what a drag lands on",
        Util::Delegate<void(const Util::Array<Util::String>&)>::FromMethod<ToolContext, &ToolContext::CommandSnap>(this));

    // The old editor's keys: W translate, E rotate, R scale, and escape returns to select. Q
    // selects
    // because the selection tool is the one worth a key of its own
    core->GetKeybinds().Register("tool.select", Input::Key::Q, Input::Modifier::NoModifier, "tool select");
    core->GetKeybinds().Register("tool.translate", Input::Key::W, Input::Modifier::NoModifier, "tool translate");
    core->GetKeybinds().Register("tool.rotate", Input::Key::E, Input::Modifier::NoModifier, "tool rotate");
    core->GetKeybinds().Register("tool.scale", Input::Key::R, Input::Modifier::NoModifier, "tool scale");

    core->GetKeybinds().Register("gizmo", Input::Key::G, Input::Modifier::NoModifier, "gizmo");

    core->GetKeybinds().Register("snap.toggle", Input::Key::X, Input::Modifier::NoModifier, "snap toggle");
    core->GetKeybinds().Register("grid.toggle", Input::Key::G, Input::Modifier::ShiftModifier, "grid toggle");
}

//------------------------------------------------------------------------------
/**
*/
void
ToolContext::Discard()
{
    if (this->core == nullptr)
        return;

    for (IndexT i = 0; i < NumToolIds; i++)
    {
        delete this->tools[i];
        this->tools[i] = nullptr;
    }
    this->core = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
void
ToolContext::SetActiveTool(ToolId tool)
{
    if (tool < 0 || tool >= NumToolIds || tool == this->activeTool)
        return;

    // A tool that is in the middle of a change keeps previews of it in the view, and switching away
    // from it leaves those previews behind. The handle lets go of the drag for the same reason
    this->AbortActiveTool();

    this->activeTool = tool;
    this->core->SetStatus(Util::String("Tool: ") + this->ActiveToolName(), 2.0f);
}

//------------------------------------------------------------------------------
/**
*/
ToolId
ToolContext::GetActiveTool() const
{
    return this->activeTool;
}

//------------------------------------------------------------------------------
/**
*/
void
ToolContext::AbortActiveTool()
{
    this->tools[this->activeTool]->Abort();
    this->handleGrabbed = false;

    // let go of an in-progress handle drag too, so the widget does not carry its grab into the
    // next scene. Disabling the widget is what cancels its interaction
    ImGuizmo::Enable(false);
    ImGuizmo::Enable(true);

    // A component gizmo handle is a drag of its own, and it has previews of its own to put back
    Gizmo::AbortDrag();
}

//------------------------------------------------------------------------------
/**
*/
bool
ToolContext::SetActiveToolByName(const Util::String& name)
{
    for (IndexT i = 0; i < NumToolIds; i++)
    {
        if (name == ToolNames[i])
        {
            this->SetActiveTool((ToolId)i);
            return true;
        }
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
const char*
ToolContext::ToolName(ToolId id) const
{
    if (id < 0 || id >= NumToolIds)
        return "none";
    return ToolNames[id];
}

//------------------------------------------------------------------------------
/**
*/
const char*
ToolContext::ActiveToolName() const
{
    return this->ToolName(this->activeTool);
}

//------------------------------------------------------------------------------
/**
*/
int
ToolContext::GetFrame() const
{
    return this->frame;
}

//------------------------------------------------------------------------------
/**
    A frame that changes while nothing is dragging takes effect on the next frame's handle, which is
    rebuilt from the selection. A frame that changes during a drag has to put the basis back under
    the handle, because the widget reports the change since the frame it took hold of the handle in,
    and a grab taken in one basis cannot be read against a handle in another. This is the path the
    gizmo key takes: the key fires while the drag is held.
*/
void
ToolContext::SetFrame(int frame)
{
    if (frame < 0 || frame >= NumFrames)
        return;

    if (frame == this->frame)
        return;

    this->frame = frame;

    if (this->handleGrabbed)
    {
        const Util::Array<Game::Entity>& entities = this->core->GetSelection()->GetSelection();

        Util::Array<TransformEntry> entries;
        this->SnapshotTransforms(entities, entries);

        this->handle = this->SelectionHandle(entries, this->frame);
        this->grab = this->handle;
    }

    this->core->SetStatus(Util::String("Frame: ") + this->FrameName(this->frame), 2.0f);
}

//------------------------------------------------------------------------------
/**
*/
const char*
ToolContext::FrameName(int frame) const
{
    if (frame < 0 || frame >= NumFrames)
        return "none";
    return FrameNames[frame];
}

//------------------------------------------------------------------------------
/**
    Escape is the tool chain the old editor had: escape during a change aborts it inside the tool,
    escape with no change goes back to the select tool, and the select tool takes escape as a
    request to clear the selection.

    The hover pick runs here rather than in the draw so that an outline lands on the entity the
    cursor is over this frame.
*/
void
ToolContext::UpdateView(const ToolView& view)
{
    ToolInterface* tool = this->tools[this->activeTool];
    tool->Update(view);

    if (view.escape && !tool->IsModifying() && this->activeTool != SelectToolId)
        this->SetActiveTool(SelectToolId);

    if (tool->IsModifying() || !view.hovered)
        this->hovered = Game::Entity::Invalid();
    else
        this->hovered = this->Pick(view);
}

//------------------------------------------------------------------------------
/**
    The transform handle is ImGuizmo's: one call draws it on the canvas and reads this frame's drag.
    It is handed the handle as a matrix and answers with the handle as the drag wants it, so the
    handle lives in the context for as long as the drag runs - the widget's rotate only reports the
    turn since the previous frame and adds up through the matrix. What the tools are told is what
    the drag has done to the handle since the widget took hold of it, which is the one thing all
    three of them need.
*/
void
ToolContext::DrawView(ToolView& view)
{
    if (!view.IsValid())
        return;

    this->DrawGrid();
    this->DrawOutlines(view);

    // The draw list is set even for the select tool, because the selection tool draws the gizmo's
    // translation feedback for its own drag through it. The rect is the canvas' for the same
    // reason:
    // the widget's own math reads it even when no handle is drawn
    ImGuizmo::SetRect(view.canvasMin.x, view.canvasMin.y, view.canvasSize.x, view.canvasSize.y);
    ImGuizmo::SetDrawlist();
    ImGuizmo::SetOrthographic(view.projection.r[3].w != 0.0f);

    if (Gizmo::DrawComponentGizmos(view))
    {
        // Tell the tools which handle has this frame
        view.gizmoUsed = true;
        return;
    }

    if (this->activeTool == SelectToolId)
        return;

    const Util::Array<Game::Entity>& entities = this->core->GetSelection()->GetSelection();

    Util::Array<TransformEntry> entries;
    this->SnapshotTransforms(entities, entries);
    if (entries.IsEmpty() && !this->handleGrabbed)
        return;

    // an idle handle follows the selection, and the frame a drag starts on is where the drag starts
    // measuring itself from
    if (!this->handleGrabbed)
    {
        this->handle = this->SelectionHandle(entries, view.frame);
        this->grab = this->handle;
    }

    ImGuizmo::Enable(true);
    ImGuizmo::SetRect(view.canvasMin.x, view.canvasMin.y, view.canvasSize.x, view.canvasSize.y);
    ImGuizmo::SetDrawlist();
    ImGuizmo::SetOrthographic(view.projection.r[3].w != 0.0f);

    ImGuizmo::OPERATION operation = ImGuizmo::TRANSLATE;
    if (this->activeTool == RotateToolId)
        operation = ImGuizmo::ROTATE;
    else if (this->activeTool == ScaleToolId)
        operation = ImGuizmo::SCALE;

    // The math library stores matrices row major with the translation in the last row, which is the
    // same layout the widget reads, so the matrices go across as a copy
    float viewMatrix[16], projMatrix[16], modelMatrix[16];
    memcpy(viewMatrix, &view.view.m[0][0], sizeof(viewMatrix));
    memcpy(projMatrix, &view.projection.m[0][0], sizeof(projMatrix));

    const Math::mat4 handleMatrix = this->handle.Matrix();
    memcpy(modelMatrix, &handleMatrix.m[0][0], sizeof(modelMatrix));

    const Math::vec3 snapValues = this->SnapValues((ToolId)this->activeTool, ImGui::GetIO().KeyCtrl);
    float snap[3] = { snapValues.x, snapValues.y, snapValues.z };
    const bool snapped = snapValues.x > 0.0f || snapValues.y > 0.0f || snapValues.z > 0.0f;

    ImGuizmo::Manipulate(viewMatrix, projMatrix, operation,
        view.frame == LocalFrame ? ImGuizmo::LOCAL : ImGuizmo::WORLD, modelMatrix,
        nullptr, snapped ? snap : nullptr);

    Math::mat4 dragged;
    memcpy(&dragged.m[0][0], modelMatrix, sizeof(modelMatrix));
    this->handle.SetFromMatrix(dragged);

    this->handleGrabbed = ImGuizmo::IsUsing();

    view.gizmoUsed = this->handleGrabbed;
    view.gizmoOffset = this->handle.pivot - this->grab.pivot;
    // The turn is the one the handle has taken, in world terms: taking the frame it was grabbed in
    // out
    // of the frame it is in now. The tools put this on the other side of each entity's own turn,
    // which is
    // what turns everything in the group the same way the widget turned the handle
    view.gizmoRotation = Math::normalize(this->handle.orientation * Math::inverse(this->grab.orientation));
    view.gizmoScaleRatio = this->handle.scale;
    view.gizmoScaleRatio /= this->grab.scale;
}

//------------------------------------------------------------------------------
/**
    The grid lies on y=0 and is centered on the world's origin, which is the plane entities are
    dropped onto and the plane the camera is reset to look across.
*/
void
ToolContext::DrawGrid() const
{
    if (!this->grid.visible)
        return;

    const float extent = this->grid.extent;
    float step = this->GridMinorStep();
    if ((extent / step) * 2.0f + 1.0f > (float)MaxGridLines)
        step = this->grid.step;
    // A big extent can be too fine even at its major lattice, and then the grid is drawn as a
    // lattice
    // coarse enough to fit the ceiling, which has every line of it come out major
    if ((extent / step) * 2.0f + 1.0f > (float)MaxGridLines)
        step = extent / (float)(MaxGridLines / 2);

    const int count = (int)(extent / step + 0.5f);
    int subdivisions = (int)(this->grid.step / step + 0.5f);
    if (subdivisions < 1)
        subdivisions = 1;

    for (int i = -count; i <= count; i++)
    {
        const bool major = (i % subdivisions) == 0;
        const Math::vec4& color = major ? this->grid.majorColor : this->grid.minorColor;
        const float width = major ? 1.5f : 1.0f;
        const float at = i * step;

        Gizmo::DrawLine(Math::vec3(at, 0.0f, -extent), Math::vec3(at, 0.0f, extent), color, width);
        Gizmo::DrawLine(Math::vec3(-extent, 0.0f, at), Math::vec3(extent, 0.0f, at), color, width);
    }

    Gizmo::DrawLine(Math::vec3(0.0f, 0.0f, -extent), Math::vec3(0.0f, 0.0f, extent), GridZAxisColor, 2.0f);
    Gizmo::DrawLine(Math::vec3(-extent, 0.0f, 0.0f), Math::vec3(extent, 0.0f, 0.0f), GridXAxisColor, 2.0f);
}

//------------------------------------------------------------------------------
/**
    The bounds are the same ones picking uses, which is what makes the outline land on what a click
    on it would have picked. The hovered entity gets the hover color even when it is selected, the
    same way the old editor drew it once instead of twice.
*/
void
ToolContext::DrawOutlines(const ToolView& view)
{
    const Ptr<SelectionContext> selection = this->core->GetSelection();
    const Util::Array<Game::Entity>& entities = selection->GetSelection();
    Scene* scene = this->core->GetScene();

    for (IndexT i = 0; i < entities.Size(); i++)
    {
        if (entities[i] == this->hovered)
            continue;

        Math::bbox box;
        if (WorldBounds(scene, entities[i], box))
        {
            Gizmo::DrawBox(box, SelectionOutlineColor, 1.0f, OutlineFlags);
        }
    }

    if (this->hovered != Game::Entity::Invalid())
    {
        Math::bbox box;
        if (WorldBounds(scene, this->hovered, box))
        {
            Gizmo::DrawBox(box, HoverOutlineColor, 1.0f, OutlineFlags);
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
ToolContext::SnapshotTransforms(const Util::Array<Game::Entity>& entities, Util::Array<TransformEntry>& out) const
{
    out.Clear();

    Game::World* world = this->core->GetScene()->GetAuthoringWorld();

    for (IndexT i = 0; i < entities.Size(); i++)
    {
        const Game::Entity entity = entities[i];

        TransformEntry entry;
        entry.entity = entity;
        entry.position = world->GetComponent<Game::Position>(entity);
        entry.orientation = world->GetComponent<Game::Orientation>(entity);
        entry.scale = world->GetComponent<Game::Scale>(entity);
        out.Append(entry);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
ToolContext::PreviewTransform(Game::Entity entity, const Math::vec3& position, const Math::quat& orientation,
    const Math::vec3& scale) const
{
    Scene* scene = this->core->GetScene();

    const Game::Position pos(position);
    const Game::Orientation orient(orientation);
    const Game::Scale sca(scale);

    scene->PreviewComponent(entity, Game::GetComponentId<Game::Position>(), &pos);
    scene->PreviewComponent(entity, Game::GetComponentId<Game::Orientation>(), &orient);
    scene->PreviewComponent(entity, Game::GetComponentId<Game::Scale>(), &sca);
}

//------------------------------------------------------------------------------
/**
    The component is only commanded when the dragged value differs from the authored one, so a click
    that never moved the handle does not become an undo entry. The commands take the authored value
    when they run, which is still the value from before the drag because the previews never touched
    the authoring world.
*/
void
ToolContext::CommitTransforms(const char* name, const Util::Array<TransformEntry>& entries) const
{
    Scene* scene = this->core->GetScene();
    Game::World* world = scene->GetAuthoringWorld();
    Ptr<EditCommandManager> manager = scene->GetCommandManager();

    manager->BeginMacro(name);

    for (IndexT i = 0; i < entries.Size(); i++)
    {
        const TransformEntry& entry = entries[i];
        if (!scene->HasInstance(entry.entity))
            continue;

        const Game::Position pos(entry.position);
        if (!(world->GetComponent<Game::Position>(entry.entity) == pos))
        {
            Ptr<SetComponentCommand> command = SetComponentCommand::Create();
            command->scene = scene;
            command->entity = entry.entity;
            command->component = Game::GetComponentId<Game::Position>();
            command->newValue.Set(&pos, sizeof(Game::Position));
            manager->Execute(command);
        }

        const Game::Orientation orient(entry.orientation);
        if (!(world->GetComponent<Game::Orientation>(entry.entity) == orient))
        {
            Ptr<SetComponentCommand> command = SetComponentCommand::Create();
            command->scene = scene;
            command->entity = entry.entity;
            command->component = Game::GetComponentId<Game::Orientation>();
            command->newValue.Set(&orient, sizeof(Game::Orientation));
            manager->Execute(command);
        }

        const Game::Scale scale(entry.scale);
        if (!(world->GetComponent<Game::Scale>(entry.entity) == scale))
        {
            Ptr<SetComponentCommand> command = SetComponentCommand::Create();
            command->scene = scene;
            command->entity = entry.entity;
            command->component = Game::GetComponentId<Game::Scale>();
            command->newValue.Set(&scale, sizeof(Game::Scale));
            manager->Execute(command);
        }
    }

    manager->EndMacro();
}

//------------------------------------------------------------------------------
/**
*/
void
ToolContext::RestoreTransforms(const Util::Array<TransformEntry>& entries) const
{
    Scene* scene = this->core->GetScene();

    for (IndexT i = 0; i < entries.Size(); i++)
    {
        // A preview writes the transform as well as the world components, because the components
        // are what the
        // hierarchy resolves from it, so putting the view back has to take the transform back too
        scene->RestoreComponent(entries[i].entity, Game::GetComponentId<Game::HTransform>());
        scene->RestoreComponent(entries[i].entity, Game::GetComponentId<Game::Position>());
        scene->RestoreComponent(entries[i].entity, Game::GetComponentId<Game::Orientation>());
        scene->RestoreComponent(entries[i].entity, Game::GetComponentId<Game::Scale>());
    }
}

//------------------------------------------------------------------------------
/**
*/
Math::vec3
ToolContext::Pivot(const Util::Array<TransformEntry>& entries)
{
    Math::vec3 pivot(0);
    for (IndexT i = 0; i < entries.Size(); i++)
        pivot += entries[i].position;

    if (!entries.IsEmpty())
        pivot = pivot * (1.0f / (float)entries.Size());
    return pivot;
}

//------------------------------------------------------------------------------
/**
*/
Math::quat
ToolContext::GizmoOrientation(const Util::Array<TransformEntry>& entries, int frame)
{
    if (frame == LocalFrame && !entries.IsEmpty())
        return entries.Back().orientation;
    return Math::quat(0, 0, 0, 1);
}

//------------------------------------------------------------------------------
/**
*/
const GridSettings&
ToolContext::GetGrid() const
{
    return this->grid;
}

//------------------------------------------------------------------------------
/**
*/
void
ToolContext::SetGrid(const GridSettings& grid)
{
    this->grid.visible = grid.visible;
    this->grid.step = Math::clamp(grid.step, MinGridStep, MaxGridStep);
    this->grid.subdivisions = Math::clamp(grid.subdivisions, MinGridSubdivisions, MaxGridSubdivisions);
    this->grid.extent = Math::clamp(grid.extent, MinGridExtent, MaxGridExtent);

    for (IndexT i = 0; i < 4; i++)
    {
        this->grid.majorColor[i] = Math::clamp(grid.majorColor[i], MinColorComponent, MaxColorComponent);
        this->grid.minorColor[i] = Math::clamp(grid.minorColor[i], MinColorComponent, MaxColorComponent);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
ToolContext::ToggleGrid()
{
    GridSettings grid = this->grid;
    grid.visible = !grid.visible;
    this->SetGrid(grid);

    this->core->SetStatus(Util::String("Grid: ") + (this->grid.visible ? "on" : "off"), 2.0f);
}

//------------------------------------------------------------------------------
/**
*/
float
ToolContext::GridMinorStep() const
{
    return this->grid.step / (float)this->grid.subdivisions;
}

//------------------------------------------------------------------------------
/**
*/
Math::vec3
ToolContext::SnapToGrid(const Math::vec3& pos) const
{
    const float step = this->GridMinorStep();
    return SnapToLattice(pos, Math::vec3(step, step, step));
}

//------------------------------------------------------------------------------
/**
*/
Math::vec3
ToolContext::SnapToLattice(const Math::vec3& pos, const Math::vec3& lattice)
{
    Math::vec3 snapped = pos;
    if (lattice.x > 0.0f)
        snapped.x = Math::round(pos.x / lattice.x) * lattice.x;
    if (lattice.y > 0.0f)
        snapped.y = Math::round(pos.y / lattice.y) * lattice.y;
    if (lattice.z > 0.0f)
        snapped.z = Math::round(pos.z / lattice.z) * lattice.z;
    return snapped;
}

//------------------------------------------------------------------------------
/**
*/
const SnapSettings&
ToolContext::GetSnapping() const
{
    return this->snapping;
}

//------------------------------------------------------------------------------
/**
*/
void
ToolContext::SetSnapping(const SnapSettings& snap)
{
    this->snapping.toGrid = snap.toGrid;
    this->snapping.toIncrements = snap.toIncrements;
    this->snapping.translate = Math::clamp(snap.translate, 0.0f, MaxTranslateSnap);
    this->snapping.rotate = Math::clamp(snap.rotate, 0.0f, MaxRotateSnap);
    this->snapping.scale = Math::clamp(snap.scale, 0.0f, MaxScaleSnap);
}

//------------------------------------------------------------------------------
/**
*/
void
ToolContext::ToggleSnapping()
{
    const bool on = this->snapping.toGrid || this->snapping.toIncrements;

    SnapSettings snap = this->snapping;
    snap.toGrid = !on;
    snap.toIncrements = !on;
    this->SetSnapping(snap);

    this->core->SetStatus(Util::String("Snapping: ") + this->SnapSourceName(), 2.0f);
}

//------------------------------------------------------------------------------
/**
*/
bool
ToolContext::SnapsToGrid() const
{
    return this->snapping.toGrid;
}

//------------------------------------------------------------------------------
/**
*/
bool
ToolContext::SnapsToIncrements() const
{
    return this->snapping.toIncrements;
}

//------------------------------------------------------------------------------
/**
*/
const char*
ToolContext::SnapSourceName(bool invert) const
{
    const bool toGrid = invert ? !this->snapping.toGrid : this->snapping.toGrid;
    const bool toIncrements = invert ? !this->snapping.toIncrements : this->snapping.toIncrements;

    if (toGrid && toIncrements)
        return "grid + increments";
    if (toGrid)
        return "grid";
    if (toIncrements)
        return "increments";
    return "off";
}

//------------------------------------------------------------------------------
/**
*/
Math::vec3
ToolContext::SnapValues(ToolId tool, bool invert) const
{
    const bool toGrid = invert ? !this->snapping.toGrid : this->snapping.toGrid;
    const bool toIncrements = invert ? !this->snapping.toIncrements : this->snapping.toIncrements;

    if (tool == RotateToolId)
        return Math::vec3(toIncrements ? this->snapping.rotate : 0.0f, 0.0f, 0.0f);

    if (tool == ScaleToolId)
        return Math::vec3(toIncrements ? this->snapping.scale : 0.0f, 0.0f, 0.0f);

    const float grid = toGrid ? this->GridMinorStep() : 0.0f;
    const float increment = toIncrements ? this->snapping.translate : 0.0f;

    return Math::vec3(
        grid > 0.0f ? grid : increment,
        increment > 0.0f ? increment : grid,
        grid > 0.0f ? grid : increment);
}

//------------------------------------------------------------------------------
/**
    The handle is a transform in parts, so it goes over to the widget through the same composition
    the renderer draws an entity with. That is what makes the handle's axes land on the entity's
    axes.
*/
Math::mat4
ToolContext::Handle::Matrix() const
{
    return Math::trs(this->pivot, this->orientation, this->scale);
}

//------------------------------------------------------------------------------
/**
    Each axis is read back off the direction it points along: the directions the widget left behind
    give the turn and the lengths along them give the scale, which is the reading its scale drag
    does to the matrix, undone directly instead of by the iterative polar decomposition the math
    library uses.
*/
void
ToolContext::Handle::SetFromMatrix(const Math::mat4& matrix)
{
    this->pivot = Math::xyz(matrix.position());

    Math::mat4 directions;
    directions.x_axis() = Math::normalize(matrix.x_axis());
    directions.y_axis() = Math::normalize(matrix.y_axis());
    directions.z_axis() = Math::normalize(matrix.z_axis());
    this->orientation = Math::normalize(Math::rotationmatrix(directions));

    const Math::mat4 axes = Math::rotationquat(this->orientation);
    this->scale = Math::vec3(
        Math::dot(Math::xyz(matrix.x_axis()), Math::xyz(axes.x_axis())),
        Math::dot(Math::xyz(matrix.y_axis()), Math::xyz(axes.y_axis())),
        Math::dot(Math::xyz(matrix.z_axis()), Math::xyz(axes.z_axis())));
}

//------------------------------------------------------------------------------
/**
    The drag is measured against this rather than against the handle as it is now, because the
    handle is what the widget has been adding the frames' turns up to, and a tool wants the one
    change the whole drag has made
*/
const ToolContext::Handle&
ToolContext::Grabbed() const
{
    return this->grab;
}

//------------------------------------------------------------------------------
/**
*/
ToolContext::Handle
ToolContext::SelectionHandle(const Util::Array<TransformEntry>& entries, int frame)
{
    Handle handle;
    handle.pivot = Pivot(entries);
    handle.orientation = GizmoOrientation(entries, frame);
    return handle;
}

//------------------------------------------------------------------------------
/**
    Picking walks the authored records and tests the ray against the world bounds of each one, which
    is the whole point of the entity being in the canvas. An editor picking pass is a scan and not a
    BVH, which is what the old editor needed the model context's tree for and what this does not.
*/
Game::Entity
ToolContext::Pick(const ToolView& view) const
{
    Math::line ray;
    if (!view.CanvasRay(view.mouse, ray))
        return Game::Entity::Invalid();

    Scene* scene = this->core->GetScene();
    const Util::Array<Editable>& editables = scene->GetEditables();

    Game::Entity best = Game::Entity::Invalid();
    float bestT = 1e9f;

    for (IndexT i = 0; i < editables.Size(); i++)
    {
        if (!editables[i].guid.IsValid())
            continue;

        Math::bbox box;
        if (!WorldBounds(scene, editables[i].entity, box))
            continue;

        float t = 0.0f;
        if (box.intersects(ray, t) && t > 0.0f && t < bestT)
        {
            bestT = t;
            best = editables[i].entity;
        }
    }

    return best;
}

//------------------------------------------------------------------------------
/**
    The bounds come from the sim world, which is the one the canvas draws, so outlines and picks
    land on an entity where it stands while a preview has it on the move. That is what the old
    editor's live model bounds did. The model bounds are local, so the entity transform takes them
    to the world and the box grows to hold the turned box. An entity with no model gets a box at the
    position and scale it was authored with, which is what makes an entity the author only gave a
    transform to pickable once it is in the canvas at all.
*/
bool
ToolContext::WorldBounds(Scene* scene, Game::Entity entity, Math::bbox& out)
{
    if (entity == Game::Entity::Invalid() || !scene->HasInstance(entity))
        return false;

    Game::World* world = scene->GetSimWorld();
    Game::Entity shown = scene->ToGameEntity(entity);
    if (shown == Game::Entity::Invalid() || !world->HasInstance(shown))
    {
        // not mirrored into the sim world, so the authored record is all there is to go by
        world = scene->GetAuthoringWorld();
        shown = entity;
    }

    const Math::vec3 pos = world->GetComponent<Game::Position>(shown);
    const Math::quat orient = world->GetComponent<Game::Orientation>(shown);
    const Math::vec3 scale = world->GetComponent<Game::Scale>(shown);

    Math::bbox local;
    if (Gizmo::GetBounds(world, shown, local))
    {
        out = local;
        out.transform(Math::trs(pos, orient, scale));
        return true;
    }

    // Nothing drawn for the entity, so the box is the scale it stands at
    const Math::vec3 extents(Math::abs(scale.x) * 0.5f, Math::abs(scale.y) * 0.5f, Math::abs(scale.z) * 0.5f);
    out.set(Math::point(pos), Math::vector(extents));
    return true;
}

//------------------------------------------------------------------------------
/**
    The link a panel is pointing at. The outline is the one the hover gets, so the entity the link
    names reads as the thing the cursor is on, and the line is what makes the relationship visible
    on the canvas. Both ends are the bounds picking uses, so the line lands where the entities are
    drawn rather than at the origins they were authored with. An entity that is not in the canvas
    has no bounds, and then only the end that does is drawn.
*/
void
ToolContext::DrawHighlight(Game::Entity entity, Game::Entity from) const
{
    Scene* scene = this->core->GetScene();

    Math::bbox box;
    if (!WorldBounds(scene, entity, box))
        return;

    Gizmo::DrawBox(box, HoverOutlineColor, 1.0f, OutlineFlags);

    Math::bbox fromBox;
    if (from == Game::Entity::Invalid() || !WorldBounds(scene, from, fromBox))
        return;

    const Math::vec3 fromCenter = fromBox.center().vec;
    const Math::vec3 center = box.center().vec;
    Gizmo::DrawLine(fromCenter, center, HighlightLineColor, 2.0f, Gizmo::DrawFlag_AlwaysOnTop);
}

//------------------------------------------------------------------------------
/**
*/
Shell*
ToolContext::GetShell() const
{
    return this->core;
}

//------------------------------------------------------------------------------
/**
    With no argument the command prints the active tool, which is what makes it something a command
    script can read back.
*/
void
ToolContext::CommandTool(const Util::Array<Util::String>& args)
{
    if (args.IsEmpty())
    {
        this->core->Log(LogMessageType::N_SYSTEM, Util::String("tool is ") + this->ActiveToolName());
        return;
    }

    if (!this->SetActiveToolByName(args[0]))
        this->core->Log(LogMessageType::N_WARNING, "tool is one of: select, translate, rotate, scale");
}

//------------------------------------------------------------------------------
/**
    The frame the gizmo works in. With no argument it cycles, which is what the gizmo key does, and
    the names it takes are the ones the toolbar's dropdown lists, so a frame added to
    FrameOfReference is accepted here as well.
*/
void
ToolContext::CommandGizmo(const Util::Array<Util::String>& args)
{
    if (args.IsEmpty() || args[0] == "toggle")
    {
        int next = this->frame + 1;
        if (next >= NumFrames)
            next = 0;
        this->SetFrame(next);
        return;
    }

    if (args[0] == "list")
    {
        const Util::Array<Gizmo::ComponentGizmo>& gizmos = Gizmo::GetRegisteredGizmos();
        if (gizmos.IsEmpty())
        {
            this->core->Log(LogMessageType::N_SYSTEM, "no component gizmos are registered");
            return;
        }

        Util::String msg;
        for (IndexT i = 0; i < gizmos.Size(); i++)
        {
            if (i > 0)
                msg += "\n";
            msg += Util::String("  ") + gizmos[i].name;
        }
        this->core->Log(LogMessageType::N_SYSTEM, msg);
        return;
    }

    for (IndexT i = 0; i < NumFrames; i++)
    {
        if (args[0] == FrameNames[i])
        {
            this->SetFrame(i);
            return;
        }
    }

    Util::String list = "gizmo is one of: ";
    for (IndexT i = 0; i < NumFrames; i++)
    {
        if (i > 0)
            list += ", ";
        list += FrameNames[i];
    }
    this->core->Log(LogMessageType::N_WARNING, list + ", list");
}

//------------------------------------------------------------------------------
/**
    With no argument the command prints what the grid is set to, which is what makes it something a
    command script can read back. The numbers it prints are the ones the grid is drawn with, and the
    clamps of SetGrid mean what it prints is what the settings became. The color words take the
    components in order and the alpha is optional, so leaving it out keeps whatever transparency the
    color had.
*/
void
ToolContext::CommandGrid(const Util::Array<Util::String>& args)
{
    if (args.IsEmpty())
    {
        Util::String msg;
        msg.Format("grid is %s: %.2f units between the major lines, cut up %d times, %.0f units out from the origin, major (%.2f, %.2f, %.2f, %.2f) minor (%.2f, %.2f, %.2f, %.2f)",
            this->grid.visible ? "on" : "off", this->grid.step, this->grid.subdivisions, this->grid.extent,
            this->grid.majorColor.x, this->grid.majorColor.y, this->grid.majorColor.z, this->grid.majorColor.w,
            this->grid.minorColor.x, this->grid.minorColor.y, this->grid.minorColor.z, this->grid.minorColor.w);
        this->core->Log(LogMessageType::N_SYSTEM, msg);
        return;
    }

    GridSettings grid = this->grid;
    const Util::String& what = args[0];

    if (what == "on")
        grid.visible = true;
    else if (what == "off")
        grid.visible = false;
    else if (what == "toggle")
        grid.visible = !grid.visible;
    else if (what == "step" || what == "subdiv" || what == "size")
    {
        if (args.Size() < 2)
        {
            this->core->Log(LogMessageType::N_WARNING, "grid needs a number after the setting");
            return;
        }

        if (what == "step")
            grid.step = args[1].AsFloat();
        else if (what == "subdiv")
            grid.subdivisions = args[1].AsInt();
        else
            grid.extent = args[1].AsFloat();
    }
    else if (what == "color" || what == "minor")
    {
        if (args.Size() < 4)
        {
            this->core->Log(LogMessageType::N_WARNING, "grid needs r g b after color or minor, alpha is optional");
            return;
        }

        Math::vec4& color = what == "color" ? grid.majorColor : grid.minorColor;
        color.x = args[1].AsFloat();
        color.y = args[2].AsFloat();
        color.z = args[3].AsFloat();
        if (args.Size() > 4)
            color.w = args[4].AsFloat();
    }
    else
    {
        this->core->Log(LogMessageType::N_WARNING, "grid is one of: on, off, toggle, step <n>, subdiv <n>, size <n>, color <r> <g> <b> [a], minor <r> <g> <b> [a]");
        return;
    }

    this->SetGrid(grid);
}

//------------------------------------------------------------------------------
/**
    The snapping is one setting with two sources and one increment per operation, so the command's
    words are those: the sources toggle, the operations are set, and on and off put all the sources
    down or take them all back. What the grid's own lattice is is asked of the grid rather than
    repeated here, so the two settings cannot drift apart in the print.
*/
void
ToolContext::CommandSnap(const Util::Array<Util::String>& args)
{
    if (args.IsEmpty())
    {
        Util::String msg;
        msg.Format("snapping is %s: translate %.2f, rotate %.1f, scale %.2f, on a %.2f grid",
            this->SnapSourceName(), this->snapping.translate, this->snapping.rotate, this->snapping.scale,
            this->GridMinorStep());
        this->core->Log(LogMessageType::N_SYSTEM, msg);
        return;
    }

    if (args[0] == "toggle")
    {
        this->ToggleSnapping();
        return;
    }

    SnapSettings snap = this->snapping;
    const Util::String& what = args[0];

    if (what == "on")
    {
        snap.toGrid = true;
        snap.toIncrements = true;
    }
    else if (what == "off")
    {
        snap.toGrid = false;
        snap.toIncrements = false;
    }
    else if (what == "grid")
        snap.toGrid = !snap.toGrid;
    else if (what == "increments")
        snap.toIncrements = !snap.toIncrements;
    else if (what == "translate" || what == "rotate" || what == "scale")
    {
        if (args.Size() < 2)
        {
            this->core->Log(LogMessageType::N_WARNING, "snap needs a number after the operation");
            return;
        }

        if (what == "translate")
            snap.translate = args[1].AsFloat();
        else if (what == "rotate")
            snap.rotate = args[1].AsFloat();
        else
            snap.scale = args[1].AsFloat();
    }
    else
    {
        this->core->Log(LogMessageType::N_WARNING,
            "snap is one of: on, off, toggle, grid, increments, translate <n>, rotate <n>, scale <n>");
        return;
    }

    this->SetSnapping(snap);
    this->core->SetStatus(Util::String("Snapping: ") + this->SnapSourceName(), 2.0f);
}

} // namespace Editor
