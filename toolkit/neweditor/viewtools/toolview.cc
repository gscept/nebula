//------------------------------------------------------------------------------
//  @file toolview.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "neweditor/viewtools/toolview.h"
#include "math/mat4.h"
#include "math/scalar.h"

namespace Editor
{

// The mouse ray's reach and near plane, same values the old editor's viewports passed
static const float RayLength = 10000.0f;
static const float NearPlane = 0.01f;

//------------------------------------------------------------------------------
/**
*/
bool
ToolView::IsValid() const
{
    return this->canvasSize.x > 0.0f && this->canvasSize.y > 0.0f;
}

//------------------------------------------------------------------------------
/**
    The old editor's MouseRayUtil ray, copied here because this project links lite-render rather
    than the stock render library the util lives in. Same construction, same reach, same near plane:
    a line from the near plane point to a point RayLength out, which is what lets
    Math::plane::intersect, which the old tools used, land on it.
*/
bool
ToolView::CanvasRay(const Math::vec2& windowPos, Math::line& ray) const
{
    if (!this->IsValid())
        return false;

    const Math::vec2 normalized(
        (windowPos.x - this->canvasMin.x) / this->canvasSize.x,
        (windowPos.y - this->canvasMin.y) / this->canvasSize.y);

    const Math::mat4 invView = Math::inverse(this->view);
    const Math::mat4 invProj = Math::inverse(this->projection);

    Math::vec4 screenCoord3D((normalized.x - 0.5f) * 2.0f, (normalized.y - 0.5f) * 2.0f, 1.0f, 1.0f);
    Math::vec4 viewCoord = invProj * screenCoord3D;
    viewCoord.w = 0.0f;
    Math::vec4 localMousePos = viewCoord * NearPlane * 1.1f;
    localMousePos.w = 1.0f;
    localMousePos.y = -localMousePos.y;
    Math::vec4 worldMousePos = invView * localMousePos;
    Math::vec4 worldMouseDir = worldMousePos - invView.position();
    worldMouseDir = Math::normalize(worldMouseDir);
    worldMouseDir = worldMouseDir * RayLength;

    ray = Math::line(Math::point(worldMousePos.vec), Math::point((worldMousePos + worldMouseDir).vec));
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
ToolView::Project(const Math::vec3& worldPos, Math::vec2& windowPos) const
{
    if (!this->IsValid())
        return false;

    const Math::vec4 clip = (this->projection * this->view) * Math::vec4(worldPos, 1.0f);
    if (clip.w < 1e-6f)
        return false;   // behind the camera

    const float ndcX = clip.x / clip.w;
    const float ndcY = clip.y / clip.w;

    const float u = (ndcX + 1.0f) * 0.5f;
    const float v = 1.0f - (ndcY + 1.0f) * 0.5f;

    windowPos.x = this->canvasMin.x + u * this->canvasSize.x;
    windowPos.y = this->canvasMin.y + v * this->canvasSize.y;
    return true;
}

} // namespace Editor
