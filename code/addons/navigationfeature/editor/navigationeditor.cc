//------------------------------------------------------------------------------
//  @file navigationeditor.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "navigationfeature/editor/navigationeditor.h"

#if WITH_NEBULA_EDITOR
#include "navigationfeature/editor/navigationpanel.h"
#include "navigationfeature/editor/navmeshdocument.h"
#include "navigationfeature/editor/navmeshsources.h"
#include "navigationfeature/components/navigation.h"
#include "navigationfeature/navigationfeatureunit.h"
#include "navigationfeature/streamnavmeshcache.h"
#include "navbuild/navareas.h"
#include "navbuild/navmeshbuilder.h"
#include "navbuild/geometrysources.h"
#include "editorassets/asseteditorpanel.h"
#include "editorassets/assettype.h"
#include "neweditor/api/editorfeature.h"
#include "neweditor/api/shell.h"
#include "neweditor/base/iconregistry.h"
#include "neweditor/commands/addcomponentcommand.h"
#include "neweditor/commands/createentitycommand.h"
#include "neweditor/commands/setcomponentcommand.h"
#include "neweditor/model/editcommandmanager.h"
#include "neweditor/model/scene.h"
#include "neweditor/model/selectioncontext.h"
#include "neweditor/viewtools/toolview.h"
#include "basegamefeature/components/position.h"
#include "basegamefeature/components/orientation.h"
#include "basegamefeature/components/scale.h"
#include "game/componentinspection.h"
#include "dynui/im3d/im3dcontext.h"
#include "DetourNavMesh.h"
#include "imgui.h"

namespace NavigationFeature
{

namespace
{

NavigationDisplay display;

//------------------------------------------------------------------------------
/**
    The outline being edited. The points are a copy while one is dragged, the drag is committed as
    one change when it lets go
*/
struct OutlineEdit
{
    Game::Entity target = Game::Entity::Invalid();
    /// the scene the target is in, an edit ends when another scene is shown
    Editor::Scene* scene = nullptr;
    Util::Array<Math::vec3> localPoints;
    Util::Array<Math::vec3> worldPoints;
    Math::mat4 frame;
    IndexT hovered = InvalidIndex;
    IndexT dragged = InvalidIndex;
    IndexT highlighted = InvalidIndex;
    bool hasSurfacePoint = false;
    Math::vec3 surfacePoint;
} outline;

//------------------------------------------------------------------------------
/**
*/
struct PathTest
{
    bool active = false;
    Navigation::NavMeshId navMesh = Navigation::InvalidNavMeshId;
    QueryFilter filter;
    bool hasStart = false;
    bool hasEnd = false;
    Math::point start;
    Math::point end;
    NavPath path;
    /// the mouse on the navmesh, drawn as a marker
    bool hasHover = false;
    Math::point hover;
} pathTest;

//------------------------------------------------------------------------------
/**
*/
Math::vec3
ToVec3(const Math::point& p)
{
    return Math::vec3(p.x, p.y, p.z);
}

//------------------------------------------------------------------------------
/**
*/
Editor::Scene*
ActiveScene()
{
    Editor::Shell* shell = Editor::EditorFeatureRegistry::GetShell();
    return shell != nullptr ? shell->GetScene().get() : nullptr;
}

//------------------------------------------------------------------------------
/**
    The navmesh is checked every frame, it goes away when its document closes or its entity is
    deleted
*/
bool
NavMeshLoaded(Navigation::NavMeshId navMesh)
{
    Navigation::StreamNavMeshCache* cache = GetNavMeshCache();
    if (navMesh == Navigation::InvalidNavMeshId || cache == nullptr)
        return false;
    for (Navigation::NavMeshId loaded : cache->GetLoadedMeshes())
    {
        if (loaded == navMesh)
            return cache->GetDetourMesh(navMesh) != nullptr;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
    The outline is written through the scene's command manager, one undo step per change
*/
void
CommitOutline(Editor::Scene* scene, Game::Entity entity, const Util::Array<Math::vec3>& localPoints)
{
    const Game::ComponentId component = Game::GetComponentId<NavigationFeature::NavArea>();
    const void* data = scene->GetComponentData(entity, component);
    if (data == nullptr)
        return;
    NavigationFeature::NavArea navArea = *(const NavigationFeature::NavArea*)data;
    navArea.pointCount = (uint)Math::min((SizeT)localPoints.Size(), (SizeT)navArea.points.size());
    for (uint i = 0; i < navArea.pointCount; i++)
        navArea.points[i] = localPoints[i];

    Ptr<Editor::SetComponentCommand> command = Editor::SetComponentCommand::Create();
    command->scene = scene;
    command->entity = entity;
    command->component = component;
    command->newValue = Util::Blob(&navArea, sizeof(navArea));
    scene->GetCommandManager()->Execute(command);
}

//------------------------------------------------------------------------------
/**
    A volume without an outline gets the footprint of its box, at the height of its center
*/
void
EnsureOutline(Editor::Scene* scene, Game::Entity entity)
{
    const void* data = scene->GetComponentData(entity, Game::GetComponentId<NavigationFeature::NavArea>());
    if (data == nullptr)
        return;
    const NavigationFeature::NavArea navArea = *(const NavigationFeature::NavArea*)data;
    if (navArea.pointCount >= 3)
        return;

    Game::World* world = scene->GetAuthoringWorld();
    const Math::vec3 position = world->GetComponent<Game::Position>(entity);
    const Math::quat orientation = world->GetComponent<Game::Orientation>(entity);
    const Math::mat4 toLocal = Math::inverse(Math::trs(position, orientation, Math::vec3(1.0f)));
    Math::bbox localBox;
    const Math::vec3 extents(Math::abs(navArea.extents.x), Math::abs(navArea.extents.y), Math::abs(navArea.extents.z));
    localBox.pmin = Math::point(-extents.x, -extents.y, -extents.z);
    localBox.pmax = Math::point(extents.x, extents.y, extents.z);
    const Math::mat4 boxTransform = Math::trs(position, orientation, world->GetComponent<Game::Scale>(entity));
    Util::Array<Math::vec3> points;
    for (const Math::vec3& p : Navigation::Build::BoxOutlineXZ(boxTransform, localBox))
    {
        const Math::vec4 l = toLocal * Math::vec4(p.x, position.y, p.z, 1.0f);
        points.Append(Math::vec3(l.x, l.y, l.z));
    }
    CommitOutline(scene, entity, points);
}

//------------------------------------------------------------------------------
/**
    Everything that doesn't need the mouse. The edit belongs to one selected NavArea in the scene
    shown, and ends when that changes. Returns false when it ended
*/
bool
RefreshOutline()
{
    Editor::Shell* shell = Editor::EditorFeatureRegistry::GetShell();
    Editor::Scene* scene = ActiveScene();
    const Game::ComponentId component = Game::GetComponentId<NavigationFeature::NavArea>();
    bool valid = outline.target != Game::Entity::Invalid() && scene != nullptr && scene == outline.scene &&
        scene->HasInstance(outline.target) && scene->HasComponent(outline.target, component);
    if (valid)
    {
        const Util::Array<Game::Entity>& selection = shell->GetSelection()->GetSelection();
        valid = selection.Size() == 1 && selection[0] == outline.target;
    }
    if (!valid)
    {
        EndOutlineEdit();
        return false;
    }

    // an undo can take the outline away while it is edited
    EnsureOutline(scene, outline.target);

    Game::World* world = scene->GetAuthoringWorld();
    outline.frame = Math::trs(world->GetComponent<Game::Position>(outline.target), world->GetComponent<Game::Orientation>(outline.target), Math::vec3(1.0f));
    const NavigationFeature::NavArea& navArea = *(const NavigationFeature::NavArea*)scene->GetComponentData(outline.target, component);
    if (outline.dragged == InvalidIndex)
    {
        outline.localPoints.Clear();
        for (uint i = 0; i < navArea.pointCount; i++)
            outline.localPoints.Append(navArea.points[i]);
    }
    outline.worldPoints.Clear();
    for (const Math::vec3& p : outline.localPoints)
    {
        const Math::vec4 w = outline.frame * Math::vec4(p.x, p.y, p.z, 1.0f);
        outline.worldPoints.Append(Math::vec3(w.x, w.y, w.z));
    }
    return true;
}

//------------------------------------------------------------------------------
/**
    Where the ray meets the surface: the loaded navmeshes, or a level plane at planeY
*/
bool
SurfacePoint(const Math::line& ray, float planeY, Math::vec3& point)
{
    float t;
    if (RaycastNavMeshes(ray.start(), ray.vec(), t))
    {
        point = ToVec3(ray.start() + ray.vec() * t);
        return true;
    }
    const Math::vector dir = ray.vec();
    if (Math::abs(dir.y) < 1e-6f)
        return false;
    t = (planeY - ray.start().y) / dir.y;
    if (t < 0.0f || t > 1.0f)
        return false;
    point = ToVec3(ray.start() + dir * t);
    return true;
}

//------------------------------------------------------------------------------
/**
    The outline point closest to the ray, within a radius that grows with the distance
*/
IndexT
PickPoint(const Math::line& ray, const Util::Array<Math::vec3>& points)
{
    const Math::vec3 origin = ToVec3(ray.start());
    const Math::vec3 dir = Math::normalize(Math::vec3(ray.vec().x, ray.vec().y, ray.vec().z));
    IndexT best = InvalidIndex;
    float bestDistance = FLT_MAX;
    for (IndexT i = 0; i < points.Size(); i++)
    {
        const Math::vec3 toPoint = points[i] - origin;
        const float along = Math::dot(toPoint, dir);
        if (along <= 0.0f)
            continue;
        const float distance = Math::length(toPoint - dir * along);
        // about the size of the drawn dot on screen
        const float radius = Math::max(0.1f, along * 0.015f);
        if (distance < radius && distance < bestDistance)
        {
            bestDistance = distance;
            best = i;
        }
    }
    return best;
}

//------------------------------------------------------------------------------
/**
    The outline's part of a frame. It takes every click over the canvas while it runs, so a click
    on the surface adds a point instead of selecting what is under it
*/
bool
OutlineInput(const Editor::ToolView& view)
{
    if (view.escape)
    {
        EndOutlineEdit();
        return false;
    }
    if (!RefreshOutline())
        return false;
    Editor::Scene* scene = outline.scene;

    const Math::vec4 framePosition = outline.frame.position();
    const Math::mat4 toLocal = Math::inverse(outline.frame);
    auto toLocalPoint = [&toLocal](const Math::vec3& p)
    {
        const Math::vec4 l = toLocal * Math::vec4(p.x, p.y, p.z, 1.0f);
        return Math::vec3(l.x, l.y, l.z);
    };

    Math::line ray;
    const bool hasRay = view.hovered && view.CanvasRay(view.mouse, ray);
    if (hasRay)
    {
        // a dragged point stays at its height where the navmeshes are missed
        const float planeY = outline.dragged != InvalidIndex ? outline.worldPoints[outline.dragged].y : framePosition.y;
        outline.hasSurfacePoint = SurfacePoint(ray, planeY, outline.surfacePoint);
    }
    else
    {
        outline.hasSurfacePoint = false;
    }

    if (outline.dragged != InvalidIndex)
    {
        if (view.held)
        {
            if (outline.hasSurfacePoint)
                outline.localPoints[outline.dragged] = toLocalPoint(outline.surfacePoint);
        }
        else
        {
            // one undo step for the whole drag
            outline.dragged = InvalidIndex;
            CommitOutline(scene, outline.target, outline.localPoints);
        }
        return true;
    }

    outline.hovered = hasRay ? PickPoint(ray, outline.worldPoints) : InvalidIndex;
    if (!view.hovered)
        return false;

    const Game::ComponentId component = Game::GetComponentId<NavigationFeature::NavArea>();
    const SizeT capacity = ((const NavigationFeature::NavArea*)scene->GetComponentData(outline.target, component))->points.size();
    const bool removeHovered = outline.hovered != InvalidIndex &&
        (ImGui::IsKeyPressed(ImGuiKey_Delete, false) || (view.ctrl && view.pressed));
    if (removeHovered)
    {
        // an outline needs three points, Reset to box in the inspector removes it
        if (outline.localPoints.Size() > 3)
        {
            outline.localPoints.EraseIndex(outline.hovered);
            outline.hovered = InvalidIndex;
            CommitOutline(scene, outline.target, outline.localPoints);
        }
        return true;
    }

    if (view.pressed)
    {
        if (outline.hovered != InvalidIndex)
        {
            outline.dragged = outline.hovered;
        }
        else if (outline.hasSurfacePoint)
        {
            if (outline.localPoints.Size() >= capacity)
            {
                n_warning("A navigation area outline takes %d points at most\n", (int)capacity);
                return true;
            }
            const IndexT edge = Navigation::Build::OutlineInsertIndex(outline.worldPoints.Begin(), outline.worldPoints.Size(), outline.surfacePoint);
            outline.localPoints.Insert(edge + 1, toLocalPoint(outline.surfacePoint));
            CommitOutline(scene, outline.target, outline.localPoints);
        }
    }
    return true;
}

//------------------------------------------------------------------------------
/**
    The points, the one under the mouse, and where a click would put a new one
*/
void
DrawOutline()
{
    if (outline.target == Game::Entity::Invalid() || outline.worldPoints.IsEmpty())
        return;

    const Math::vec4 white(1.0f, 1.0f, 1.0f, 1.0f);
    const Math::vec4 highlight(1.0f, 0.85f, 0.2f, 1.0f);

    // while dragging, the outline of the drag; the committed one is drawn with the volumes
    if (outline.dragged != InvalidIndex)
    {
        for (IndexT i = 0; i < outline.worldPoints.Size(); i++)
        {
            const Math::vec3& a = outline.worldPoints[i];
            const Math::vec3& b = outline.worldPoints[(i + 1) % outline.worldPoints.Size()];
            Im3d::Im3dContext::DrawLine(Math::line(Math::point(a.x, a.y, a.z), Math::point(b.x, b.y, b.z)), 2.0f, white);
        }
    }
    for (IndexT i = 0; i < outline.worldPoints.Size(); i++)
    {
        const bool active = i == outline.hovered || i == outline.dragged || i == outline.highlighted;
        Im3d::Im3dContext::DrawPoint(outline.worldPoints[i], active ? 16.0f : 10.0f, active ? highlight : white);
    }

    // the edge a click would put the new point into
    if (outline.dragged == InvalidIndex && outline.hovered == InvalidIndex && outline.hasSurfacePoint)
    {
        const IndexT edge = Navigation::Build::OutlineInsertIndex(outline.worldPoints.Begin(), outline.worldPoints.Size(), outline.surfacePoint);
        const Math::vec3& a = outline.worldPoints[edge];
        const Math::vec3& b = outline.worldPoints[(edge + 1) % outline.worldPoints.Size()];
        const Math::point p(outline.surfacePoint.x, outline.surfacePoint.y, outline.surfacePoint.z);
        const Math::vec4 preview(1.0f, 1.0f, 1.0f, 0.5f);
        Im3d::Im3dContext::DrawLine(Math::line(Math::point(a.x, a.y, a.z), p), 1.0f, preview);
        Im3d::Im3dContext::DrawLine(Math::line(p, Math::point(b.x, b.y, b.z)), 1.0f, preview);
        Im3d::Im3dContext::DrawPoint(outline.surfacePoint, 10.0f, preview);
    }
}

//------------------------------------------------------------------------------
/**
    The path test's part of a frame. It takes the clicks over the canvas while it runs
*/
bool
PathTestInput(const Editor::ToolView& view)
{
    if (view.escape || !NavMeshLoaded(pathTest.navMesh))
    {
        EndPathTest();
        return false;
    }

    Math::line ray;
    pathTest.hasHover = false;
    Navigation::StreamNavMeshCache* cache = GetNavMeshCache();
    const dtNavMesh* mesh = cache->GetDetourMesh(pathTest.navMesh);
    float t;
    if (view.hovered && view.CanvasRay(view.mouse, ray) && mesh != nullptr && Navigation::Build::RaycastNavMesh(*mesh, ray.start(), ray.vec(), t))
    {
        pathTest.hover = ray.start() + ray.vec() * t;
        pathTest.hasHover = true;
    }

    if (view.pressed && pathTest.hasHover)
        SetPathTestPoint(view.shift, pathTest.hover);

    return view.hovered;
}

//------------------------------------------------------------------------------
/**
*/
void
DrawPathTest()
{
    if (!pathTest.active)
        return;

    const Math::vec4 startColor(0.2f, 1.0f, 0.3f, 1.0f);
    const Math::vec4 endColor(1.0f, 0.3f, 0.2f, 1.0f);
    const Math::vector lift(0.0f, 0.05f, 0.0f);
    const uint32_t onTop = Im3d::AlwaysOnTop;

    if (pathTest.hasHover)
        Im3d::Im3dContext::DrawPoint(ToVec3(pathTest.hover + lift), 8.0f, Math::vec4(1.0f, 1.0f, 1.0f, 0.8f), onTop);
    if (pathTest.hasStart)
        Im3d::Im3dContext::DrawPoint(ToVec3(pathTest.start + lift), 14.0f, startColor, onTop);
    if (pathTest.hasEnd)
        Im3d::Im3dContext::DrawPoint(ToVec3(pathTest.end + lift), 14.0f, endColor, onTop);

    // each segment in the color of its area
    const NavPath& path = pathTest.path;
    const Navigation::Build::AreaTable& table = GetAreaTable();
    for (IndexT i = 0; i + 1 < path.points.Size(); i++)
    {
        const Math::vec4 color = table.GetColor(path.areas[i]);
        Im3d::Im3dContext::DrawLine(Math::line(path.points[i] + lift, path.points[i + 1] + lift), 4.0f, Math::vec4(color.x, color.y, color.z, 1.0f), onTop);
        if (i > 0)
            Im3d::Im3dContext::DrawPoint(ToVec3(path.points[i] + lift), 7.0f, Math::vec4(1.0f, 1.0f, 1.0f, 1.0f), onTop);
    }
    // a partial path ends short of the end, the rest is shown as a thin line
    if (path.status == PathStatus::Partial && !path.points.IsEmpty())
        Im3d::Im3dContext::DrawLine(Math::line(path.points.Back() + lift, pathTest.end + lift), 1.0f, endColor, onTop);
}

//------------------------------------------------------------------------------
/**
    The area volumes of the scene. An unknown area is drawn in red, it fails the bake
*/
void
DrawAreaVolumes(Editor::Scene* scene)
{
    Editor::Shell* shell = Editor::EditorFeatureRegistry::GetShell();
    const Util::Array<Game::Entity>& selection = shell->GetSelection()->GetSelection();
    const Navigation::Build::AreaTable& table = GetAreaTable();
    for (const NavAreaVolume& volume : GetNavAreaVolumes(scene))
    {
        const bool selected = selection.FindIndex(volume.entity) != InvalidIndex;
        if (!selected && !display.areaVolumes)
            continue;

        const Navigation::Build::AreaDefinition* area = table.Find(volume.area.Value());
        Math::vec4 color = area != nullptr ? table.GetColor(area->id) : Math::vec4(1.0f, 0.0f, 0.0f, 1.0f);
        color.w = selected ? 1.0f : 0.6f;
        if (volume.points.Size() < 3)
        {
            Im3d::Im3dContext::DrawOrientedBox(volume.transform, volume.localBox, color, Im3d::CheckDepth | Im3d::Wireframe);
        }
        else
        {
            // crossing edges are drawn red, they fail the bake
            const Navigation::Build::AreaPrism prism = NavAreaPrism(volume);
            const bool simple = Navigation::Build::IsSimpleOutlineXZ(prism.outline.Begin(), prism.outline.Size());
            DrawAreaPrism(prism, simple ? color : Math::vec4(1.0f, 0.0f, 0.0f, 1.0f));
            if (selected && outline.target != volume.entity)
            {
                for (IndexT i = 0; i < volume.points.Size(); i++)
                {
                    const Math::vec3& point = volume.points[i];
                    const Math::vec4 p = volume.pointTransform * Math::vec4(point.x, point.y, point.z, 1.0f);
                    Im3d::Im3dContext::DrawPoint(Math::vec3(p.x, p.y, p.z), 8.0f, color);
                }
            }
        }
        if (selected)
        {
            const Math::vec4 center = volume.transform.position();
            Im3d::Im3dContext::DrawText(Math::vec3(center.x, center.y, center.z), volume.area.Value(), 16.0f, color);
        }
    }
}

//------------------------------------------------------------------------------
/**
    The feature's part of every viewport frame: what it draws, and the clicks it takes while an
    outline is edited or paths are tested
*/
bool
ViewportInput(const Editor::ToolView& view)
{
    Editor::Scene* scene = ActiveScene();
    if (scene == nullptr)
        return false;

    // a frame a gizmo handle took is the handle's
    bool taken = false;
    if (!view.gizmoUsed)
    {
        if (outline.target != Game::Entity::Invalid())
            taken = OutlineInput(view);
        else if (pathTest.active)
            taken = PathTestInput(view);
    }

    if (pathTest.active && pathTest.hasStart && pathTest.hasEnd)
        pathTest.path = FindPath(pathTest.navMesh, pathTest.start, pathTest.end, &pathTest.filter);

    if (display.navMeshes)
        DrawNavMeshesAndAgents();
    DrawAreaVolumes(scene);
    DrawOutline();
    DrawPathTest();
    return taken;
}

//------------------------------------------------------------------------------
/**
    The inspector field of NavArea.area, a choice of the areas in the area table
*/
void
DrawAreaField(Game::Entity owner, Game::ComponentId component, void* data, bool* commit)
{
    Util::StringAtom& value = *(Util::StringAtom*)data;
    const Navigation::Build::AreaTable& table = GetAreaTable();
    const Navigation::Build::AreaDefinition* current = value.IsValid() ? table.Find(value.Value()) : nullptr;
    const float swatch = ImGui::GetTextLineHeight();

    Util::String preview = value.IsValid() ? value.Value() : "";
    if (current == nullptr)
    {
        // a name the table lost keeps showing, so it can be seen and replaced
        preview.Append(" (not in the area table)");
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
    }
    const bool open = ImGui::BeginCombo("##area", preview.AsCharPtr());
    if (current == nullptr)
        ImGui::PopStyleColor();
    if (open)
    {
        for (const Navigation::Build::AreaDefinition& area : table.Areas())
        {
            ImGui::PushID(area.id);
            const Math::vec4 c = table.GetColor(area.id);
            ImGui::ColorButton("##color", ImVec4(c.x, c.y, c.z, area.id == Navigation::Build::NoArea ? 0.0f : 1.0f),
                ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_AlphaPreviewHalf, ImVec2(swatch, swatch));
            ImGui::SameLine();
            const bool selected = current == &area;
            if (ImGui::Selectable(area.name.AsCharPtr(), selected))
            {
                value = area.name;
                *commit = true;
            }
            if (area.id == Navigation::Build::NoArea)
                ImGui::SetItemTooltip("Cuts the surface out of the navmesh");
            else if (area.id == Navigation::Build::DefaultWalkableArea)
                ImGui::SetItemTooltip("The area of every surface nothing marks");
            if (selected)
                ImGui::SetItemDefaultFocus();
            ImGui::PopID();
        }
        ImGui::Separator();
        ImGui::TextDisabled("Areas are defined in the navigation panel");
        ImGui::EndCombo();
    }
}

//------------------------------------------------------------------------------
/**
    The inspector field of NavAgent.agentKind, a choice of the agent kinds of the loaded navmeshes
    and the open ones, or any name typed in
*/
void
DrawAgentKindField(Game::Entity owner, Game::ComponentId component, void* data, bool* commit)
{
    Util::StringAtom& value = *(Util::StringAtom*)data;
    Util::Array<Util::String> kinds;
    Navigation::StreamNavMeshCache* cache = GetNavMeshCache();
    if (cache != nullptr)
    {
        for (Navigation::NavMeshId id : cache->GetLoadedMeshes())
        {
            if (id == Navigation::InvalidNavMeshId || cache->GetDetourMesh(id) == nullptr)
                continue;
            const Util::String name = cache->GetAgent(id).name.c_str();
            if (!name.IsEmpty() && kinds.FindIndex(name) == InvalidIndex)
                kinds.Append(name);
        }
    }
    const Util::String open = NavMeshDocument::OpenAgentKind();
    if (!open.IsEmpty() && kinds.FindIndex(open) == InvalidIndex)
        kinds.Append(open);

    const Util::String current = value.IsValid() ? value.Value() : "";
    const bool known = kinds.FindIndex(current) != InvalidIndex;
    Util::String preview = current;
    if (!known)
    {
        // no navmesh for it is loaded, the agent waits for one
        preview.Append(current.IsEmpty() ? "(none)" : " (no navmesh loaded)");
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.6f, 0.3f, 1.0f));
    }
    const bool comboOpen = ImGui::BeginCombo("##agentkind", preview.AsCharPtr());
    if (!known)
        ImGui::PopStyleColor();
    if (comboOpen)
    {
        for (const Util::String& kind : kinds)
        {
            const bool selected = kind == current;
            if (ImGui::Selectable(kind.AsCharPtr(), selected))
            {
                value = kind;
                *commit = true;
            }
            if (selected)
                ImGui::SetItemDefaultFocus();
        }
        if (!kinds.IsEmpty())
            ImGui::Separator();
        static char typed[64] = "";
        ImGui::SetNextItemWidth(160.0f);
        if (ImGui::InputTextWithHint("##typed", "other kind", typed, sizeof(typed), ImGuiInputTextFlags_EnterReturnsTrue) && typed[0] != '\0')
        {
            value = Util::String(typed);
            typed[0] = '\0';
            *commit = true;
            ImGui::CloseCurrentPopup();
        }
        ImGui::TextDisabled("Kinds of the loaded navmeshes and the open one");
        ImGui::EndCombo();
    }
}

//------------------------------------------------------------------------------
/**
    The inspector of NavArea: its fields, and the outline which replaces the box
*/
void
DrawNavAreaInspector(Game::Entity owner, Game::ComponentId component, void* data, bool* commit)
{
    Game::ComponentDrawFuncT<NavigationFeature::NavArea>(owner, component, data, commit);

    NavigationFeature::NavArea& navArea = *(NavigationFeature::NavArea*)data;
    const uint capacity = (uint)navArea.points.size();
    navArea.pointCount = Math::min(navArea.pointCount, capacity);
    const bool hasOutline = navArea.pointCount >= 3;
    const char* trash = Editor::IconGlyph("trash-2");

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::AlignTextToFramePadding();
    ImGui::TextDisabled("Outline");
    ImGui::TableSetColumnIndex(1);
    ImGui::PushID("navareaoutline");

    const bool editing = outline.target == owner;
    // the label changes the moment it takes effect, a fixed id keeps the button the same item
    if (ImGui::Button(editing ? "Stop editing###editoutline" : "Edit outline###editoutline"))
    {
        if (editing)
            EndOutlineEdit();
        else
            BeginOutlineEdit(owner);
    }
    ImGui::SameLine();
    if (hasOutline)
    {
        ImGui::Text("%u points", navArea.pointCount);
        ImGui::SameLine();
        if (ImGui::Button("Reset to box"))
        {
            EndOutlineEdit();
            navArea.pointCount = 0;
            *commit = true;
        }
    }
    else
    {
        ImGui::TextDisabled("the box");
    }
    if (editing)
        ImGui::TextDisabled("Viewport: click adds, drag moves, Del or ctrl click removes, Esc stops");

    if (hasOutline)
    {
        Util::Array<Math::vec3> points;
        for (uint i = 0; i < navArea.pointCount; i++)
            points.Append(navArea.points[i]);
        if (!Navigation::Build::IsSimpleOutlineXZ(points.Begin(), points.Size()))
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Edges of the outline cross, move a point");

        // the point under the mouse in the viewport is highlighted here, a hovered row highlights its point there
        const IndexT viewportHovered = editing ? outline.hovered : InvalidIndex;
        IndexT rowHovered = InvalidIndex;
        int remove = -1;
        for (uint i = 0; i < navArea.pointCount; i++)
        {
            ImGui::PushID((int)i);
            const bool highlight = (IndexT)i == viewportHovered;
            if (highlight)
            {
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(1.0f, 0.85f, 0.2f, 0.35f));
                ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(1.0f, 0.85f, 0.2f, 0.5f));
            }
            // an outline keeps three points, Reset to box removes it
            ImGui::BeginDisabled(navArea.pointCount <= 3);
            if (ImGui::SmallButton(trash != nullptr ? trash : "x"))
                remove = (int)i;
            ImGui::EndDisabled();
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                rowHovered = i;
            ImGui::SameLine();
            ImGui::SetNextItemWidth(-FLT_MIN);
            // x and z place the point, y is the height it was put at
            if (ImGui::DragFloat3("##point", &navArea.points[i].x, 0.05f, 0.0f, 0.0f, "%.2f"))
                *commit = true;
            if (ImGui::IsItemHovered() || ImGui::IsItemActive())
                rowHovered = i;
            if (highlight)
                ImGui::PopStyleColor(2);
            ImGui::PopID();
        }
        SetOutlineHighlightedPoint(rowHovered);
        if (remove >= 0)
        {
            for (uint i = (uint)remove; i + 1 < navArea.pointCount; i++)
                navArea.points[i] = navArea.points[i + 1];
            navArea.pointCount--;
            *commit = true;
        }
    }
    ImGui::PopID();
}

//------------------------------------------------------------------------------
/**
    A navmesh asset dropped on the viewport is an entity that loads it
*/
void
DropNavMesh(Editor::EditorDroppedAsset& entity)
{
    NavigationFeature::NavMesh navMesh;
    navMesh.resource = entity.asset.AsString();
    entity.Add(Game::GetComponentId<NavigationFeature::NavMesh>(), &navMesh, sizeof(navMesh));
}

} // namespace

//------------------------------------------------------------------------------
/**
*/
NavigationDisplay&
GetNavigationDisplay()
{
    return display;
}

//------------------------------------------------------------------------------
/**
    The edit starts right away, the button that starts it is outside the viewport the clicks come
    from. The outline is made from the box on the next viewport frame
*/
void
BeginOutlineEdit(Game::Entity entity)
{
    EndPathTest();
    outline = OutlineEdit();
    outline.target = entity;
    outline.scene = ActiveScene();
}

//------------------------------------------------------------------------------
/**
*/
void
EndOutlineEdit()
{
    outline = OutlineEdit();
}

//------------------------------------------------------------------------------
/**
*/
Game::Entity
OutlineEditTarget()
{
    return outline.target;
}

//------------------------------------------------------------------------------
/**
*/
IndexT
OutlineHoveredPoint()
{
    return outline.target != Game::Entity::Invalid() ? outline.hovered : InvalidIndex;
}

//------------------------------------------------------------------------------
/**
*/
void
SetOutlineHighlightedPoint(IndexT index)
{
    outline.highlighted = index;
}

//------------------------------------------------------------------------------
/**
    The costs are kept when the test starts again, unless it moves to another navmesh
*/
void
BeginPathTest(Navigation::NavMeshId navMesh)
{
    EndOutlineEdit();
    if (!pathTest.active && pathTest.navMesh != navMesh)
        pathTest.filter = DefaultQueryFilter();
    if (pathTest.navMesh != navMesh)
        ClearPathTest();
    pathTest.active = true;
    pathTest.navMesh = navMesh;
}

//------------------------------------------------------------------------------
/**
*/
void
EndPathTest()
{
    pathTest.active = false;
    pathTest.hasHover = false;
}

//------------------------------------------------------------------------------
/**
*/
bool
IsPathTesting()
{
    return pathTest.active;
}

//------------------------------------------------------------------------------
/**
*/
Navigation::NavMeshId
PathTestNavMesh()
{
    return pathTest.navMesh;
}

//------------------------------------------------------------------------------
/**
*/
QueryFilter&
PathTestFilter()
{
    return pathTest.filter;
}

//------------------------------------------------------------------------------
/**
*/
const NavPath&
PathTestPath()
{
    return pathTest.path;
}

//------------------------------------------------------------------------------
/**
*/
bool
PathTestHasStart()
{
    return pathTest.hasStart;
}

//------------------------------------------------------------------------------
/**
*/
bool
PathTestHasEnd()
{
    return pathTest.hasEnd;
}

//------------------------------------------------------------------------------
/**
*/
void
ClearPathTest()
{
    pathTest.hasStart = false;
    pathTest.hasEnd = false;
    pathTest.path = NavPath();
}

//------------------------------------------------------------------------------
/**
*/
void
SetPathTestPoint(bool end, const Math::point& point)
{
    if (end)
    {
        pathTest.end = point;
        pathTest.hasEnd = true;
    }
    else
    {
        pathTest.start = point;
        pathTest.hasStart = true;
    }
    if (pathTest.hasStart && pathTest.hasEnd)
        pathTest.path = FindPath(pathTest.navMesh, pathTest.start, pathTest.end, &pathTest.filter);
    else
        pathTest.path = NavPath();
}

//------------------------------------------------------------------------------
/**
    Selected so it can be placed and its outline edited
*/
void
CreateAreaVolume(const char* area)
{
    Editor::Shell* shell = Editor::EditorFeatureRegistry::GetShell();
    Ptr<Editor::Scene> scene = shell != nullptr ? shell->GetScene() : nullptr;
    if (!scene.isvalid())
        return;

    Ptr<Editor::EditCommandManager> manager = scene->GetCommandManager();
    manager->BeginMacro("Create navigation area");
    Ptr<Editor::CreateEntityCommand> create = Editor::CreateEntityCommand::Create();
    create->scene = scene;
    create->record.name = scene->UniqueEntityName(Util::String("Nav Area ") + area);
    create->record.collection = scene->GetActiveCollection();
    manager->Execute(create);
    if (scene->HasInstance(create->record.entity))
    {
        NavigationFeature::NavArea navArea;
        navArea.area = area;
        Ptr<Editor::AddComponentCommand> add = Editor::AddComponentCommand::Create();
        add->scene = scene;
        add->entity = create->record.entity;
        add->component = Game::GetComponentId<NavigationFeature::NavArea>();
        add->value = Util::Blob(&navArea, sizeof(navArea));
        manager->Execute(add);

        Util::Array<Game::Entity> selection;
        selection.Append(create->record.entity);
        shell->GetSelection()->SetSelection(selection);
    }
    manager->EndMacro();
}

//------------------------------------------------------------------------------
/**
*/
void
RegisterEditorContributions()
{
    // the area table is compiled from the project's, so names and colors match what a bake uses
    Navigation::Build::CompileAreaTable();
    ReloadAreaTable();

    using OpenDelegate = Util::Delegate<void(const IO::Path&)>;
    using CreateDelegate = Util::Delegate<bool(const IO::Path&, const IO::URI&, const Util::String&, Util::String&)>;
    Editor::AssetTypeRegistry::Register({ .type = "nav", .workExtension = "nanav", .name = "Navmesh", .icon = "map",
        .create = CreateDelegate::FromFunction<&NavMeshDocument::CreateAsset>(),
        .open = OpenDelegate::FromFunction<&NavMeshDocument::OpenAsset>() });
    Editor::EditorFeatureRegistry::RegisterAssetDrop("nav", Util::Delegate<void(Editor::EditorDroppedAsset&)>::FromFunction<&DropNavMesh>());

    Game::ComponentInspection::OverrideField(Game::GetComponentId<NavigationFeature::NavArea>(), "area", &DrawAreaField);
    Game::ComponentInspection::OverrideField(Game::GetComponentId<NavigationFeature::NavAgent>(), "agentKind", &DrawAgentKindField);
    Game::ComponentInspection::OverrideInspector(Game::GetComponentId<NavigationFeature::NavArea>(), &DrawNavAreaInspector);

    Editor::EditorFeatureRegistry::RegisterViewportInput(Util::Delegate<bool(const Editor::ToolView&)>::FromFunction<&ViewportInput>());
    Editor::EditorFeatureRegistry::RegisterPanel(NavigationPanel::Create());
}

//------------------------------------------------------------------------------
/**
    The documents hold navmeshes from the cache, so they go first. The inspector of NavArea goes
    back to the one the component was registered with
*/
void
ClearEditorContributions()
{
    EndOutlineEdit();
    EndPathTest();
    Editor::AssetEditorPanel::CloseDocumentsOf("nav");
    Editor::AssetTypeRegistry::Unregister("nav");
    Game::ComponentInspection::OverrideField(Game::GetComponentId<NavigationFeature::NavArea>(), "area", nullptr);
    Game::ComponentInspection::OverrideField(Game::GetComponentId<NavigationFeature::NavAgent>(), "agentKind", nullptr);
    Game::ComponentInspection::OverrideInspector(Game::GetComponentId<NavigationFeature::NavArea>(), &Game::ComponentDrawFuncT<NavigationFeature::NavArea>);
}

} // namespace NavigationFeature
#endif // WITH_NEBULA_EDITOR
