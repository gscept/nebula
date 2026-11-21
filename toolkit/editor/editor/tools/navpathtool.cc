//------------------------------------------------------------------------------
//  @file navpathtool.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "navpathtool.h"
#include "input/inputserver.h"
#include "input/mouse.h"
#include "input/keyboard.h"
#include "renderutil/mouserayutil.h"
#include "dynui/im3d/im3dcontext.h"
#include "navigationfeature/navigationfeatureunit.h"
#include "navigationfeature/streamnavmeshcache.h"
#include "navbuild/geometrysources.h"
#include "DetourNavMesh.h"
#include "imgui.h"

namespace Navigation
{
extern StreamNavMeshCache* navMeshCache;
}

namespace Tools
{

namespace
{
Math::vec3
ToVec3(const Math::point& p)
{
    return Math::vec3(p.x, p.y, p.z);
}

struct PathToolState
{
    bool active = false;
    Navigation::NavMeshId navMesh = Navigation::InvalidNavMeshId;
    NavigationFeature::QueryFilter filter;
    bool hasStart = false;
    bool hasEnd = false;
    Math::point start;
    Math::point end;
    NavigationFeature::NavPath path;
    /// the mouse on the navmesh, drawn as a marker
    bool hasHover = false;
    Math::point hover;
};
PathToolState pathState;
}

//------------------------------------------------------------------------------
/**
*/
void
NavPathTool::Begin(Navigation::NavMeshId navMesh)
{
    // the costs are kept when the tool is started again, unless it moves to another navmesh
    if (!pathState.active && pathState.navMesh != navMesh)
    {
        pathState.filter = NavigationFeature::DefaultQueryFilter();
    }
    if (pathState.navMesh != navMesh)
    {
        Clear();
    }
    pathState.active = true;
    pathState.navMesh = navMesh;
}

//------------------------------------------------------------------------------
/**
*/
void
NavPathTool::End()
{
    pathState.active = false;
    pathState.hasHover = false;
}

//------------------------------------------------------------------------------
/**
*/
bool
NavPathTool::Active()
{
    return pathState.active;
}

//------------------------------------------------------------------------------
/**
*/
Navigation::NavMeshId
NavPathTool::NavMesh()
{
    return pathState.navMesh;
}

//------------------------------------------------------------------------------
/**
*/
NavigationFeature::QueryFilter&
NavPathTool::Filter()
{
    return pathState.filter;
}

//------------------------------------------------------------------------------
/**
*/
const NavigationFeature::NavPath&
NavPathTool::Path()
{
    return pathState.path;
}

//------------------------------------------------------------------------------
/**
*/
bool
NavPathTool::HasStart()
{
    return pathState.hasStart;
}

//------------------------------------------------------------------------------
/**
*/
bool
NavPathTool::HasEnd()
{
    return pathState.hasEnd;
}

//------------------------------------------------------------------------------
/**
*/
void
NavPathTool::Clear()
{
    pathState.hasStart = false;
    pathState.hasEnd = false;
    pathState.path = NavigationFeature::NavPath();
}

//------------------------------------------------------------------------------
/**
*/
bool
NavPathTool::IsModifying() const
{
    return false;
}

//------------------------------------------------------------------------------
/**
*/
void
NavPathTool::Abort()
{
    End();
}

//------------------------------------------------------------------------------
/**
    The navmesh is checked every frame, it goes away when its preview is discarded or its entity deleted
*/
static bool
NavMeshLoaded(Navigation::NavMeshId navMesh)
{
    if (navMesh == Navigation::InvalidNavMeshId || Navigation::navMeshCache == nullptr)
    {
        return false;
    }
    for (Navigation::NavMeshId loaded : Navigation::navMeshCache->GetLoadedMeshes())
    {
        if (loaded == navMesh)
        {
            return Navigation::navMeshCache->GetDetourMesh(navMesh) != nullptr;
        }
    }
    return false;
}

//------------------------------------------------------------------------------
/**
    Where the mouse ray meets the navmesh
*/
static bool
MouseOnNavMesh(Presentation::Modules::Viewport* viewport, Navigation::NavMeshId navMesh, Math::point& out)
{
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
    const Math::line ray = RenderUtil::MouseRayUtil::ComputeWorldMouseRay(mousePos, 10000, camTransform, invProj, 0.01f);

    const dtNavMesh* mesh = Navigation::navMeshCache->GetDetourMesh(navMesh);
    float t;
    if (mesh == nullptr || !Navigation::Build::RaycastNavMesh(*mesh, ray.start(), ray.vec(), t))
    {
        return false;
    }
    out = ray.start() + ray.vec() * t;
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
NavPathTool::Update(Presentation::Modules::Viewport* viewport)
{
    Ptr<Input::Keyboard> keyboard = Input::InputServer::Instance()->GetDefaultKeyboard();
    Ptr<Input::Mouse> mouse = Input::InputServer::Instance()->GetDefaultMouse();
    if (keyboard->KeyDown(Input::Key::Escape) || !NavMeshLoaded(pathState.navMesh))
    {
        End();
        return;
    }

    pathState.hasHover = MouseOnNavMesh(viewport, pathState.navMesh, pathState.hover);

    // a click without dragging, dragging turns the camera
    const ImGuiIO& io = ImGui::GetIO();
    const bool clicked = ImGui::IsMouseReleased(ImGuiMouseButton_Left) &&
        io.MouseDragMaxDistanceSqr[ImGuiMouseButton_Left] < io.MouseDragThreshold * io.MouseDragThreshold;
    if (clicked && pathState.hasHover)
    {
        const bool shift = keyboard->KeyPressed(Input::Key::LeftShift) || keyboard->KeyPressed(Input::Key::RightShift);
        if (shift)
        {
            pathState.end = pathState.hover;
            pathState.hasEnd = true;
        }
        else
        {
            pathState.start = pathState.hover;
            pathState.hasStart = true;
        }
    }

    if (pathState.hasStart && pathState.hasEnd)
    {
        pathState.path = NavigationFeature::FindPath(pathState.navMesh, pathState.start, pathState.end, &pathState.filter);
    }
    else
    {
        pathState.path = NavigationFeature::NavPath();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
NavPathTool::Render(Presentation::Modules::Viewport* viewport)
{
    if (!pathState.active)
    {
        return;
    }

    const Math::vec4 startColor(0.2f, 1.0f, 0.3f, 1.0f);
    const Math::vec4 endColor(1.0f, 0.3f, 0.2f, 1.0f);
    const Math::vector lift(0.0f, 0.05f, 0.0f);
    const uint32_t onTop = Im3d::AlwaysOnTop;

    if (pathState.hasHover)
    {
        Im3d::Im3dContext::DrawPoint(ToVec3(pathState.hover + lift), 8.0f, Math::vec4(1.0f, 1.0f, 1.0f, 0.8f), onTop);
    }
    if (pathState.hasStart)
    {
        Im3d::Im3dContext::DrawPoint(ToVec3(pathState.start + lift), 14.0f, startColor, onTop);
    }
    if (pathState.hasEnd)
    {
        Im3d::Im3dContext::DrawPoint(ToVec3(pathState.end + lift), 14.0f, endColor, onTop);
    }

    // each segment in the color of its area
    const NavigationFeature::NavPath& path = pathState.path;
    const Navigation::Build::AreaTable& table = NavigationFeature::GetAreaTable();
    for (IndexT i = 0; i + 1 < path.points.Size(); i++)
    {
        const Math::vec4 color = table.GetColor(path.areas[i]);
        Im3d::Im3dContext::DrawLine(Math::line(path.points[i] + lift, path.points[i + 1] + lift), 4.0f, Math::vec4(color.x, color.y, color.z, 1.0f), onTop);
        if (i > 0)
        {
            Im3d::Im3dContext::DrawPoint(ToVec3(path.points[i] + lift), 7.0f, Math::vec4(1.0f, 1.0f, 1.0f, 1.0f), onTop);
        }
    }

    // a partial path ends short of the end, the rest is shown as a thin line
    if (path.status == NavigationFeature::PathStatus::Partial && !path.points.IsEmpty())
    {
        Im3d::Im3dContext::DrawLine(Math::line(path.points.Back() + lift, pathState.end + lift), 1.0f, endColor, onTop);
    }
}

} // namespace Tools
