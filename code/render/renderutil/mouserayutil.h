#pragma once
//------------------------------------------------------------------------------
/**
    @class RenderUtil::MouseRayUtil

    Helper class to compute a world-space ray from mouse coords.

    (C) 2009 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "math/vec2.h"
#include "math/mat4.h"
#include "math/line.h"

//------------------------------------------------------------------------------
namespace RenderUtil
{
class MouseRayUtil
{
public:
    /// compute world-space ray from mouse position (mouse position is expected in the range 0..1)
    static Math::line ComputeWorldMouseRay(const Math::vec2& mousePos, float length, const Math::mat4& invViewMatrix, const Math::mat4& invProjMatrix, float nearPlane);
    static Math::line ComputeWorldMouseRayOrtho(const Math::vec2& mousePos, float length, const Math::mat4& invViewMatrix, const Math::mat4& invProjMatrix, float nearPlane);
};

} // namespace RenderUtil
//------------------------------------------------------------------------------

