//------------------------------------------------------------------------------
//  @file gizmo.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "neweditor/api/gizmo.h"
#include "neweditor/api/editorfeature.h"
#include "neweditor/api/shell.h"
#include "neweditor/commands/setcomponentcommand.h"
#include "neweditor/model/editcommandmanager.h"
#include "neweditor/model/selectioncontext.h"
#include "neweditor/viewtools/toolcontext.h"
#include "game/component.h"
#include "basegamefeature/components/position.h"
#include "basegamefeature/components/orientation.h"
#include "basegamefeature/components/scale.h"
#include "memdb/attributeregistry.h"
#include "math/line.h"
#include "math/plane.h"
#include "math/scalar.h"
#include "imgui.h"
#include "neweditor/imguizmo/imguizmo.h"

namespace Editor
{
namespace Gizmo
{

// What a render side filled. The editor calls through this and does not know who filled it
static Interface filled;

// The frame the pass is drawing for. The shapes the editor draws itself read it, so they only work
// inside a gizmo
static const GizmoFrame* current;

// The queued changes are run as commands after the frame has drawn
static void ApplyPending();

// The color a handle is drawn with when the caller does not say one
static const Math::vec4 DefaultHandleColor(1.0f, 0.30f, 0.0f, 1.0f);

// A handle is grabbed by the size it is drawn at, but a smaller one is still grabbable
static const float MinGrabSize = 8.0f;

// The guard for a plane that cannot be built: a handle with no axis, or a camera looking straight down
// the axis
static const float PlaneEpsilon = 1e-6f;

//------------------------------------------------------------------------------
/**
*/
void
Fill(const Interface& interface)
{
    filled = interface;
}

//------------------------------------------------------------------------------
/**
*/
void
DrawLine(const Math::vec3& from, const Math::vec3& to, const Math::vec4& color, float width, DrawFlag flags)
{
    if (!filled.line.IsValid())
        return;

    filled.line(from, to, color, width, flags);
}

//------------------------------------------------------------------------------
/**
*/
void
DrawBox(const Math::bbox& box, const Math::vec4& color, float width, DrawFlag flags)
{
    if (!filled.box.IsValid())
        return;

    filled.box(box, color, width, flags);
}

//------------------------------------------------------------------------------
/**
*/
void
DrawBox(const Math::mat4& transform, const Math::vec4& color, float width, DrawFlag flags)
{
    if (!filled.boxTransform.IsValid())
        return;

    filled.boxTransform(transform, color, width, flags);
}

//------------------------------------------------------------------------------
/**
*/
void
DrawPoint(const Math::vec3& position, const Math::vec4& color, float size, DrawFlag flags)
{
    if (!filled.point.IsValid())
        return;

    filled.point(position, color, size, flags);
}

//------------------------------------------------------------------------------
/**
*/
void
DrawCircle(const Math::vec3& center, float radius, const Math::vec3& normal, const Math::vec4& color,
    float width, DrawFlag flags)
{
    if (!filled.circle.IsValid())
        return;

    filled.circle(center, radius, normal, color, width, flags);
}

//------------------------------------------------------------------------------
/**
    A label at a world point. Text is the one shape the editor draws itself, because it is screen
    space. The point is put on the canvas with the camera the canvas is drawn with, which is the
    smaller rect and not the window, and the label goes on the gizmo's draw list. That list is drawn
    after the shapes, so a label is not hidden by what the canvas drew before it.
*/
void
DrawText(const char* text, const Math::vec3& point, const Math::vec4& color)
{
    if (current == nullptr || text == nullptr)
        return;

    Math::vec2 screen;
    if (!current->view->Project(point, screen))
        return;

    // Centered on the point and put above it, so the label reads as the thing it belongs to rather
    // than as something sitting on top of it
    const ImVec2 size = ImGui::CalcTextSize(text);
    const ImVec2 pos(screen.x - size.x * 0.5f, screen.y - size.y - 4.0f);

    ImGuizmo::DrawText(text, pos,
        ImGui::ColorConvertFloat4ToU32(ImVec4(color.x, color.y, color.z, color.w)));
}

//------------------------------------------------------------------------------
/**
*/
bool
GetBounds(Game::World* world, Game::Entity entity, Math::bbox& out)
{
    if (!filled.bounds.IsValid())
        return false;

    return filled.bounds(world, entity, out);
}

//------------------------------------------------------------------------------
// -- The component gizmos --

// What the features registered. The pass calls these in the order they were registered, which is the
// order two gizmos on the same entity are drawn in
static Util::Array<ComponentGizmo> registered;

// Which handle has the drag, where it stood when the press happened, and what the value was then. The
// value itself is not held here, because the component holds it
struct Drag
{
    uint hash;
    Game::Entity entity;
    Game::ComponentId component;
    Math::vec3 grabPoint = Math::vec3(0);
    Math::vec3 grabValue = Math::vec3(0);
    Math::vec3 direction = Math::vec3(0);
    float grabFloat = 0.0f;
    bool isFloat = false;
    bool grabbing = false;
};

// What a handle has changed so far. One entry per entity and component, so a drag that reports on
// every frame is one command
struct PendingChange
{
    Game::Entity entity;
    Game::ComponentId component;
    Util::Blob oldValue;
    Util::Blob newValue;
};

static Drag drag;
static Util::Array<PendingChange> pending;

// The gizmo the active handle belongs to
static Util::String activeName;

//------------------------------------------------------------------------------
/**
    Take the interface back, and the state above with it.

    A pending change is a Blob, and a Blob held when the program ends is freed after Blob::Shutdown()
    has taken the heap away, which asserts. This is the last point where the heap is still there.
*/
void
Clear()
{
    filled = Interface();

    pending.Clear();
    drag = Drag();
    activeName.Clear();
}

//------------------------------------------------------------------------------
/**
*/
HandleStyle
DefaultHandleStyle()
{
    HandleStyle style;
    style.color = DefaultHandleColor;
    style.size = 8.0f;
    style.flags = DrawFlag_AlwaysOnTop;
    style.tip = nullptr;
    return style;
}

//------------------------------------------------------------------------------
/**
*/
static uint
HashId(const char* id)
{
    uint hash = 2166136261u;
    if (id == nullptr)
        return hash;

    for (const char* c = id; *c != 0; c++)
    {
        hash ^= (uint)(unsigned char)*c;
        hash *= 16777619u;
    }
    return hash;
}

//------------------------------------------------------------------------------
/**
    Which handle this frame's press belongs to. The handle under the cursor on the press frame takes the
    drag and keeps it until the release, so the cursor does not have to stay on it while it moves.

    Only a selected entity's handles take a press.

    Returns 0 when this is not the handle the drag is on, 1 when it took this frame's press, and 2 when
    it has held it since an earlier frame. The caller needs the difference, because the drag measures
    itself against the value the handle had when it was taken.
*/
static int
ClaimHandle(const char* id, const Math::vec3& handlePos, const Math::vec3& direction, const HandleStyle& style)
{
    const uint hash = HashId(id);

    if (drag.grabbing)
        return hash == drag.hash && drag.entity == current->entity ? 2 : 0;

    Math::vec2 window;
    if (!current->view->Project(handlePos, window))
        return 0;

    const float radius = style.size > MinGrabSize ? style.size : MinGrabSize;
    const Math::vec2 delta(current->view->mouse.x - window.x, current->view->mouse.y - window.y);
    if (delta.length() > radius)
        return 0;

    // The tip is drawn as a canvas label. An ImGui tooltip only shows for the widget it is called
    // inside, and the canvas is not an ImGui item
    if (style.tip != nullptr)
        DrawText(style.tip, handlePos, style.color);

    if (!current->selected || !current->view->pressed)
        return 0;

    drag.hash = hash;
    drag.entity = current->entity;
    drag.component = current->component;
    drag.grabPoint = handlePos;
    drag.direction = direction;
    drag.grabbing = true;
    activeName = current->name;

    return 1;
}

//------------------------------------------------------------------------------
/**
    The point the cursor is on the drag plane. The plane contains the handle's axis and faces the
    camera. A handle with no axis drags on the plane facing the camera.
*/
static bool
DragPoint(Math::vec3& out)
{
    Math::line ray;
    if (!current->view->CanvasRay(current->view->mouse, ray))
        return false;

    const Math::vec3 viewDir = Math::normalize(Math::vec3(ray.m.x, ray.m.y, ray.m.z));

    Math::vec3 normal;
    if (Math::length(drag.direction) < PlaneEpsilon)
    {
        normal = viewDir;
    }
    else
    {
        const Math::vec3 dir = Math::normalize(drag.direction);
        const float along = Math::dot(viewDir, dir);
        normal = Math::vec3(viewDir.x - dir.x * along, viewDir.y - dir.y * along, viewDir.z - dir.z * along);
        if (Math::length(normal) < PlaneEpsilon)
            return false;   // the camera looks straight down the axis, and there is no plane to read on
        normal = Math::normalize(normal);
    }

    Math::plane plane(Math::point(drag.grabPoint), Math::vector(normal));

    Math::point hit;
    if (!plane.intersect(ray, hit))
        return false;

    out = Math::vec3(hit.x, hit.y, hit.z);
    return true;
}

//------------------------------------------------------------------------------
/**
    The snap increment for the axis the handle drives along.
*/
static float
SnapForAxis(const Math::vec3& direction)
{
    if (Math::length(direction) < PlaneEpsilon)
        return 0.0f;

    int axis = 0;
    float best = Math::abs(direction.x);
    if (Math::abs(direction.y) > best)
    {
        axis = 1;
        best = Math::abs(direction.y);
    }
    if (Math::abs(direction.z) > best)
        axis = 2;

    return current->snap[axis];
}

//------------------------------------------------------------------------------
/**
*/
static float
SnapValue(float value, float increment)
{
    if (increment <= 0.0f)
        return value;

    return Math::round(value / increment) * increment;
}

//------------------------------------------------------------------------------
/**
    The handle on the canvas, at the position the value it answers with puts it. Drawn for every handle
    the gizmo asks about, not only the one with the drag.
*/
static void
DrawHandle(const Math::vec3& pos, const HandleStyle& style)
{
    DrawPoint(pos, style.color, style.size, (DrawFlag)style.flags);
}

//------------------------------------------------------------------------------
/**
*/
bool
HandleFloat(const char* id, float value, float* out, const Math::vec3& handlePos,
    const Math::vec3& direction, const HandleStyle& style)
{
    if (current == nullptr || out == nullptr)
        return false;

    // The number is the distance along direction. The direction is taken as it is given, because the
    // caller put the handle at direction * value
    const float units = Math::length(direction);
    if (units < PlaneEpsilon)
        return false;

    const Math::vec3 dir = direction / units;

    const int claim = ClaimHandle(id, handlePos, direction, style);

    float resolved = value;
    if (claim != 0)
    {
        if (claim == 1)
        {
            drag.isFloat = true;
            drag.grabFloat = value;
        }

        Math::vec3 moved;
        if (DragPoint(moved))
        {
            const float along = Math::dot(Math::vec3(moved.x - drag.grabPoint.x, moved.y - drag.grabPoint.y,
                moved.z - drag.grabPoint.z), dir);

            // The cursor's world distance comes back through the units the direction carries
            resolved = SnapValue(drag.grabFloat + along / units, SnapForAxis(dir));
        }
    }

    DrawHandle(handlePos + direction * (resolved - value), style);

    if (claim == 0 || resolved == value)
        return false;

    *out = resolved;
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
HandleVec3(const char* id, const Math::vec3& value, Math::vec3* out, const Math::vec3& handlePos,
    const Math::vec3& direction, const HandleStyle& style)
{
    if (current == nullptr || out == nullptr)
        return false;

    const int claim = ClaimHandle(id, handlePos, direction, style);

    Math::vec3 resolved = value;
    if (claim != 0 && !drag.isFloat)
    {
        if (claim == 1)
        {
            drag.isFloat = false;
            drag.grabValue = value;
        }

        Math::vec3 moved;
        if (DragPoint(moved))
        {
            for (int i = 0; i < 3; i++)
            {
                const float movedValue = drag.grabValue[i] + (moved[i] - drag.grabPoint[i]);
                const float increment = current->snap[i];
                resolved[i] = increment > 0.0f ? SnapValue(movedValue, increment) : movedValue;
            }
        }
    }

    DrawHandle(handlePos + (resolved - value), style);

    if (claim == 0 || resolved == value)
        return false;

    *out = resolved;
    return true;
}

//------------------------------------------------------------------------------
/**
    The value a handle changed. Written into the sim world while the drag runs, and put through the
    command manager as one entry when the drag ends.
*/
void
CommitComponent(Scene* scene, Game::Entity entity, Game::ComponentId component, const void* bytes, SizeT size)
{
    if (scene == nullptr || bytes == nullptr || size == 0)
        return;

    Shell* core = EditorFeatureRegistry::GetShell();
    if (core == nullptr)
        return;

    const void* authored = scene->GetComponentData(entity, component);
    if (authored == nullptr)
        return;

    IndexT entry = InvalidIndex;
    for (IndexT i = 0; i < pending.Size(); i++)
    {
        if (pending[i].entity == entity && pending[i].component == component)
        {
            entry = i;
            break;
        }
    }

    if (entry == InvalidIndex)
    {
        PendingChange change;
        change.entity = entity;
        change.component = component;
        change.oldValue.Set(authored, size);
        change.newValue.Set(bytes, size);
        pending.Append(change);
        entry = pending.Size() - 1;

        // Queued once per frame, because the later reports only change what this one already queued
        core->QueueAction(Util::Delegate<void()>::FromFunction<ApplyPending>());
    }
    else
    {
        pending[entry].newValue.Set(bytes, size);
    }

    // The preview is what the canvas shows while the drag runs
    scene->PreviewComponent(entity, component, bytes);
}

//------------------------------------------------------------------------------
/**
    Run the queued changes as commands. A drag is not an edit until it ends, so a change waits for the
    release and the action goes back on the queue for the next frame.
*/
void
ApplyPending()
{
    Shell* core = EditorFeatureRegistry::GetShell();
    if (core == nullptr || pending.IsEmpty())
        return;

    if (drag.grabbing)
    {
        core->QueueAction(Util::Delegate<void()>::FromFunction<ApplyPending>());
        return;
    }

    Ptr<Scene> scene = core->GetScene();
    if (scene == nullptr)
        return;

    Ptr<EditCommandManager> manager = core->GetCommandManager();

    for (IndexT i = 0; i < pending.Size(); i++)
    {
        const PendingChange& change = pending[i];
        if (!scene->HasInstance(change.entity))
            continue;

        if (change.oldValue == change.newValue)
        {
            // A press on a handle that never moved it. The preview is put back and nothing goes on
            // the stack
            scene->RestoreComponent(change.entity, change.component);
            continue;
        }

        Ptr<SetComponentCommand> command = SetComponentCommand::Create();
        command->scene = scene;
        command->entity = change.entity;
        command->component = change.component;
        command->newValue = change.newValue;
        manager->Execute(command);
    }

    pending.Clear();
    activeName = "";
}

//------------------------------------------------------------------------------
/**
    The world transform the canvas draws the entity at. The sim world is the one the canvas shows.
*/
static Math::mat4
WorldTransform(Scene* scene, Game::Entity entity)
{
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

    return Math::trs(pos, orient, scale);
}

//------------------------------------------------------------------------------
/**
*/
static const ComponentGizmo*
FindGizmo(Game::ComponentId component)
{
    for (IndexT i = 0; i < registered.Size(); i++)
    {
        if (registered[i].component == component)
            return &registered[i];
    }
    return nullptr;
}

//------------------------------------------------------------------------------
/**
    The word the status line and the 'gizmo list' command use for a gizmo. A feature that names its
    gizmo is read as it named it; one that does not is read as the component it was registered for.
*/
static Util::String
ComponentName(Game::ComponentId component)
{
    MemDb::Attribute* attr = MemDb::AttributeRegistry::GetAttribute(component);
    if (attr == nullptr)
        return Util::String();

    return Util::String(((Game::ComponentInterface*)attr)->GetName());
}

//------------------------------------------------------------------------------
/**
*/
void
RegisterComponentGizmo(ComponentGizmo gizmo)
{
    if (!gizmo.draw.IsValid())
        return;

    if (gizmo.name.IsEmpty())
        gizmo.name = ComponentName(gizmo.component);

    for (IndexT i = 0; i < registered.Size(); i++)
    {
        if (registered[i].component == gizmo.component)
        {
            registered[i] = gizmo;
            return;
        }
    }

    registered.Append(gizmo);
}

//------------------------------------------------------------------------------
/**
*/
void
UnregisterComponentGizmo(Game::ComponentId component)
{
    for (IndexT i = 0; i < registered.Size(); i++)
    {
        if (registered[i].component == component)
        {
            registered.EraseIndex(i);
            break;
        }
    }

    if (drag.component == component)
    {
        drag.grabbing = false;
        activeName = "";
    }
}

//------------------------------------------------------------------------------
/**
*/
void
ClearComponentGizmos()
{
    registered.Clear();
    activeName = "";
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<ComponentGizmo>&
GetRegisteredGizmos()
{
    return registered;
}

//------------------------------------------------------------------------------
/**
    Run this frame's pass over the selection. The pass is the only place the frame is valid.

    Returns true when a handle took this frame's press, which keeps the transform handle from taking the
    same press.
*/
bool
DrawComponentGizmos(const ToolView& view)
{
    if (!view.IsValid() || registered.IsEmpty())
        return false;

    Shell* core = EditorFeatureRegistry::GetShell();
    if (core == nullptr)
        return false;

    Ptr<Scene> scene = core->GetScene();
    if (scene == nullptr)
        return false;

    const Util::Array<Game::Entity>& entities = core->GetSelection()->GetSelection();
    if (entities.IsEmpty() && !drag.grabbing)
        return false;

    // The snap values a handle uses. A gizmo handle is not one of the four transform tools, so these
    // are the plain ones: the grid or the translate increment
    const bool invert = ImGui::GetIO().KeyCtrl;
    const Math::vec3 snap = core->GetTools()->SnapValues(SelectToolId, invert);

    GizmoFrame frame;
    frame.scene = scene;
    frame.view = &view;
    frame.selected = true;
    frame.snap = snap;

    for (IndexT i = 0; i < entities.Size(); i++)
    {
        const Util::Array<Game::ComponentId>& components = scene->GetComponents(entities[i]);
        if (components.IsEmpty())
            continue;

        // The transform is the entity's, so it is resolved once for the entity rather than once for
        // every component it has
        frame.entity = entities[i];
        frame.transform = WorldTransform(scene, entities[i]);

        for (IndexT c = 0; c < components.Size(); c++)
        {
            const ComponentGizmo* gizmo = FindGizmo(components[c]);
            if (gizmo == nullptr)
                continue;

            frame.component = components[c];
            frame.name = gizmo->name;
            frame.value = scene->GetComponentData(entities[i], components[c]);

            current = &frame;
            gizmo->draw(frame);
        }
    }

    current = nullptr;

    // The drag ends on the frame the button came up, or the frame escape put it away. The change is
    // run as a command after the frame has drawn.
    //
    // The end frame is reported as claimed too, because the release belongs to the handle. A tool
    // reading the same release would take it as its own click
    const bool ended = drag.grabbing && (view.released || view.escape);
    if (ended)
    {
        drag.grabbing = false;
        drag.isFloat = false;
    }

    return drag.grabbing || ended;
}

//------------------------------------------------------------------------------
/**
*/
bool
IsDragging()
{
    return drag.grabbing;
}

//------------------------------------------------------------------------------
/**
*/
const Util::String&
ActiveGizmoName()
{
    return activeName;
}

//------------------------------------------------------------------------------
/**
    Put the drag back without committing it. A handle cannot carry its grab into another frame's
    selection, which is what a tool switch or a scene switch does.
*/
void
AbortDrag()
{
    if (!drag.grabbing)
        return;

    drag.grabbing = false;
    drag.isFloat = false;
    activeName = "";

    Shell* core = EditorFeatureRegistry::GetShell();
    if (core == nullptr || pending.IsEmpty())
        return;

    Ptr<Scene> scene = core->GetScene();
    if (scene == nullptr)
        return;

    for (IndexT i = 0; i < pending.Size(); i++)
        scene->RestoreComponent(pending[i].entity, pending[i].component);
}

//------------------------------------------------------------------------------
// The same thing as the managed side calls it. These are thin wrappers over the C++ shapes above, and
// they read the frame the pass is drawing for.

void
GizmoLine(const float* from, const float* to, const float* color, float width, int flags)
{
    if (from == nullptr || to == nullptr || color == nullptr)
        return;

    DrawLine(Math::vec3(from[0], from[1], from[2]), Math::vec3(to[0], to[1], to[2]),
        Math::vec4(color[0], color[1], color[2], color[3]), width, (DrawFlag)flags);
}

void
GizmoBox(const float* min, const float* max, const float* color, float width, int flags)
{
    if (min == nullptr || max == nullptr || color == nullptr)
        return;

    Math::bbox box;
    box.set(Math::point((min[0] + max[0]) * 0.5f, (min[1] + max[1]) * 0.5f, (min[2] + max[2]) * 0.5f),
        Math::vector((max[0] - min[0]) * 0.5f, (max[1] - min[1]) * 0.5f, (max[2] - min[2]) * 0.5f));

    DrawBox(box, Math::vec4(color[0], color[1], color[2], color[3]), width, (DrawFlag)flags);
}

void
GizmoBoxTransform(const float* transform, const float* color, float width, int flags)
{
    if (transform == nullptr || color == nullptr)
        return;

    Math::mat4 m;
    for (int i = 0; i < 16; i++)
        ((float*)&m)[i] = transform[i];

    DrawBox(m, Math::vec4(color[0], color[1], color[2], color[3]), width, (DrawFlag)flags);
}

void
GizmoPoint(const float* position, const float* color, float size, int flags)
{
    if (position == nullptr || color == nullptr)
        return;

    DrawPoint(Math::vec3(position[0], position[1], position[2]),
        Math::vec4(color[0], color[1], color[2], color[3]), size, (DrawFlag)flags);
}

void
GizmoCircle(const float* center, float radius, const float* normal, const float* color, float width, int flags)
{
    if (center == nullptr || normal == nullptr || color == nullptr)
        return;

    DrawCircle(Math::vec3(center[0], center[1], center[2]), radius,
        Math::vec3(normal[0], normal[1], normal[2]),
        Math::vec4(color[0], color[1], color[2], color[3]), width, (DrawFlag)flags);
}

void
GizmoText(const char* text, const float* point, const float* color)
{
    if (point == nullptr || color == nullptr)
        return;

    DrawText(text, Math::vec3(point[0], point[1], point[2]),
        Math::vec4(color[0], color[1], color[2], color[3]));
}

bool
GizmoHandleFloat(const char* id, float value, float* out, const float* handlePos, const float* direction,
    const float* color, float size, int flags, const char* tip)
{
    if (handlePos == nullptr || direction == nullptr || color == nullptr)
        return false;

    HandleStyle style;
    style.color = Math::vec4(color[0], color[1], color[2], color[3]);
    style.size = size;
    style.flags = flags;
    style.tip = tip;

    return HandleFloat(id, value, out, Math::vec3(handlePos[0], handlePos[1], handlePos[2]),
        Math::vec3(direction[0], direction[1], direction[2]), style);
}

bool
GizmoHandleVec3(const char* id, const float* value, float* out, const float* handlePos, const float* direction,
    const float* color, float size, int flags, const char* tip)
{
    if (value == nullptr || out == nullptr || handlePos == nullptr || direction == nullptr || color == nullptr)
        return false;

    HandleStyle style;
    style.color = Math::vec4(color[0], color[1], color[2], color[3]);
    style.size = size;
    style.flags = flags;
    style.tip = tip;

    Math::vec3 result;
    if (!HandleVec3(id, Math::vec3(value[0], value[1], value[2]), &result,
        Math::vec3(handlePos[0], handlePos[1], handlePos[2]),
        Math::vec3(direction[0], direction[1], direction[2]), style))
        return false;

    out[0] = result[0];
    out[1] = result[1];
    out[2] = result[2];
    return true;
}

unsigned long long
GizmoEntityId()
{
    if (current == nullptr)
        return 0xFFFFFFFFFFFFFFFFULL;

    return (unsigned long long)static_cast<Ids::Id64>(current->entity);
}

unsigned int
GizmoComponentId()
{
    if (current == nullptr)
        return 0;

    return (unsigned int)current->component.HashCode();
}

bool
GizmoIsSelected()
{
    return current != nullptr && current->selected;
}

void
GizmoTransform(float* out16)
{
    if (current == nullptr || out16 == nullptr)
        return;

    for (int i = 0; i < 16; i++)
        out16[i] = ((const float*)&current->transform)[i];
}

bool
GizmoProject(const float* world, float* outWindow)
{
    if (current == nullptr || world == nullptr || outWindow == nullptr)
        return false;

    Math::vec2 window;
    if (!current->view->Project(Math::vec3(world[0], world[1], world[2]), window))
        return false;

    outWindow[0] = window.x;
    outWindow[1] = window.y;
    return true;
}

float
GizmoSnapAxis(int axis)
{
    if (current == nullptr || axis < 0 || axis > 2)
        return 0.0f;

    return current->snap[axis];
}

} // namespace Gizmo

} // namespace Editor
//------------------------------------------------------------------------------
