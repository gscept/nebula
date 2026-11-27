//------------------------------------------------------------------------------
//  @file assetpreview.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "graphicsfeature/editor/assets/assetpreview.h"

#if WITH_NEBULA_EDITOR
#include "coregraphics/meshresource.h"
#include "dynui/imguicontext.h"
// the generated script header needs the material types declared first
#include "materials/material.h"
#include "frame/preview.h"
#include "graphics/cameracontext.h"
#include "graphics/graphicsserver.h"
#include "lighting/lightcontext.h"
#include "resources/resourceserver.h"
#include "visibility/visibilitycontext.h"

namespace GraphicsFeature
{

__ImplementClass(GraphicsFeature::AssetPreview, 'GAPV', Core::RefCounted);

Ptr<AssetPreview> AssetPreview::instance;

// The target is drawn stretched over the space it is given, the camera's aspect keeps the proportions
static const int TargetSize = 1024;
static const float DefaultFieldOfView = 50.0f;
static const float NearPlane = 0.05f;
static const float FarPlane = 1000.0f;

//------------------------------------------------------------------------------
/**
*/
AssetPreview::AssetPreview() :
    view(Graphics::InvalidViewId),
    camera(Graphics::InvalidGraphicsEntityId),
    light(Graphics::InvalidGraphicsEntityId),
    target(CoreGraphics::InvalidTextureId),
    textureId(0),
    hasTextureId(false),
    knob(Resources::InvalidResourceId),
    enabled(false),
    fieldOfView(DefaultFieldOfView),
    orthographic(false),
    shading(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
AssetPreview::~AssetPreview()
{
    this->Discard();
}

//------------------------------------------------------------------------------
/**
*/
AssetPreview*
AssetPreview::Get()
{
    if (!AssetPreview::instance.isvalid())
    {
        Ptr<AssetPreview> preview = AssetPreview::Create();
        if (!preview->Setup())
            return nullptr;
        AssetPreview::instance = preview;
    }
    return AssetPreview::instance;
}

//------------------------------------------------------------------------------
/**
*/
AssetPreview*
AssetPreview::Peek()
{
    // a Ptr asserts when it is turned into a pointer while empty, and there is often no preview
    return AssetPreview::instance.isvalid() ? AssetPreview::instance.get() : nullptr;
}

//------------------------------------------------------------------------------
/**
*/
void
AssetPreview::Destroy()
{
    AssetPreview::instance = nullptr;
}

//------------------------------------------------------------------------------
/**
    The script is sized once, to the target, because the target is what the view renders into
    whatever space the preview is drawn in.
*/
bool
AssetPreview::Setup()
{
    if (!Graphics::GraphicsServer::HasInstance())
        return false;

    CoreGraphics::TextureCreateInfo texInfo;
    texInfo.name = "Asset Preview Target"_atm;
    texInfo.format = CoreGraphics::PixelFormat::SRGBA8;
    texInfo.width = TargetSize;
    texInfo.height = TargetSize;
    // copied to by the preview frame script and sampled by ImGui, cleared so it starts out in a readable layout
    texInfo.usage = CoreGraphics::TextureUsage::Sample | CoreGraphics::TextureUsage::TransferDestination;
    texInfo.clear = true;
    this->target = CoreGraphics::CreateTexture(texInfo);

    const CoreGraphics::TextureId targetTexture = this->target;
    this->view = Graphics::GraphicsServer::Instance()->CreateView("AssetPreview", FrameScript_preview::Run,
        Math::rectangle<int>(0, 0, TargetSize, TargetSize), Stage,
        [targetTexture](IndexT frameIndex, IndexT bufferIndex)
        {
            FrameScript_preview::Bind_Target(Frame::TextureImport(targetTexture));
        });
    if (this->view == Graphics::InvalidViewId)
        return false;
    FrameScript_preview::Initialize(TargetSize, TargetSize);
    FrameScript_preview::SetupPipelines();
    Graphics::ViewDisable(this->view);

    this->camera = Graphics::CreateEntity();
    Graphics::CameraContext::RegisterEntity(this->camera);
    Graphics::CameraContext::SetupProjectionFov(this->camera, 1.0f, Math::deg2rad(this->fieldOfView), NearPlane, FarPlane, Stage);
    Visibility::ObserverContext::RegisterEntity(this->camera);
    Visibility::ObserverContext::Setup(this->camera, Visibility::VisibilityEntityType::Camera, Stage);
    Graphics::ViewSetCamera(this->view, this->camera);

    this->light = Graphics::CreateEntity();
    Lighting::LightContext::RegisterEntity(this->light);
    Lighting::LightContext::SetupDirectionalLight(this->light,
        {
            .view = this->view,
            .color = Math::vec3(1.0f),
            .intensity = 50.0f,
            .zenith = Math::deg2rad(70.0f),
            .azimuth = 0.0f,
            .stageMask = Stage,
            .castShadows = false
        });

    Dynui::ImguiTextureId data;
    data.nebulaHandle = this->target;
    this->textureId = Dynui::AllocateImguiTextureId(data);
    this->hasTextureId = true;

    // looking at the origin from a little above, where a material knob or a small model sits
    this->orbit.rotx = Math::deg2rad(-20.0f);
    this->orbit.roty = Math::deg2rad(30.0f);
    this->orbit.Update(Editor::EditorCameraInput(), 0.0f);
    this->orbit.SetPivot(Math::vec3(0.0f), 3.0f);
    this->orbit.position = this->orbit.targetPosition;
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
AssetPreview::Discard()
{
    if (this->view != Graphics::InvalidViewId && Graphics::GraphicsServer::HasInstance())
    {
        Graphics::GraphicsServer::Instance()->DiscardView(this->view);
        this->view = Graphics::InvalidViewId;
    }
    if (this->camera != Graphics::InvalidGraphicsEntityId)
    {
        if (Graphics::CameraContext::IsEntityRegistered(this->camera))
            Graphics::CameraContext::DeregisterEntityImmediate(this->camera);
        if (Visibility::ObserverContext::IsEntityRegistered(this->camera))
            Visibility::ObserverContext::DeregisterEntityImmediate(this->camera);
        Graphics::DestroyEntity(this->camera);
        this->camera = Graphics::InvalidGraphicsEntityId;
    }
    if (this->light != Graphics::InvalidGraphicsEntityId)
    {
        if (Lighting::LightContext::IsEntityRegistered(this->light))
            Lighting::LightContext::DeregisterEntityImmediate(this->light);
        Graphics::DestroyEntity(this->light);
        this->light = Graphics::InvalidGraphicsEntityId;
    }
    if (this->hasTextureId)
    {
        Dynui::DeallocateImguiTextureId(this->textureId);
        this->hasTextureId = false;
    }
    if (this->target != CoreGraphics::InvalidTextureId)
    {
        CoreGraphics::DestroyTexture(this->target);
        this->target = CoreGraphics::InvalidTextureId;
    }
    if (this->knob != Resources::InvalidResourceId)
    {
        Resources::DiscardResource(this->knob);
        this->knob = Resources::InvalidResourceId;
    }
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::MeshId
AssetPreview::GetMaterialKnob()
{
    if (this->knob == Resources::InvalidResourceId)
    {
        // a system mesh, it is there when the editor is, so it is loaded right away
        this->knob = Resources::CreateResource(":material_knob:msh"_path, "preview", nullptr, nullptr, { .immediate = true, .stream = false });
    }
    return CoreGraphics::MeshResourceGetMesh(CoreGraphics::MeshResourceId(this->knob), 0);
}

//------------------------------------------------------------------------------
/**
*/
void
AssetPreview::Draw(const ImVec2& size, const Overlay& overlay)
{
    const ImVec2 area(Math::max(size.x, 1.0f), Math::max(size.y, 1.0f));

    if (!this->enabled)
    {
        Graphics::ViewEnable(this->view);
        this->enabled = true;
    }

    // the image is an item of its own, the gestures are read from it. The View button goes over it
    // afterwards and takes its clicks
    ImGui::SetNextItemAllowOverlap();
    ImGui::InvisibleButton("##assetpreview", area, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonMiddle);
    const ImVec2 min = ImGui::GetItemRectMin();
    const ImVec2 max = ImGui::GetItemRectMax();
    const bool hovered = ImGui::IsItemHovered();
    const bool active = ImGui::IsItemActive();
    const ImGuiIO& io = ImGui::GetIO();

    Editor::EditorCameraInput input;
    if (active && ImGui::IsMouseDown(ImGuiMouseButton_Left))
        input.orbit = Math::vec2(io.MouseDelta.x, io.MouseDelta.y);
    if (active && ImGui::IsMouseDown(ImGuiMouseButton_Middle))
        input.pan = Math::vec2(io.MouseDelta.x, io.MouseDelta.y);
    if (hovered && io.MouseWheel != 0.0f)
    {
        // a step that is a part of the distance, so it feels the same close up and far away
        input.zoom = io.MouseWheel * Math::max(this->orbit.distance * 0.1f, 0.05f);
    }
    this->orbit.Update(input, io.DeltaTime);

    Graphics::CameraContext::SetView(this->camera, this->orbit.view);
    const float aspect = area.x / area.y;
    if (this->orthographic)
    {
        // the height the field of view shows at the pivot, so switching keeps what is framed there
        const float height = 2.0f * this->orbit.distance * Math::tan(Math::deg2rad(this->fieldOfView) * 0.5f);
        Graphics::CameraContext::SetupOrthographic(this->camera, height * aspect, height, NearPlane, FarPlane, Stage);
    }
    else
    {
        Graphics::CameraContext::SetupProjectionFov(this->camera, aspect, Math::deg2rad(this->fieldOfView), NearPlane, FarPlane, Stage);
    }

    // the scene renders later in the frame, so the image is the one the previous frame produced
    ImGui::GetWindowDrawList()->AddImage(ImTextureRef(this->textureId), min, max);
    if (overlay)
        overlay(ImGui::GetWindowDrawList(), Graphics::CameraContext::GetViewProjection(this->camera), min, max);

    // the button sits over the image's corner, and the layout goes on below the image
    const ImVec2 after = ImGui::GetCursorScreenPos();
    this->DrawViewMenu(min);
    // ImGui wants an item where the cursor is put back, the layout goes on from it
    ImGui::SetCursorScreenPos(after);
    ImGui::Dummy(ImVec2(0.0f, 0.0f));
}

//------------------------------------------------------------------------------
/**
*/
void
AssetPreview::DrawViewMenu(const ImVec2& min)
{
    const ImGuiStyle& style = ImGui::GetStyle();
    ImGui::SetCursorScreenPos(ImVec2(min.x + style.FramePadding.x, min.y + style.FramePadding.y));
    ImGui::PushID("assetpreviewview");
    if (ImGui::SmallButton("View"))
        ImGui::OpenPopup("##previewview");
    ImGui::SetItemTooltip("Projection, the side to look from and the shading");
    if (ImGui::BeginPopup("##previewview"))
    {
        ImGui::SeparatorText("Projection");
        if (ImGui::RadioButton("Perspective", !this->orthographic))
            this->orthographic = false;
        ImGui::SameLine();
        if (ImGui::RadioButton("Orthographic", this->orthographic))
            this->orthographic = true;
        ImGui::SetNextItemWidth(180.0f);
        ImGui::SliderFloat("Field of view", &this->fieldOfView, 10.0f, 150.0f, "%.0f deg");

        ImGui::SeparatorText("Look from");
        struct Side { const char* name; Math::vec3 direction; };
        static const Side sides[6] =
        {
            { "Top", Math::vec3(0, 1, 0) }, { "Front", Math::vec3(0, 0, 1) }, { "Right", Math::vec3(1, 0, 0) },
            { "Bottom", Math::vec3(0, -1, 0) }, { "Back", Math::vec3(0, 0, -1) }, { "Left", Math::vec3(-1, 0, 0) },
        };
        for (int i = 0; i < 6; i++)
        {
            if (i % 3 != 0)
                ImGui::SameLine();
            if (ImGui::Button(sides[i].name, ImVec2(60.0f, 0.0f)))
                this->orbit.LookFrom(sides[i].direction);
        }

        ImGui::SeparatorText("Shading");
        ImGui::RadioButton("Lit", &this->shading, 0);
        ImGui::SameLine();
        ImGui::RadioButton("Wireframe", &this->shading, 1);
        ImGui::SameLine();
        ImGui::RadioButton("Lit wireframe", &this->shading, 2);
        ImGui::EndPopup();
    }
    ImGui::PopID();
}

//------------------------------------------------------------------------------
/**
*/
Graphics::ViewId
AssetPreview::GetView() const
{
    return this->view;
}

//------------------------------------------------------------------------------
/**
*/
int
AssetPreview::GetShading() const
{
    return this->shading;
}

//------------------------------------------------------------------------------
/**
*/
void
AssetPreview::Hide()
{
    if (this->enabled)
    {
        Graphics::ViewDisable(this->view);
        this->enabled = false;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
AssetPreview::Frame(const Math::bbox& box)
{
    const Math::vec3 center = xyz(box.center());
    const Math::vector extents = box.extents();
    const float radius = Math::max(length(Math::vec3(extents.x, extents.y, extents.z)), 0.1f);
    // far enough that a sphere around the box fits the field of view
    const float distance = radius / Math::sin(Math::deg2rad(this->fieldOfView) * 0.5f);
    this->orbit.SetPivot(center, distance);
    this->orbit.position = this->orbit.targetPosition;
}

} // namespace GraphicsFeature
#endif // WITH_NEBULA_EDITOR
