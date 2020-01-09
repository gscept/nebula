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
MouseRayUtil::ComputeWorldMouseRay(const float2& mousePos, float length, const matrix44& invViewMatrix, const matrix44& invProjMatrix, float nearPlane)
{
    // Compute mouse position in world coordinates.
    point screenCoord3D((mousePos.x() - 0.5f) * 2.0f, (mousePos.y() - 0.5f) * 2.0f, 1.0f);
    vector viewCoord = matrix44::transform(screenCoord3D, invProjMatrix);
    point localMousePos = viewCoord * nearPlane * 1.1f;
    localMousePos.y() = -1 * localMousePos.y();
    point worldMousePos = matrix44::transform(localMousePos, invViewMatrix);
    vector worldMouseDir = worldMousePos - point(invViewMatrix.get_position());
    worldMouseDir = vector::normalize(worldMouseDir);
    worldMouseDir *= length;

    return line(worldMousePos, worldMousePos + worldMouseDir);
}

} // namespace RenderUtil
