#pragma once
//------------------------------------------------------------------------------
/**
    The graphics feature's editor contribution.

    @see Editor::EditorFeatureRegistry

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#if WITH_NEBULA_EDITOR
#include "game/entity.h"
#include "game/world.h"
#include "graphics/graphicsentity.h"
#include "graphics/view.h"
#include "imgui.h"
#include "math/bbox.h"
#include "math/mat4.h"
#include "math/vec2.h"
#include "math/vec3.h"
#include "math/vec4.h"
#include "neweditor/panels/viewportpanel.h"

namespace GraphicsFeature
{

//------------------------------------------------------------------------------
/**
    @class GraphicsFeature::GraphicsViewportPanel

    The render half of the editor's scene view.

    The panel has its own view, because the game's view renders with the game's camera. The view runs
    the editor's frame script, which draws the scene at the canvas size and writes the gizmos into the
    same output. The panel shows that output as an image.

    The scene data the material batches read is the game view's, because the render contexts register
    their compute subgraphs for the game's script instance only.

    The Rendering view mode shows the game's own frame script instead, drawn with the panel's camera:
    the full pipeline with shadows, fog, post effects and depth of field, which the editor's script
    leaves out. The game's view lends its camera slot for as long as the mode is shown, and the image
    is its scene buffer, which is the frame before the UI is drawn into it. The game's script keeps
    the window's size, so the canvas shows its whole image squeezed to the canvas; the projection has
    the canvas' aspect, which is what makes it come out right.

    @see Editor::ViewportPanel
*/
//------------------------------------------------------------------------------
class GraphicsViewportPanel final : public Editor::ViewportPanel
{
    __DeclareClass(GraphicsViewportPanel)
public:
    GraphicsViewportPanel();
    virtual ~GraphicsViewportPanel();

    /// the light the editor's view draws the scene with, the environment panel moves it with the sun.
    /// Invalid while there is no panel
    static Graphics::GraphicsEntityId GetViewportLight();
    /// the camera the panel drives, the camera effects are set on it
    static Graphics::GraphicsEntityId GetViewportCamera();
    /// true while the canvas shows the game's frame script, where the shading modes and the camera
    /// effects show
    static bool ShowsFullPipeline();

protected:
    bool OnSetup() override;

    ImTextureID OnTarget(int width, int height) override;
    void OnRender(const Math::mat4& view, const Math::mat4& projection) override;
    void OnLockMouse(bool locked) override;
    void OnWarpMouse(const Math::vec2& pos) override;
    Math::vec2 OnMouseDelta() const override;
    void OnCanvasClosed() override;

private:
    /// the view this panel renders with
    Graphics::ViewId view;

    /// the camera entity the panel drives
    Graphics::GraphicsEntityId camera;

    /// the light for this view, because the scene batches read the light list of the view they draw
    /// for
    Graphics::GraphicsEntityId directionalLight;

    /// the id the canvas knows the output under. Made once, filled every frame
    ImTextureID textureId;

    /// the size the frame script has sized its textures to
    int scriptWidth;
    int scriptHeight;

    /// show the game's frame script with the panel's camera, or give the game's view its camera back
    void ShowGameScript(bool show);
    /// the game's view while it draws with the panel's camera, and the camera it had
    bool showsGameScript;
    Graphics::GraphicsEntityId gameViewCamera;

    static GraphicsViewportPanel* instance;
};

/// register the panel, the shapes and the key callback
void RegisterEditorContributions();

/// the transform processors that keep the render contexts in step while the world is in editor mode.
/// Called once the world exists, at activate
void RegisterEditorProcessors();
/// the shapes and the key callback are taken back when the feature stops
void ClearEditorContributions();

} // namespace GraphicsFeature
#endif // WITH_NEBULA_EDITOR
//------------------------------------------------------------------------------
