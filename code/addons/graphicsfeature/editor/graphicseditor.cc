//------------------------------------------------------------------------------
//  @file graphicsfeature/editor/graphicseditor.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "graphicsfeature/editor/graphicseditor.h"
#include "graphicsfeature/editor/graphicsprofilerpanel.h"

#if WITH_NEBULA_EDITOR
#include "graphicsfeature/components/model.h"
#include "graphicsfeature/components/decal.h"
#include "graphicsfeature/components/lighting.h"
#include "graphicsfeature/components/gi.h"
#include "basegamefeature/components/position.h"
#include "basegamefeature/components/orientation.h"
#include "basegamefeature/components/scale.h"
#include "game/processor.h"
#include "decals/decalcontext.h"
#include "coregraphics/resourcetable.h"
#include "gi/ddgicontext.h"
#include "io/assignregistry.h"
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
#include "editorassets/assettype.h"
#include "graphicsfeature/editor/assets/texturedocument.h"
#include "graphicsfeature/editor/assets/materialdocument.h"
#include "graphicsfeature/editor/assets/modeldocument.h"
#include "graphicsfeature/editor/assets/assetpreview.h"
#include "editorassets/asseteditorpanel.h"
#include "editorassets/importpanel.h"
#include "frame/editorview.h"
#include "frame/default.h"
#include "graphicsfeature/graphicsfeatureunit.h"
#include "graphicsfeature/editor/rendersettings.h"
#include "graphicsfeature/editor/environmentpanel.h"
#include "graphicsfeature/editor/resourcespanel.h"
#include "graphicsfeature/editor/terrain/terraineditor.h"
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

    // ComputeBoundingBox answers in world space, and the editor puts the box where the entity stands,
    // so it is taken back through the model's transform first. Without this an entity away from the
    // origin is picked and outlined at twice its position. The inverse is affine only up to float
    // error in its last column, which the affine transform warns about every frame, so the box goes
    // through the general one
    const Graphics::GraphicsEntityId gid(model.graphicsEntityId);
    Math::bbox box = Models::ModelContext::ComputeBoundingBox(gid);
    if (box.pmin == box.pmax)
    {
        return false;   // no nodes yet, the model is still loading
    }
    box.transform(Math::inverse(Models::ModelContext::GetTransform(gid)));
    out = box;
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
GraphicsViewportPanel* GraphicsViewportPanel::instance = nullptr;

//------------------------------------------------------------------------------
/**
*/
GraphicsViewportPanel::GraphicsViewportPanel() :
    view(Graphics::InvalidViewId),
    camera(Graphics::InvalidGraphicsEntityId),
    directionalLight(Graphics::InvalidGraphicsEntityId),
    textureId(0),
    scriptWidth(0),
    scriptHeight(0),
    showsGameScript(false),
    gameViewCamera(Graphics::InvalidGraphicsEntityId)
{
    GraphicsViewportPanel::instance = this;
}

//------------------------------------------------------------------------------
/**
*/
Graphics::GraphicsEntityId
GraphicsViewportPanel::GetViewportLight()
{
    return GraphicsViewportPanel::instance != nullptr ? GraphicsViewportPanel::instance->directionalLight : Graphics::InvalidGraphicsEntityId;
}

//------------------------------------------------------------------------------
/**
*/
Graphics::GraphicsEntityId
GraphicsViewportPanel::GetViewportCamera()
{
    return GraphicsViewportPanel::instance != nullptr ? GraphicsViewportPanel::instance->camera : Graphics::InvalidGraphicsEntityId;
}

//------------------------------------------------------------------------------
/**
*/
bool
GraphicsViewportPanel::ShowsFullPipeline()
{
    return GraphicsViewportPanel::instance != nullptr && GraphicsViewportPanel::instance->showsGameScript;
}

//------------------------------------------------------------------------------
/**
    The game's view draws with the panel's camera while the mode is shown, and the editor's view,
    which would draw the same scene again, is off. The shapes go to the view that is shown
*/
void
GraphicsViewportPanel::ShowGameScript(bool show)
{
    if (show == this->showsGameScript)
        return;
    const Graphics::ViewId gameView = GraphicsFeatureUnit::Instance()->GetDefaultView();
    if (gameView == Graphics::InvalidViewId)
        return;

    if (show)
    {
        this->gameViewCamera = Graphics::ViewGetCamera(gameView);
        Graphics::ViewSetCamera(gameView, this->camera);
        if (this->view != Graphics::InvalidViewId)
            Graphics::ViewDisable(this->view);
        Im3d::Im3dContext::SetView(gameView);
    }
    else
    {
        Graphics::ViewSetCamera(gameView, this->gameViewCamera);
        this->gameViewCamera = Graphics::InvalidGraphicsEntityId;
        Im3d::Im3dContext::SetView(this->view);
    }
    this->showsGameScript = show;
}

//------------------------------------------------------------------------------
/**
*/
GraphicsViewportPanel::~GraphicsViewportPanel()
{
    // the game's view gets its camera back before the camera goes
    if (Graphics::GraphicsServer::HasInstance() && GraphicsFeatureUnit::HasInstance())
        this->ShowGameScript(false);
    if (GraphicsViewportPanel::instance == this)
        GraphicsViewportPanel::instance = nullptr;

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
    SetupWireframe();
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

    // the full pipeline is the game's script, at the window's size
    const bool full = Util::String(this->GetViewModeName()) == "rendering";
    this->ShowGameScript(full);
    if (full)
    {
        const CoreGraphics::TextureId scene = FrameScript_default::Texture_SceneBuffer();
        if (scene == CoreGraphics::InvalidTextureId)
            return 0;
        Dynui::ImguiTextureId data;
        data.nebulaHandle = scene;
        data.rangeMin = 0.0f;
        data.rangeMax = 0.0f;
        Dynui::SetImguiTextureIdData((Ids::Id32)this->textureId, data);
        return this->textureId;
    }

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
    // an orthographic projection has 1 in its last diagonal element, its size is in the first two
    if (projection.r[3].w != 0.0f)
        Graphics::CameraContext::SetupOrthographic(this->camera, 2.0f / projection.r[0].x, 2.0f / projection.r[1].y,
            NearPlane, FarPlane, Graphics::PRIMARY_STAGE_MASK);
    else
        Graphics::CameraContext::SetupProjectionFov(this->camera, aspect, Math::deg2rad(this->FieldOfView()),
            NearPlane, FarPlane, Graphics::PRIMARY_STAGE_MASK);
    ApplyCameraEffects(this->camera);
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
    this->ShowGameScript(false);
    if (this->view != Graphics::InvalidViewId)
        Graphics::ViewDisable(this->view);
}

//------------------------------------------------------------------------------
/**
    The asset types the render side owns, with the work file each is authored in. The export side
    of each mapping is the graphics server's, because a game loads the exported files without the
    editor. A type with an editor opens its documents in the asset editor.
*/
static void
RegisterAssetTypes()
{
    using OpenDelegate = Util::Delegate<void(const IO::Path&)>;
    using CreateDelegate = Util::Delegate<bool(const IO::Path&, const IO::URI&, const Util::String&, Util::String&)>;
    using VariantsDelegate = Util::Delegate<void(Util::Array<Util::String>&)>;
    // models and textures are imported from source files, the importer takes them in again
    using ReimportDelegate = Util::Delegate<bool(const IO::Path&, Util::String&)>;
    // Packaging a model writes its meshes, and for a model that has them its animation, skeleton and
    // physics, under the model's folder and file, so a field that takes one of those takes a model
    Editor::AssetTypeRegistry::Register({ .type = "mdl", .workExtension = "nasset", .name = "Model", .icon = "box",
        .exports = { "msh", "ani", "ske", "phys" },
        .open = OpenDelegate::FromFunction<&ModelDocument::OpenAsset>(),
        .reimport = ReimportDelegate::FromFunction<&Editor::ImportPanel::Reimport>() });
    Editor::AssetTypeRegistry::Register({ .type = "tex", .workExtension = "natex", .name = "Texture", .icon = "image",
        .open = OpenDelegate::FromFunction<&TextureDocument::OpenAsset>(),
        .reimport = ReimportDelegate::FromFunction<&Editor::ImportPanel::Reimport>() });
    Editor::AssetTypeRegistry::Register({ .type = "mat", .workExtension = "namat", .name = "Material", .icon = "palette",
        .variants = VariantsDelegate::FromFunction<&MaterialDocument::ListTemplates>(),
        .create = CreateDelegate::FromFunction<&MaterialDocument::CreateAsset>(),
        .open = OpenDelegate::FromFunction<&MaterialDocument::OpenAsset>() });
    Editor::AssetTypeRegistry::Register({ .type = "par", .workExtension = "napar", .name = "Particle", .icon = "sparkles" });
}

//------------------------------------------------------------------------------
/**
    A model asset dropped on the viewport is an entity that draws it
*/
static void
DropModel(Editor::EditorDroppedAsset& entity)
{
    GraphicsFeature::Model model;
    model.resource = entity.asset.AsString();
    entity.Add(Game::GetComponentId<GraphicsFeature::Model>(), &model, sizeof(model));
}

/// the types registered above, taken back when the feature stops
static const char* const AssetTypeNames[] = { "mdl", "tex", "mat", "par" };

//------------------------------------------------------------------------------
/**
    The panel is the editor's half, so this is only the render half: the shapes, the keys, the
    asset types and the panel itself.
*/
void
RegisterEditorContributions()
{
    // The editor's file browser and dialogs start in 'work:'. A Nebula project keeps that folder under
    // its root, which the old editor reached as 'proj:work/'
    if (!IO::AssignRegistry::Instance()->HasAssign("work"))
    {
        IO::AssignRegistry::Instance()->SetAssign(IO::Assign("work", "proj:work"));
    }

    Editor::Gizmo::Interface interface;
    interface.line = Editor::Gizmo::LineDelegate::FromFunction<&DrawLine>();
    interface.box = Editor::Gizmo::BoxDelegate::FromFunction<&DrawBox>();
    interface.point = Editor::Gizmo::PointDelegate::FromFunction<&DrawPoint>();
    interface.bounds = Editor::Gizmo::BoundsDelegate::FromFunction<&ModelBounds>();
    Editor::Gizmo::Fill(interface);

    Dynui::ImguiContext::SetKeyCallback(
        Util::Delegate<bool(Input::Key::Code, bool, Input::Modifier::Code)>::FromFunction<&ForwardKey>());

    RegisterAssetTypes();
    Editor::EditorFeatureRegistry::RegisterAssetDrop("mdl", Util::Delegate<void(Editor::EditorDroppedAsset&)>::FromFunction<&DropModel>());

    Editor::EditorFeatureRegistry::RegisterPanel(GraphicsViewportPanel::Create());
    Editor::EditorFeatureRegistry::RegisterPanel(GraphicsProfilerPanel::Create());
    Editor::EditorFeatureRegistry::RegisterPanel(EnvironmentPanel::Create());
    Editor::EditorFeatureRegistry::RegisterPanel(ResourcesPanel::Create());
    RegisterRenderSettings();
    TerrainPanel::Register();
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
    // the exit stops the features in the order they started, so the editor may still have these open.
    // Their documents hold models and textures, which go before the contexts they are registered in
    for (const char* type : AssetTypeNames)
    {
        Editor::AssetEditorPanel::CloseDocumentsOf(type);
        Editor::AssetTypeRegistry::Unregister(type);
    }
    AssetPreview::Destroy();
    ClearRenderSettings();
    TerrainPanel::Clear();
}

//------------------------------------------------------------------------------
/**
    The graphics manager's transform processors only run while the world plays, so a move the editor
    makes in editor mode never reaches the render contexts. These run in editor mode too. They are the
    old editor's, which had the same problem.
*/
void
RegisterEditorProcessors()
{
    Game::World* world = Game::GetWorld(WORLD_DEFAULT);

    Game::ProcessorBuilder(world, "GraphicsEditor.UpdateModelTransforms"_atm)
        .On("OnEndFrame")
        .OnlyModified()
        .RunInEditor()
        .Func(
            [](Game::World* world, Game::Position const& pos, Game::Orientation const& orient, Game::Scale const& scale, GraphicsFeature::Model const& model)
            {
                Models::ModelContext::SetTransform(model.graphicsEntityId, Math::trs(pos, orient, scale));
            }
        )
        .Build();

    Game::ProcessorBuilder(world, "GraphicsEditor.UpdatePointLightPositions"_atm)
        .On("OnEndFrame")
        .OnlyModified()
        .RunInEditor()
        .Func(
            [](Game::World* world, Game::Position const& pos, GraphicsFeature::PointLight const& light)
            {
                Lighting::LightContext::SetPosition(light.graphicsEntityId, pos);
            }
        )
        .Build();

    Game::ProcessorBuilder(world, "GraphicsEditor.UpdateSpotLightTransform"_atm)
        .On("OnEndFrame")
        .OnlyModified()
        .RunInEditor()
        .Func(
            [](Game::World* world, Game::Position const& pos, Game::Orientation const& rot, GraphicsFeature::SpotLight const& light)
            {
                Lighting::LightContext::SetPosition(light.graphicsEntityId, pos);
                Lighting::LightContext::SetRotation(light.graphicsEntityId, rot);
            }
        )
        .Build();

    Game::ProcessorBuilder(world, "GraphicsEditor.UpdateAreaLightTransform"_atm)
        .On("OnEndFrame")
        .OnlyModified()
        .RunInEditor()
        .Func(
            [](Game::World* world, Game::Position const& pos, Game::Orientation const& rot, Game::Scale const& scale, GraphicsFeature::AreaLight const& light)
            {
                Lighting::LightContext::SetPosition(light.graphicsEntityId, pos);
                Lighting::LightContext::SetRotation(light.graphicsEntityId, rot);
                Lighting::LightContext::SetScale(light.graphicsEntityId, scale);
            }
        )
        .Build();

    Game::ProcessorBuilder(world, "GraphicsEditor.UpdateDecalTransform"_atm)
        .On("OnEndFrame")
        .OnlyModified()
        .RunInEditor()
        .Func(
            [](Game::World* world, Game::Position const& pos, Game::Orientation const& rot, Game::Scale const& scale, GraphicsFeature::Decal const& decal)
            {
                Decals::DecalContext::SetTransform(decal.graphicsEntityId, Math::trs(pos, rot, scale));
            }
        )
        .Build();

    Game::ProcessorBuilder(world, "GraphicsEditor.UpdateDDGIVolumeTransform"_atm)
        .On("OnEndFrame")
        .OnlyModified()
        .RunInEditor()
        .Func(
            [](Game::World* world, Game::Position const& pos, Game::Orientation const& rot, Game::Scale const& scale, GraphicsFeature::DDGIVolume const& volume)
            {
                GI::DDGIContext::SetPosition(volume.graphicsEntityId, pos);
                GI::DDGIContext::SetSize(volume.graphicsEntityId, scale);
            }
        )
        .Build();
}

} // namespace GraphicsFeature
#endif // WITH_NEBULA_EDITOR
//------------------------------------------------------------------------------
