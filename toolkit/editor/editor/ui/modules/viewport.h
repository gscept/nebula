#pragma once
//------------------------------------------------------------------------------
/**
    Viewport module

    Shows a viewport, including rendering settings, camera settings etc.

    (C) 2019 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "dynui/imguicontext.h"
#include "editor/tools/camera.h"

namespace Graphics{ class Stage; class View; }

namespace Presentation
{

namespace Modules
{

class Viewport
{
public:
    enum RenderMode
    {
        TexturedLit,
        TexturedUnlit,
        SolidLit,
        SolidUnlit,
        /// only the mesh edges, the shaded scene is hidden
        Wireframe,
        /// the shaded scene with the mesh edges drawn on top
        LitWireframe,
    };

    Viewport();
    ~Viewport();
    
    void Init(Util::String const& viewName, const Graphics::StageMask mask = Graphics::PRIMARY_STAGE_MASK);
    void Init(const Graphics::ViewId view);

    /// render the viewport and its menus, the current window needs ImGuiWindowFlags_MenuBar for the menus to show
    void Render();
    /// render the viewport with its menus in a child window filling the remaining space, for windows without a menu bar.
    /// The overlay draws on top of the image, with the draw list of the child window, lastViewportImagePositionAbsolute
    /// and lastViewportImageSizeAbsolute give the image rectangle
    void RenderInChildWindow(const std::function<void(ImDrawList* drawList)>& overlay = nullptr);
    void SetStage(const uint16_t stage);
    void SetFrameBuffer(Util::String const& name);

    void Update();

    bool IsFocused() const
    {
        return this->focused;
    }

    const Graphics::ViewId GetView() const;

    /// get the render mode
    RenderMode GetRenderMode() const
    {
        return this->renderMode;
    }
    /// set the render mode
    void SetRenderMode(const RenderMode mode)
    {
        this->renderMode = mode;
    }
    /// get the render mode of the viewport showing a view, TexturedLit if no viewport shows it
    static RenderMode GetRenderMode(const Graphics::ViewId view);

    Editor::Camera camera;
    Math::vec2 viewportScreenPosition;
    Math::vec2 lastViewportImagePositionAbsolute;
    Math::vec2 lastViewportImageSizeAbsolute;
    /// the latest actual position of the viewport. This is in normalized space (0...1)
    Math::vec2 lastViewportImagePosition;
    /// the latest actual size of the viewport. This is in normalized space (0...1)
    Math::vec2 lastViewportImageSize;

private:
    RenderMode renderMode = TexturedLit;

    Graphics::ViewId view = Graphics::InvalidViewId;
    Graphics::GraphicsEntityId directionalLight;

    CoreGraphics::TextureId targetTexture;
    Resources::ResourceId resourceId;
    Ids::Id32 imguiTextureId;
    bool focused = false;
    bool showWorldAxis = false;

    Util::String frameBuffer;
};

} // namespace Modules

} // namespace Presentation
