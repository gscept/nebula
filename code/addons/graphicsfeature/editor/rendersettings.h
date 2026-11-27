#pragma once
//------------------------------------------------------------------------------
/**
    @file rendersettings.h
    How the editor's viewport draws the scene, the render side of its View popup.

    - shading: lit, wireframe over a dark background, or the wireframe over the lit scene. The
      wireframe is drawn by the game's frame script, so it shows in the Rendering view
    - depth of field on the viewport's camera, which the game's frame script applies, so it shows in
      the Rendering view as well

    The settings are the editor's, they are not saved with a scene.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#if WITH_NEBULA_EDITOR
#include "graphics/graphicsentity.h"

namespace GraphicsFeature
{

/// add the settings to the viewport's View popup and the wireframe to the game's frame script
void RegisterRenderSettings();
/// load the wireframe shader and draw it in the game's frame script, once the graphics server is up
void SetupWireframe();
/// take them back
void ClearRenderSettings();
/// put the camera effects on the viewport's camera, every frame the viewport renders
void ApplyCameraEffects(Graphics::GraphicsEntityId camera);

} // namespace GraphicsFeature
#endif // WITH_NEBULA_EDITOR
//------------------------------------------------------------------------------
