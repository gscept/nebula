#pragma once
//------------------------------------------------------------------------------
/**
    @class GraphicsFeature::AssetPreview
    The 3D preview the asset documents draw what they edit in: a material on a knob, a model, a
    particle system. One preview is shared by all of them, because only the tab in front shows it. A
    document puts its objects in the preview's stage and takes them out of it when its tab goes
    behind another one, so the preview shows the front tab's objects only.

    The preview renders its own view with the preview frame script, lit by a directional light of
    its own, into a fixed size target that is drawn stretched over the space it is given. The camera's
    aspect is that space's, so what is shown keeps its proportions. The camera orbits a pivot: a
    left drag swings around it, a middle drag slides across, and the wheel moves in and out. The
    View button in the corner switches to an orthographic projection, looks from a side, and shades
    the preview as a wireframe, like the viewport's View popup.

    The view only renders while a document draws the preview, it is turned off when the document's
    tab goes behind another one.

    The render contexts register their compute subgraphs for the game's script only, so the
    preview's light and decal passes do nothing. The directional light is set per view, which is what
    lights the preview.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#if WITH_NEBULA_EDITOR
#include "core/refcounted.h"
#include "coregraphics/mesh.h"
#include "coregraphics/texture.h"
#include "graphics/graphicsentity.h"
#include "graphics/view.h"
#include "math/bbox.h"
#include "neweditor/viewtools/editorcamera.h"
#include "resources/resourceid.h"
#include "imgui.h"
#include <functional>

namespace GraphicsFeature
{

class AssetPreview : public Core::RefCounted
{
    __DeclareClass(AssetPreview)
public:
    /// the stage the preview's objects are drawn in, nothing of the scene is in it
    static const Graphics::StageMask Stage = 1 << 3;

    AssetPreview();
    virtual ~AssetPreview();

    /// the preview, made the first time a document asks for it
    static AssetPreview* Get();
    /// the preview if there is one, without making it. What only hides it asks this
    static AssetPreview* Peek();
    /// let go of the preview, when the graphics feature stops
    static void Destroy();

    /// what a document draws over the preview: the draw list, the camera's view projection and the
    /// image's corners
    using Overlay = std::function<void(ImDrawList* drawList, const Math::mat4& viewProjection, const ImVec2& min, const ImVec2& max)>;

    /// draw the preview at the given size in the current window, and take the camera gestures over it
    void Draw(const ImVec2& size, const Overlay& overlay = nullptr);
    /// stop rendering, the document that showed the preview went behind another tab
    void Hide();
    /// look at a box from where the camera is turned to, far enough away to see all of it
    void Frame(const Math::bbox& box);
    /// the knob materials are shown on
    CoreGraphics::MeshId GetMaterialKnob();
    /// the view the preview renders, the wireframe pass asks which one it draws for
    Graphics::ViewId GetView() const;
    /// how the preview shades: 0 lit, 1 wireframe, 2 the wireframe over the lit scene
    int GetShading() const;

private:
    /// make the view, the target, the camera and the light
    bool Setup();
    /// release them
    void Discard();
    /// the View button over the image's corner and its popup: the projection, the sides to look
    /// from and the shading
    void DrawViewMenu(const ImVec2& min);

    static Ptr<AssetPreview> instance;

    Graphics::ViewId view;
    Graphics::GraphicsEntityId camera;
    Graphics::GraphicsEntityId light;
    CoreGraphics::TextureId target;
    Ids::Id32 textureId;
    bool hasTextureId;
    Resources::ResourceId knob;
    Editor::EditorCamera orbit;
    bool enabled;
    float fieldOfView;
    bool orthographic;
    int shading;
};

} // namespace GraphicsFeature
#endif // WITH_NEBULA_EDITOR
//------------------------------------------------------------------------------
