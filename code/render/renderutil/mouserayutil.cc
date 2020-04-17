//------------------------------------------------------------------------------
//  mouserayutil.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "renderutil/mouserayutil.h"

namespace RenderUtil
{
using namespace Math;

//------------------------------------------------------------------------------
/**
    Utility function which computes a ray in world space between the eye
    and the current mouse position on the near plane.
    Mouse position is expected in the range 0..1.
*/
line
MouseRayUtil::ComputeWorldMouseRay(const vec2& mousePos, float length, const mat4& invViewMatrix, const mat4& invProjMatrix, float nearPlane)
{
    // Compute mouse position in world coordinates.
    vec4 screenCoord3D((mousePos.x - 0.5f) * 2.0f, (mousePos.y - 0.5f) * 2.0f, 1.0f, 1.0f);
    vec4 viewCoord = invProjMatrix * screenCoord3D;
    vec4 localMousePos = viewCoord * nearPlane * 1.1f;
    localMousePos.y = -1 * localMousePos.y;
    vec4 worldMousePos = invViewMatrix * localMousePos;
    vec4 worldMouseDir = worldMousePos - invViewMatrix.r[Math::POSITION];
    worldMouseDir = normalize(worldMouseDir);
    worldMouseDir *= length;

    return line(worldMousePos.vec, (worldMousePos + worldMouseDir).vec);
}

} // namespace RenderUtil
