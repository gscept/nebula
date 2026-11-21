#pragma once
//------------------------------------------------------------------------------
/**
    @class Editor::ToolContext

    The viewport tools: the set of them, the one that is active, the frame of reference they work
    in, and the picking and transform plumbing they share. It replaces the tool array the old
    editor's scene window owned.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "neweditor/viewtools/toolinterface.h"
#include "neweditor/viewtools/toolview.h"
#include "game/entity.h"
#include "math/bbox.h"
#include "math/mat4.h"
#include "math/quat.h"
#include "math/vec3.h"
#include "math/vec4.h"
#include "util/array.h"
#include "util/string.h"

namespace Editor
{

class Shell;
class Scene;

enum ToolId
{
    SelectToolId = 0,
    TranslateToolId,
    RotateToolId,
    ScaleToolId,
    NumToolIds,
};

//------------------------------------------------------------------------------
/**
*/
struct GridSettings
{
    bool visible = true;
    float step = 1.0f;
    int subdivisions = 2;
    float extent = 16.0f;
    Math::vec4 majorColor = Math::vec4(0.37f, 0.40f, 0.45f, 1.0f);
    Math::vec4 minorColor = Math::vec4(0.22f, 0.24f, 0.27f, 1.0f);
};

//------------------------------------------------------------------------------
/**
*/
struct SnapSettings
{
    bool toGrid = false;
    bool toIncrements = false;
    float translate = 0.25f;    // world units
    float rotate = 15.0f;       // degrees
    float scale = 0.05f;        // ratio, so 0.05 lands the drag on 1.05, 1.10, and so on
};

class ToolContext : public Core::RefCounted
{
    __DeclareClass(ToolContext)
public:
    /// the core whose active scene the tools act on. Creates the tools and registers their command
    /// and keybinds
    void Setup(Shell* core);
    /// destroy the tool instances
    void Discard();

    /// make a tool the active one, aborting whatever the previous one was in the middle of
    void SetActiveTool(ToolId tool);
    ToolId GetActiveTool() const;
    /// put back what the active tool is in the middle of writing. This is what a tab switch does,
    /// because the preview belonged to the scene that is no longer shown
    void AbortActiveTool();
    /// make a tool the active one by the name the tool command uses
    bool SetActiveToolByName(const Util::String& name);
    const char* ToolName(ToolId id) const;
    const char* ActiveToolName() const;

    /// the basis the gizmo is drawn and dragged in. The toolbar's frame dropdown and the gizmo
    /// command
    /// both set it, and the canvas' status line reads it
    int GetFrame() const;
    void SetFrame(int frame);
    /// the words the dropdown, the canvas' status line and the gizmo command all use for a frame
    const char* FrameName(int frame) const;

    /// run this frame's transform handle widget: it draws on the canvas and reports the drag into
    /// the view, which the tools then read in UpdateView
    void DrawView(ToolView& view);
    /// run the active tool against one frame of canvas input
    void UpdateView(const ToolView& view);

    // -- The transform plumbing the transform tools share --

    /// read the current transforms of a set of entities
    void SnapshotTransforms(const Util::Array<Game::Entity>& entities, Util::Array<TransformEntry>& out) const;
    /// write a transform to the sim world without touching the authored value
    void PreviewTransform(Game::Entity entity, const Math::vec3& position, const Math::quat& orientation,
        const Math::vec3& scale) const;
    /// write transforms through the command manager as one undo entry
    void CommitTransforms(const char* name, const Util::Array<TransformEntry>& entries) const;
    /// put the authored transforms back into the sim world, which is what an aborted preview undoes
    void RestoreTransforms(const Util::Array<TransformEntry>& entries) const;

    /// A transform handle in the three parts the tools work with. The widget is handed the matrix
    /// form
    /// and answers with one, and the parts are what the tools want: it scales a handle by
    /// stretching
    /// each of its axes, which is what going between the two has to agree with. The math library's
    /// trs and decompose do not agree with each other here - trs stretches the world coordinates
    /// instead of the axes - so the handle crosses over by its own pair of functions
    struct Handle
    {
        Math::vec3 pivot = Math::vec3(0);
        Math::quat orientation = Math::quat(0, 0, 0, 1);
        Math::vec3 scale = Math::vec3(1);

        Math::mat4 Matrix() const;
        void SetFromMatrix(const Math::mat4& matrix);
    };

    /// the handle as it stood when the widget took hold of it, which is what a tool's drag is
    /// measured
    /// against: the center a drag turns or scales the selection around, and the basis it does that
    /// in
    const Handle& Grabbed() const;

    /// the grid the canvas draws
    const GridSettings& GetGrid() const;
    void SetGrid(const GridSettings& grid);
    /// the grid button and 'grid toggle'
    void ToggleGrid();

    /// what a drag lands on, and the settings the toolbar's snap popup and the snap command change
    const SnapSettings& GetSnapping() const;
    void SetSnapping(const SnapSettings& snap);
    /// the magnet button and 'snap toggle': snapping is one thing on the bar, so going off takes
    /// both
    /// sources back and going on puts both down
    void ToggleSnapping();
    /// the snap popup and the canvas' status line
    bool SnapsToGrid() const;
    /// the snap popup and the canvas' status line
    bool SnapsToIncrements() const;
    /// what the snapping is set to, in the words the status bar, the canvas' status line and the
    /// snap
    /// command all use. An invert answers with what ctrl held through a drag makes of the settings
    const char* SnapSourceName(bool invert = false) const;

    /// the distance between the grid's minor lines, which is the lattice snapping to the grid puts
    /// the
    /// canvas on
    float GridMinorStep() const;
    /// put a world position on that lattice. This is what a dropped file's placement goes through
    Math::vec3 SnapToGrid(const Math::vec3& pos) const;

    /// put a world position on a lattice given per axis
    static Math::vec3 SnapToLattice(const Math::vec3& pos, const Math::vec3& lattice);

    /// the three values the handle widget's snap reads for a tool: the spacing per axis for
    /// translate,
    /// the turn in degrees for rotate, and the ratio for scale, in x.
    Math::vec3 SnapValues(ToolId tool, bool invert) const;

    /// the entity whose world bounds the mouse ray hits first. Invalid when nothing was hit
    Game::Entity Pick(const ToolView& view) const;
    /// the world bounds an entity can be picked by: the box around what the renderer draws for it
    /// when there is
    /// something, which comes through Editor::Gizmo, and a box at the position and scale it stands
    /// at otherwise.
    /// Read from the sim world the canvas draws
    static bool WorldBounds(Scene* scene, Game::Entity entity, Math::bbox& out);

    /// the entity a panel wants the canvas to show as hovered, and the entity the link to it is
    /// drawn from.
    /// The canvas' own hover comes from the cursor, so this is the second source of the outline: an
    /// entity a
    /// panel is pointing at rather than the one the cursor is over. The shapes are queued for the
    /// frame that
    /// renders the canvas, which is after the panels have drawn, so a panel that calls this during
    /// its draw gets
    /// the outline on the same frame
    void DrawHighlight(Game::Entity entity, Game::Entity from) const;

    Shell* GetShell() const;

private:
    /// console command: tool [select|translate|rotate|scale]
    void CommandTool(const Util::Array<Util::String>& args);
    /// console command: gizmo [world|local] - print, cycle or set the frame the gizmo works in
    void CommandGizmo(const Util::Array<Util::String>& args);
    /// console command: grid [on|off|toggle|step <units>|subdiv <n>|size <units>]
    void CommandGrid(const Util::Array<Util::String>& args);
    /// console command: snap [on|off|toggle|grid|increments|translate <units>|rotate
    /// <degrees>|scale <ratio>]
    void CommandSnap(const Util::Array<Util::String>& args);
    /// the outlines of the selection and the hovered entity
    void DrawOutlines(const ToolView& view);
    /// the grid's lines, queued into the frame that renders the canvas
    void DrawGrid() const;

    /// the point a handle sits at for a set of entities
    static Math::vec3 Pivot(const Util::Array<TransformEntry>& entries);
    /// the basis a handle follows: identity in the world frame, the last entity's orientation in a
    /// frame
    /// that has a basis of its own
    static Math::quat GizmoOrientation(const Util::Array<TransformEntry>& entries, int frame);
    /// where a handle sits when nothing is dragging it: the selection's center, in the selection's
    /// basis
    static Handle SelectionHandle(const Util::Array<TransformEntry>& entries, int frame);

    Shell* core = nullptr;
    ToolInterface* tools[NumToolIds];
    ToolId activeTool = SelectToolId;
    /// the frame the gizmo is drawn and dragged in, which the toolbar's dropdown and the gizmo
    /// command set
    int frame = WorldFrame;
    Game::Entity hovered = Game::Entity::Invalid();

    GridSettings grid;
    SnapSettings snapping;

    // The handle the widget drives. It lives here between frames rather than being rebuilt from the
    // selection every frame, because a widget's rotate only ever reports the turn since the
    // previous
    // frame and adds up through the matrix it is handed. grab is where the handle stood when the
    // widget took hold of it, which is what the drag is measured against
    Handle handle;
    Handle grab;
    bool handleGrabbed = false;
};

} // namespace Editor
