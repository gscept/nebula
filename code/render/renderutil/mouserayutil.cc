//------------------------------------------------------------------------------
//  mouserayutil.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "renderutil/mouserayutil.h"

namespace RenderUtil
{
using namespace Math;

//------------------------------------------------------------------------------
/**
    Utility function which computes a ray in world space between the eye
    and the current mouse position on the near plane of a perspective view frustum.
    Mouse position is expected in the range 0..1.
*/
line
MouseRayUtil::ComputeWorldMouseRay(const vec2& mousePos, float length, const mat4& invViewMatrix, const mat4& invProjMatrix, float nearPlane)
{
    // Compute mouse position in world coordinates.
    vec4 screenCoord3D((mousePos.x - 0.5f) * 2.0f, (mousePos.y - 0.5f) * 2.0f, 1.0f, 1.0f);
    vec4 viewCoord = invProjMatrix * screenCoord3D;
    viewCoord.w = 0.0f;
    vec4 localMousePos = viewCoord * nearPlane * 1.1f;;
    localMousePos.w = 1;
    localMousePos.y = -localMousePos.y;
    vec4 worldMousePos = invViewMatrix * localMousePos;
    vec4 worldMouseDir = worldMousePos - invViewMatrix.position;
    worldMouseDir = normalize(worldMouseDir);
    worldMouseDir *= length;
    return line(worldMousePos.vec, (worldMousePos + worldMouseDir).vec);
}
//------------------------------------------------------------------------------
/**
    Utility function which computes a ray that extends in world space between mouse
    position on the near plane and length in cameras z direction
    Mouse position is expected in the range 0..1.

    @todo   There are better ways to do this!
*/
line
MouseRayUtil::ComputeWorldMouseRayOrtho(const vec2& mousePos, float length, const mat4& invViewMatrix, const mat4& invProjMatrix, float nearPlane)
{
    vec4 screenCoord3D((mousePos.x - 0.5f) * 2.0f, (mousePos.y - 0.5f) * 2.0f, 1.0f, 1.0f);
    vec4 viewCoord = invProjMatrix * screenCoord3D;
    viewCoord.w = 0.0f;
    vec4 localMousePos = viewCoord;
    localMousePos.w = 1;
    localMousePos.y = -localMousePos.y;
    vec4 worldMousePos = invViewMatrix * localMousePos;
    vec4 worldMouseDir = invViewMatrix.z_axis;
    worldMouseDir *= length;
    return line(worldMousePos.vec, (worldMousePos + worldMouseDir).vec);
}

} // namespace RenderUtil
