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

    @see Editor::ViewportPanel
*/
//------------------------------------------------------------------------------
class GraphicsViewportPanel final : public Editor::ViewportPanel
{
    __DeclareClass(GraphicsViewportPanel)
public:
    GraphicsViewportPanel();
    virtual ~GraphicsViewportPanel();

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
};

/// register the panel, the shapes and the key callback
void RegisterEditorContributions();

/// the shapes and the key callback are taken back when the feature stops
void ClearEditorContributions();

} // namespace GraphicsFeature
#endif // WITH_NEBULA_EDITOR
//------------------------------------------------------------------------------
