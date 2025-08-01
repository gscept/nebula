#pragma once
//------------------------------------------------------------------------------
/**
    Physics::DebugUI

    @copyright
    (C) 2020-2025 Individual contributors, see AUTHORS file
*/
#include "graphics/graphicsentity.h"
#include "util/color.h"
#include <functional>

namespace Physics
{

struct DebugDrawInterface
{
    std::function<void(Math::vec3 point, Util::Color color, float size)> DrawPoint = nullptr;
    std::function<void(Math::vec3 p0, Math::vec3 p1, Util::Color color0, Util::Color color1, float lineWidth)> DrawLine = nullptr;
    std::function<void(Math::vec3 p0, Math::vec3 p1, Math::vec3 p2, Util::Color color0, Util::Color color1, Util::Color color2, float triLineWidth)> DrawTriangle = nullptr;
};

void SetDebugDrawInterface(DebugDrawInterface const& interface);

void EnableDebugDrawing(bool enabled);

void RenderUI(Graphics::GraphicsEntityId camera);

void RenderMaterialsUI();

void DrawPhysicsDebug();

}