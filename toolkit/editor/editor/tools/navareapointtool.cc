//------------------------------------------------------------------------------
//  @file navareapointtool.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "navareapointtool.h"
#include "navmeshsources.h"
#include "selectioncontext.h"
#include "camera.h"
#include "editor/cmds.h"
#include "input/inputserver.h"
#include "input/mouse.h"
#include "input/keyboard.h"
#include "renderutil/mouserayutil.h"
#include "dynui/im3d/im3dcontext.h"
#include "basegamefeature/components/position.h"
#include "basegamefeature/components/orientation.h"
#include "basegamefeature/components/scale.h"
#include "navigationfeature/components/navigation.h"
#include "navigationfeature/navigationfeatureunit.h"
#include "navbuild/geometrysources.h"
#include "imgui.h"

namespace Tools
{

static Editor::Entity outlineTarget = Editor::Entity::Invalid();
static IndexT viewportHoveredPoint = InvalidIndex;
static IndexT highlightedPoint = InvalidIndex;
static const char* PointMenuId = "##navareapointmenu";
static void EnsureOutline(Editor::Entity entity);

//------------------------------------------------------------------------------
/**
*/
void
NavAreaPointTool::Begin(Editor::Entity entity)
{
    outlineTarget = entity;
    // right away, the button which starts the tool is outside the viewport the tool runs in
    EnsureOutline(entity);
}

//------------------------------------------------------------------------------
/**
*/
void
NavAreaPointTool::End()
{
    outlineTarget = Editor::Entity::Invalid();
}

//------------------------------------------------------------------------------
/**
*/
Editor::Entity
NavAreaPointTool::Target()
{
    return outlineTarget;
}

//------------------------------------------------------------------------------
/**
*/
IndexT
NavAreaPointTool::HoveredPoint()
{
    return outlineTarget != Editor::Entity::Invalid() ? viewportHoveredPoint : InvalidIndex;
}

//------------------------------------------------------------------------------
/**
*/
void
NavAreaPointTool::SetHighlightedPoint(IndexT index)
{
    highlightedPoint = index;
}

//------------------------------------------------------------------------------
/**
*/
IndexT
NavAreaPointTool::HighlightedPoint()
{
    return highlightedPoint;
}

//------------------------------------------------------------------------------
/**
*/
bool
NavAreaPointTool::IsModifying() const
{
    return this->dragged != InvalidIndex;
}

//------------------------------------------------------------------------------
/**
*/
void
NavAreaPointTool::Abort()
{
    this->dragged = InvalidIndex;
    End();
}

//------------------------------------------------------------------------------
/**
*/
static void
Commit(Editor::Entity entity, const Util::Array<Math::vec3>& localPoints)
{
    NavigationFeature::NavArea navArea = Editor::state.editorWorld->GetComponent<NavigationFeature::NavArea>(entity);
    navArea.pointCount = (uint)Math::min((SizeT)localPoints.Size(), (SizeT)navArea.points.size());
    for (uint i = 0; i < navArea.pointCount; i++)
    {
        navArea.points[i] = localPoints[i];
    }
    Edit::SetComponent(entity, Game::GetComponentId<NavigationFeature::NavArea>(), &navArea);
}

//------------------------------------------------------------------------------
/**
*/
bool
NavAreaPointTool::MouseRay(Presentation::Modules::Viewport* viewport, Math::line& ray) const
{
    // the mouse in the space of the viewport image, the same way picking does it
    Ptr<Input::Mouse> mouse = Input::InputServer::Instance()->GetDefaultMouse();
    Math::vec2 mousePos = mouse->GetPixelPosition();
    mousePos -= viewport->lastViewportImagePosition;
    mousePos = { mousePos.x / viewport->lastViewportImageSize.x, mousePos.y / viewport->lastViewportImageSize.y };
    if (mousePos.x < 0.0f || mousePos.y < 0.0f || mousePos.x > 1.0f || mousePos.y > 1.0f)
    {
        return false;
    }
    const Math::mat4 camTransform = Math::inverse(viewport->camera.GetViewTransform());
    const Math::mat4 invProj = Math::inverse(viewport->camera.GetProjectionTransform());
    ray = RenderUtil::MouseRayUtil::ComputeWorldMouseRay(mousePos, 10000, camTransform, invProj, 0.01f);
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
NavAreaPointTool::SurfacePoint(const Math::line& ray, float planeY, Math::vec3& point) const
{
    float t;
    if (NavigationFeature::RaycastNavMeshes(ray.start(), ray.vec(), t))
    {
        const Math::point hit = ray.start() + ray.vec() * t;
        point = Math::vec3(hit.x, hit.y, hit.z);
        return true;
    }
    const Math::vector dir = ray.vec();
    if (Math::abs(dir.y) < 1e-6f)
    {
        return false;
    }
    t = (planeY - ray.start().y) / dir.y;
    if (t < 0.0f || t > 1.0f)
    {
        return false;
    }
    const Math::point hit = ray.start() + dir * t;
    point = Math::vec3(hit.x, hit.y, hit.z);
    return true;
}

//------------------------------------------------------------------------------
/**
*/
IndexT
NavAreaPointTool::PickPoint(const Math::line& ray, const Util::Array<Math::vec3>& points) const
{
    const Math::vec3 origin(ray.start().x, ray.start().y, ray.start().z);
    const Math::vec3 dir = Math::normalize(Math::vec3(ray.vec().x, ray.vec().y, ray.vec().z));
    IndexT best = InvalidIndex;
    float bestDistance = FLT_MAX;
    for (IndexT i = 0; i < points.Size(); i++)
    {
        const Math::vec3 toPoint = points[i] - origin;
        const float along = Math::dot(toPoint, dir);
        if (along <= 0.0f)
        {
            continue;
        }
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
    A volume without an outline gets the footprint of its box, at the height of its center
*/
static void
EnsureOutline(Editor::Entity entity)
{
    Game::World* world = Editor::state.editorWorld;
    if (!world->HasComponent<NavigationFeature::NavArea>(entity))
    {
        return;
    }
    const NavigationFeature::NavArea navArea = world->GetComponent<NavigationFeature::NavArea>(entity);
    if (navArea.pointCount >= 3)
    {
        return;
    }

    const Math::vec3 position = world->GetComponent<Game::Position>(entity);
    const Math::quat orientation = world->GetComponent<Game::Orientation>(entity);
    const Math::mat4 toLocal = Math::inverse(Math::trs(position, orientation, Math::vec3(1.0f)));
    Math::bbox localBox;
    const Math::vec3 extents(Math::abs(navArea.extents.x), Math::abs(navArea.extents.y), Math::abs(navArea.extents.z));
    localBox.pmin = Math::point(-extents.x, -extents.y, -extents.z);
    localBox.pmax = Math::point(extents.x, extents.y, extents.z);
    const Math::mat4 boxTransform = Math::trs(position, orientation, world->GetComponent<Game::Scale>(entity));
    Util::Array<Math::vec3> outline;
    for (const Math::vec3& p : Navigation::Build::BoxOutlineXZ(boxTransform, localBox))
    {
        const Math::vec4 l = toLocal * Math::vec4(p.x, position.y, p.z, 1.0f);
        outline.Append(Math::vec3(l.x, l.y, l.z));
    }
    Commit(entity, outline);
}

//------------------------------------------------------------------------------
/**
    Everything which doesn't need the mouse, so the outline shows and stays valid while the mouse is
    elsewhere, like on the inspector button which started the tool. Returns false if the tool ended
*/
bool
NavAreaPointTool::Refresh()
{
    Game::World* world = Editor::state.editorWorld;

    // the tool belongs to one selected NavArea
    const Util::Array<Editor::Entity>& selection = SelectionContext::Selection();
    if (outlineTarget == Editor::Entity::Invalid() || selection.Size() != 1 ||
        selection[0] != outlineTarget || !world->HasComponent<NavigationFeature::NavArea>(outlineTarget))
    {
        this->dragged = InvalidIndex;
        this->hovered = InvalidIndex;
        this->menuPoint = InvalidIndex;
        viewportHoveredPoint = InvalidIndex;
        this->worldPoints.Clear();
        End();
        return false;
    }

    // an undo can take the outline away while the tool runs
    EnsureOutline(outlineTarget);

    this->frame = Math::trs(world->GetComponent<Game::Position>(outlineTarget), world->GetComponent<Game::Orientation>(outlineTarget), Math::vec3(1.0f));
    const NavigationFeature::NavArea navArea = world->GetComponent<NavigationFeature::NavArea>(outlineTarget);
    if (this->dragged == InvalidIndex)
    {
        this->localPoints.Clear();
        for (uint i = 0; i < navArea.pointCount; i++)
        {
            this->localPoints.Append(navArea.points[i]);
        }
    }
    this->worldPoints.Clear();
    for (const Math::vec3& p : this->localPoints)
    {
        const Math::vec4 w = this->frame * Math::vec4(p.x, p.y, p.z, 1.0f);
        this->worldPoints.Append(Math::vec3(w.x, w.y, w.z));
    }
    return true;
}

//------------------------------------------------------------------------------
/**
    Runs only while the viewport has the mouse
*/
void
NavAreaPointTool::Update(Presentation::Modules::Viewport* viewport)
{
    Ptr<Input::Keyboard> keyboard = Input::InputServer::Instance()->GetDefaultKeyboard();
    Ptr<Input::Mouse> mouse = Input::InputServer::Instance()->GetDefaultMouse();

    if (keyboard->KeyDown(Input::Key::Escape))
    {
        this->dragged = InvalidIndex;
        End();
    }
    if (!this->Refresh())
    {
        return;
    }

    const Math::vec4 framePosition = this->frame.position();
    const Math::vec3 position(framePosition.x, framePosition.y, framePosition.z);
    const Math::mat4 toLocal = Math::inverse(this->frame);
    auto toLocalPoint = [&toLocal](const Math::vec3& p)
    {
        const Math::vec4 l = toLocal * Math::vec4(p.x, p.y, p.z, 1.0f);
        return Math::vec3(l.x, l.y, l.z);
    };
    const NavigationFeature::NavArea navArea = Editor::state.editorWorld->GetComponent<NavigationFeature::NavArea>(outlineTarget);

    // the menu takes the mouse while it is open, and the click which closes it
    if (ImGui::IsPopupOpen(PointMenuId) || ImGui::GetFrameCount() - this->menuVisibleFrame <= 1)
    {
        return;
    }

    Math::line ray;
    if (!this->MouseRay(viewport, ray))
    {
        this->hasSurfacePoint = false;
        this->hovered = InvalidIndex;
        viewportHoveredPoint = InvalidIndex;
        return;
    }

    // a dragged point stays at its height where the navmeshes are missed
    const float planeY = this->dragged != InvalidIndex ? this->worldPoints[this->dragged].y : position.y;
    this->hasSurfacePoint = this->SurfacePoint(ray, planeY, this->surfacePoint);

    if (this->dragged != InvalidIndex)
    {
        if (mouse->ButtonPressed(Input::MouseButton::LeftButton))
        {
            if (this->hasSurfacePoint)
            {
                this->localPoints[this->dragged] = toLocalPoint(this->surfacePoint);
            }
        }
        else
        {
            // one undo step for the whole drag
            this->dragged = InvalidIndex;
            Commit(outlineTarget, this->localPoints);
        }
        return;
    }

    this->hovered = this->PickPoint(ray, this->worldPoints);
    viewportHoveredPoint = this->hovered;

    // a right click on a point, without turning the camera, opens its menu
    const ImGuiIO& io = ImGui::GetIO();
    if (this->hovered != InvalidIndex && ImGui::IsMouseReleased(ImGuiMouseButton_Right) &&
        io.MouseDragMaxDistanceSqr[ImGuiMouseButton_Right] < io.MouseDragThreshold * io.MouseDragThreshold)
    {
        this->menuPoint = this->hovered;
        ImGui::OpenPopup(PointMenuId);
        return;
    }

    const bool ctrl = keyboard->KeyPressed(Input::Key::LeftControl) || keyboard->KeyPressed(Input::Key::RightControl);
    const bool removeHovered = this->hovered != InvalidIndex &&
        (keyboard->KeyDown(Input::Key::Delete) || (ctrl && mouse->ButtonDown(Input::MouseButton::LeftButton)));
    if (removeHovered)
    {
        // an outline needs three points, Reset to box in the inspector removes it
        if (this->localPoints.Size() > 3)
        {
            this->localPoints.EraseIndex(this->hovered);
            this->hovered = InvalidIndex;
            Commit(outlineTarget, this->localPoints);
        }
        return;
    }

    if (mouse->ButtonDown(Input::MouseButton::LeftButton))
    {
        if (this->hovered != InvalidIndex)
        {
            this->dragged = this->hovered;
        }
        else if (this->hasSurfacePoint)
        {
            if (this->localPoints.Size() >= navArea.points.size())
            {
                n_warning("A navigation area outline takes %d points at most\n", (int)navArea.points.size());
                return;
            }
            const IndexT edge = Navigation::Build::OutlineInsertIndex(this->worldPoints.Begin(), this->worldPoints.Size(), this->surfacePoint);
            this->localPoints.Insert(edge + 1, toLocalPoint(this->surfacePoint));
            Commit(outlineTarget, this->localPoints);
        }
    }
}

//------------------------------------------------------------------------------
/**
    The points, the one under the mouse, and where a click would put a new one
*/
void
NavAreaPointTool::Render(Presentation::Modules::Viewport* viewport)
{
    if (outlineTarget == Editor::Entity::Invalid() || !this->Refresh() || this->worldPoints.IsEmpty())
    {
        return;
    }

    if (ImGui::BeginPopup(PointMenuId))
    {
        this->menuVisibleFrame = ImGui::GetFrameCount();
        const bool canRemove = this->localPoints.Size() > 3 && this->menuPoint >= 0 && this->menuPoint < this->localPoints.Size();
        ImGui::TextDisabled("Point %d", (int)this->menuPoint + 1);
        ImGui::Separator();
        if (ImGui::MenuItem("Delete point", "Del", false, canRemove))
        {
            this->localPoints.EraseIndex(this->menuPoint);
            this->hovered = InvalidIndex;
            Commit(outlineTarget, this->localPoints);
        }
        if (!canRemove)
        {
            ImGui::SetItemTooltip("An outline keeps three points, Reset to box in the inspector removes it");
        }
        if (ImGui::MenuItem("Stop editing", "Esc"))
        {
            End();
        }
        ImGui::EndPopup();
    }

    const Math::vec4 white(1.0f, 1.0f, 1.0f, 1.0f);
    const Math::vec4 highlight(1.0f, 0.85f, 0.2f, 1.0f);

    // while dragging, the outline of the drag; the committed one is drawn by the navigation window
    if (this->dragged != InvalidIndex)
    {
        for (IndexT i = 0; i < this->worldPoints.Size(); i++)
        {
            const Math::vec3& a = this->worldPoints[i];
            const Math::vec3& b = this->worldPoints[(i + 1) % this->worldPoints.Size()];
            Im3d::Im3dContext::DrawLine(Math::line(Math::point(a.x, a.y, a.z), Math::point(b.x, b.y, b.z)), 2.0f, white);
        }
    }
    for (IndexT i = 0; i < this->worldPoints.Size(); i++)
    {
        const bool active = i == this->hovered || i == this->dragged || (i == this->menuPoint && ImGui::IsPopupOpen(PointMenuId)) || i == highlightedPoint;
        Im3d::Im3dContext::DrawPoint(this->worldPoints[i], active ? 16.0f : 10.0f, active ? highlight : white);
    }

    // the edge a click would put the new point into
    if (this->dragged == InvalidIndex && this->hovered == InvalidIndex && this->hasSurfacePoint)
    {
        const IndexT edge = Navigation::Build::OutlineInsertIndex(this->worldPoints.Begin(), this->worldPoints.Size(), this->surfacePoint);
        const Math::vec3& a = this->worldPoints[edge];
        const Math::vec3& b = this->worldPoints[(edge + 1) % this->worldPoints.Size()];
        const Math::point p(this->surfacePoint.x, this->surfacePoint.y, this->surfacePoint.z);
        const Math::vec4 preview(1.0f, 1.0f, 1.0f, 0.5f);
        Im3d::Im3dContext::DrawLine(Math::line(Math::point(a.x, a.y, a.z), p), 1.0f, preview);
        Im3d::Im3dContext::DrawLine(Math::line(p, Math::point(b.x, b.y, b.z)), 1.0f, preview);
        Im3d::Im3dContext::DrawPoint(this->surfacePoint, 10.0f, preview);
    }
}

} // namespace Tools
