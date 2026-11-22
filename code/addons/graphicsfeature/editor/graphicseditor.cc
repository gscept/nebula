//------------------------------------------------------------------------------
//  @file graphicsfeature/editor/graphicseditor.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "graphicsfeature/editor/graphicseditor.h"
#include "graphicsfeature/editor/graphicsprofilerpanel.h"

#if WITH_NEBULA_EDITOR
#include "graphicsfeature/components/model.h"
#include "graphics/graphicsserver.h"
#include "graphics/cameracontext.h"
#include "lighting/lightcontext.h"
#include "models/modelcontext.h"
#include "visibility/visibilitycontext.h"
#include "dynui/im3d/im3dcontext.h"
#include "dynui/imguicontext.h"
#include "math/rectangle.h"
#include "neweditor/api/editorfeature.h"
#include "neweditor/api/gizmo.h"
#include "neweditor/api/shell.h"
#include "frame/editorview.h"
#include "imgui.h"

namespace GraphicsFeature
{

__ImplementClass(GraphicsFeature::GraphicsViewportPanel, 'GVVP', Editor::ViewportPanel);

// The name the frame script inspector shows this view under
static const Util::StringAtom EditorViewName("editorview");

// The planes the panel's camera uses. The frame script reads them from the camera settings
static const float NearPlane = 0.01f;
static const float FarPlane = 1000.0f;

//------------------------------------------------------------------------------
/**
    The editor's words for how a shape is drawn, put on the frame script's flags.
*/
uint32_t
Im3dFlags(int flags)
{
    uint32_t renderFlags = Im3d::CheckDepth;
    if (flags & Editor::Gizmo::DrawFlag_Wireframe)
        renderFlags |= Im3d::Wireframe;
    if (flags & Editor::Gizmo::DrawFlag_AlwaysOnTop)
        renderFlags |= Im3d::AlwaysOnTop;
    return renderFlags;
}

//------------------------------------------------------------------------------
/**
*/
void
DrawLine(const Math::vec3& from, const Math::vec3& to, const Math::vec4& color, float width, int flags)
{
    Im3d::Im3dContext::DrawLine(Math::line(from, to), width, color, Im3dFlags(flags));
}

//------------------------------------------------------------------------------
/**
    A box is drawn as its edges at the line width the frame script uses, so the width the editor gives
    is not used here.
*/
void
DrawBox(const Math::bbox& box, const Math::vec4& color, float width, int flags)
{
    Im3d::Im3dContext::DrawBox(box, color, Im3dFlags(flags));
}

//------------------------------------------------------------------------------
/**
*/
void
DrawPoint(const Math::vec3& position, const Math::vec4& color, float size, int flags)
{
    Im3d::Im3dContext::DrawPoint(position, size, color, Im3dFlags(flags));
}

//------------------------------------------------------------------------------
/**
    The box around what the frame script draws for a model. This is the local box, which is what the
    editor puts where the entity stands.
*/
bool
ModelBounds(Game::World* world, Game::Entity entity, Math::bbox& out)
{
    if (world == nullptr || entity == Game::Entity::Invalid())
        return false;

    if (!world->HasComponent<GraphicsFeature::Model>(entity))
        return false;

    const GraphicsFeature::Model& model = world->GetComponent<GraphicsFeature::Model>(entity);
    if (model.graphicsEntityId == uint(-1))
        return false;

    out = Models::ModelContext::ComputeBoundingBox(Graphics::GraphicsEntityId(model.graphicsEntityId));
    return true;
}

//------------------------------------------------------------------------------
/**
    The keys a text field did not take. The shell is read when a key arrives, because the editor opens
    after this feature attaches.
*/
bool
ForwardKey(Input::Key::Code key, bool isPress, Input::Modifier::Code mods)
{
    Editor::Shell* shell = Editor::EditorFeatureRegistry::GetShell();
    if (shell == nullptr)
        return false;

    return shell->OnKey(key, isPress, mods);
}

//------------------------------------------------------------------------------
/**
*/
GraphicsViewportPanel::GraphicsViewportPanel() :
    view(Graphics::InvalidViewId),
    camera(Graphics::InvalidGraphicsEntityId),
    directionalLight(Graphics::InvalidGraphicsEntityId),
    textureId(0),
    scriptWidth(0),
    scriptHeight(0)
{
}

//------------------------------------------------------------------------------
/**
*/
GraphicsViewportPanel::~GraphicsViewportPanel()
{
    if (this->view != Graphics::InvalidViewId && Graphics::GraphicsServer::HasInstance())
        Graphics::GraphicsServer::Instance()->DiscardView(this->view);

    if (this->camera != Graphics::InvalidGraphicsEntityId)
    {
        if (Graphics::CameraContext::IsEntityRegistered(this->camera))
        {
            Graphics::CameraContext::RemoveLODCamera(this->camera);
            Graphics::CameraContext::DeregisterEntityImmediate(this->camera);
        }

        if (Visibility::ObserverContext::IsEntityRegistered(this->camera))
            Visibility::ObserverContext::DeregisterEntityImmediate(this->camera);

        Graphics::DestroyEntity(this->camera);
    }

    if (this->directionalLight != Graphics::InvalidGraphicsEntityId)
    {
        if (Lighting::LightContext::IsEntityRegistered(this->directionalLight))
            Lighting::LightContext::DeregisterEntityImmediate(this->directionalLight);

        Graphics::DestroyEntity(this->directionalLight);
    }

    if (this->textureId != 0)
        Dynui::DeallocateImguiTextureId((Ids::Id32)this->textureId);
}

//------------------------------------------------------------------------------
/**
    The view, the camera and the light are made here, before the base prepares the panel, because the
    base asks for a target at once.

    The pipelines are set up after that first target, because the script sizes its textures there. The
    draw subgraphs are registered at the graphics feature's activation, which is before this.

    The icon face is added here too. ImGui needs a context for the atlas, and the context is made at
    the graphics feature's activation, which is before this.
*/
bool
GraphicsViewportPanel::OnSetup()
{
    if (!Graphics::GraphicsServer::HasInstance())
        return false;

    this->view = Graphics::GraphicsServer::Instance()->CreateView(EditorViewName, FrameScript_editorview::Run,
        Math::rectangle<int>(0, 0, this->InitialTargetWidth, this->InitialTargetHeight),
        Graphics::PRIMARY_STAGE_MASK);
    if (this->view == Graphics::InvalidViewId)
        return false;

    this->camera = Graphics::CreateEntity();
    Graphics::CameraContext::RegisterEntity(this->camera);
    Graphics::CameraContext::AddLODCamera(this->camera);
    Visibility::ObserverContext::RegisterEntity(this->camera);
    Visibility::ObserverContext::Setup(this->camera, Visibility::VisibilityEntityType::Camera,
        Graphics::PRIMARY_STAGE_MASK);
    Graphics::ViewSetCamera(this->view, this->camera);

    this->directionalLight = Graphics::GraphicsServer::Instance()->CreateGraphicsEntity();
    Lighting::LightContext::RegisterEntity(this->directionalLight);
    Lighting::LightContext::SetupDirectionalLight(this->directionalLight,
        {
            .view = this->view,
            .color = Math::vec3(1.0f),
            .intensity = 50.0f,
            .zenith = Math::deg2rad(70.0f),
            .azimuth = 0.0f,
            .stageMask = Graphics::PRIMARY_STAGE_MASK,
            .castShadows = false
        });

    // The shapes are drawn into this view, so the gizmo scale follows the canvas
    Im3d::Im3dContext::SetView(this->view);

    this->textureId = Dynui::AllocateImguiTextureId({});

    // The base asks for a target at once, which is where the script gets its size. The pipelines are
    // set up after that, and after the graphics feature has registered the draw subgraphs, which is at
    // its activation
    const bool ok = Editor::ViewportPanel::OnSetup();
    if (!ok)
        return false;

    FrameScript_editorview::SetupPipelines();
    return true;
}

//------------------------------------------------------------------------------
/**
    The canvas size is given to the frame script here, because that is what sizes its textures. The
    output is the canvas, so the image is drawn at the size the panel asked for.

    The scene is drawn later in the frame, so the image the panel shows is the one the previous frame
    produced.
*/
ImTextureID
GraphicsViewportPanel::OnTarget(int width, int height)
{
    if (this->textureId == 0)
        return 0;

    if (width != this->scriptWidth || height != this->scriptHeight)
    {
        FrameScript_editorview::Initialize(width, height);
        this->scriptWidth = width;
        this->scriptHeight = height;
    }

    Graphics::ViewSetViewport(this->view, Math::rectangle<int>(0, 0, width, height));
    Graphics::ViewEnable(this->view);

    const CoreGraphics::TextureId output = FrameScript_editorview::Texture_EditorOutput();
    if (output == CoreGraphics::InvalidTextureId)
        return 0;

    Dynui::ImguiTextureId data;
    data.nebulaHandle = output;
    data.rangeMin = 0.0f;
    data.rangeMax = 0.0f;
    Dynui::SetImguiTextureIdData((Ids::Id32)this->textureId, data);

    return this->textureId;
}

//------------------------------------------------------------------------------
/**
    The frame script reads the camera from the context, so this is the camera the view runs with. The
    projection is built from the panel's field of view and the canvas aspect, which are the numbers the
    passed projection was built from, because the frame script reads a projection from the camera
    settings rather than from a matrix.
*/
void
GraphicsViewportPanel::OnRender(const Math::mat4& view, const Math::mat4& projection)
{
    if (this->camera == Graphics::InvalidGraphicsEntityId)
        return;

    const float aspect = float(this->TargetWidth()) / float(this->TargetHeight());

    Graphics::CameraContext::SetView(this->camera, view);
    Graphics::CameraContext::SetupProjectionFov(this->camera, aspect, Math::deg2rad(this->FieldOfView()),
        NearPlane, FarPlane, Graphics::PRIMARY_STAGE_MASK);
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsViewportPanel::OnLockMouse(bool locked)
{
    if (CoreGraphics::MainWindow == CoreGraphics::InvalidWindowId)
        return;

    CoreGraphics::WindowSetCursorLocked(CoreGraphics::MainWindow, locked);
    CoreGraphics::WindowSetCursorVisible(CoreGraphics::MainWindow, !locked);
}

//------------------------------------------------------------------------------
/**
    The engine has no way to put the cursor somewhere, so a drag that holds the cursor is not
    re-centered. It stops at the edge of the window.
*/
void
GraphicsViewportPanel::OnWarpMouse(const Math::vec2& pos)
{
}

//------------------------------------------------------------------------------
/**
    While the cursor is locked ImGui reports no position for it, so the movement is the one the display
    events have been giving the context.
*/
Math::vec2
GraphicsViewportPanel::OnMouseDelta() const
{
    const ImGuiIO& io = ImGui::GetIO();
    return Math::vec2(io.MouseDelta.x, io.MouseDelta.y);
}

//------------------------------------------------------------------------------
/**
    A view that is not shown would still be rendered, so it is turned off here. The game's view is the
    one that renders while the canvas is closed.
*/
void
GraphicsViewportPanel::OnCanvasClosed()
{
    if (this->view != Graphics::InvalidViewId)
        Graphics::ViewDisable(this->view);
}

//------------------------------------------------------------------------------
/**
    The panel is the editor's half, so this is only the render half: the shapes, the keys and the panel
    itself.
*/
void
RegisterEditorContributions()
{
    Editor::Gizmo::Interface interface;
    interface.line = Editor::Gizmo::LineDelegate::FromFunction<&DrawLine>();
    interface.box = Editor::Gizmo::BoxDelegate::FromFunction<&DrawBox>();
    interface.point = Editor::Gizmo::PointDelegate::FromFunction<&DrawPoint>();
    interface.bounds = Editor::Gizmo::BoundsDelegate::FromFunction<&ModelBounds>();
    Editor::Gizmo::Fill(interface);

    Dynui::ImguiContext::SetKeyCallback(
        Util::Delegate<bool(Input::Key::Code, bool, Input::Modifier::Code)>::FromFunction<&ForwardKey>());

    Editor::EditorFeatureRegistry::RegisterPanel(GraphicsViewportPanel::Create());
    Editor::EditorFeatureRegistry::RegisterPanel(GraphicsProfilerPanel::Create());
}

//------------------------------------------------------------------------------
/**
    Taken back here rather than by the editor, because the editor never knew who filled them.
*/
void
ClearEditorContributions()
{
    Editor::Gizmo::Clear();
    Dynui::ImguiContext::SetKeyCallback(Util::Delegate<bool(Input::Key::Code, bool, Input::Modifier::Code)>());
}

} // namespace GraphicsFeature
#endif // WITH_NEBULA_EDITOR
//------------------------------------------------------------------------------
