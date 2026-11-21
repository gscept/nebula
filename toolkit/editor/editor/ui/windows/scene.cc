//------------------------------------------------------------------------------
//  scene.cc
//  (C) 2021 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "scene.h"
#include "editor/editor.h"
#include "editor/commandmanager.h"
#include "editor/cmds.h"
#include "editor/ui/uimanager.h"
#include "graphicsfeature/graphicsfeatureunit.h"
#include "editor/ui/windowserver.h"
#include "editor/tools/selectioncontext.h"

#include "basegamefeature/components/position.h"
#include "graphicsfeature/components/model.h"
#include "graphicsfeature/components/lighting.h"
#include "graphicsfeature/components/terrain.h"
#include "physicsfeature/components/physicsfeature.h"
#include "models/modelcontext.h"
#include "dynui/im3d/im3dcontext.h"
#include "renderutil/mouserayutil.h"
#include "io/ioserver.h"
#include "imgui_internal.h"

#include "nflatbuffer/flatbufferinterface.h"
#include "nflatbuffer/nebula_flat.h"
#include "flat/model.h"

#include "editor/tools/selectiontool.h"
#include "editor/tools/navareapointtool.h"
#include "editor/tools/navpathtool.h"
#include "editor/tools/translatetool.h"
#include "editor/tools/rotatetool.h"
#include "editor/tools/scaletool.h"
#include "editor/ui/windows/terraineditor/terraineditor.h"

#include "input/keyboard.h"
#include "input/mouse.h"
#include "input/inputserver.h"

using namespace Editor;

namespace Presentation
{

//------------------------------------------------------------------------------
/**
*/
Scene::Scene()
{
    this->allTools[0] = new Tools::SelectionTool();
    this->allTools[1] = new Tools::TranslateTool();
    this->allTools[2] = new Tools::RotateTool();
    this->allTools[3] = new Tools::ScaleTool();
    this->allTools[4] = new Tools::TerrainEditorTool();
    this->allTools[5] = new Tools::NavAreaPointTool();
    this->allTools[6] = new Tools::NavPathTool();

    this->currentTool = this->allTools[0];

    this->viewPort.Init(GraphicsFeature::GraphicsFeatureUnit::Instance()->GetDefaultView());
    this->viewPort.SetFrameBuffer("ColorBufferNoGUI");

    this->SetWindowPadding({0, 0});

    Util::Delegate<void()> focusDelegate = Util::Delegate<void()>::FromMethod<Scene, &Scene::FocusCamera>(this);

    WindowServer::Instance()->RegisterCommand(focusDelegate, "Focus camera on current selection", "F", "Camera");

    this->additionalFlags = ImGuiWindowFlags_MenuBar;
}

//------------------------------------------------------------------------------
/**
*/
Scene::~Scene()
{
    delete this->allTools[0];
    delete this->allTools[1];
    delete this->allTools[2];
    delete this->allTools[3];
    delete this->allTools[4];
    delete this->allTools[5];
    delete this->allTools[6];
    this->currentTool = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
void
Scene::Update()
{
    this->viewPort.Update();
}

//------------------------------------------------------------------------------
/**
    Reads the mesh of a model asset straight from the flatbuffer without unpacking it. Returns false if
    the asset has no mesh data, or no shape in its scene that references a mesh. Otherwise the mesh
    URN and primitive group of the first shape are returned.
*/
static bool
FindAssetMesh(const IO::Path& asset, Util::String& outMeshResource, uint& outPrimGroup)
{
    bool found = false;
    Ptr<IO::Stream> stream = IO::CreateStream(asset.WorkURI("assets"));
    stream->SetAccessMode(IO::Stream::ReadAccess);
    if (stream->Open())
    {
        // assets are written without a file identifier, so the only thing to check before reading is that
        // the offset to the root table the buffer starts with lies within the file
        IO::Stream::Size const size = stream->GetSize();
        if (size >= (IO::Stream::Size)sizeof(flatbuffers::uoffset_t))
        {
            const void* data = stream->MemoryMap();
            if (flatbuffers::ReadScalar<flatbuffers::uoffset_t>(data) < (flatbuffers::uoffset_t)size)
            {
                const ToolkitUtil::ModelAsset* modelAsset = ToolkitUtil::GetModelAsset(data);
                const ToolkitUtil::MeshResource* mesh = modelAsset->mesh();
                const ToolkitUtil::SceneResource* scene = modelAsset->scene();
                bool const hasMesh = mesh != nullptr && mesh->meshes() != nullptr && mesh->meshes()->size() > 0;
                if (hasMesh && scene != nullptr)
                {
                    // skinned models keep their shapes in the skin fragments instead
                    const ToolkitUtil::ShapeNode* shape = nullptr;
                    if (scene->shapes() != nullptr && scene->shapes()->size() > 0)
                        shape = scene->shapes()->Get(0);
                    else if (scene->skins() != nullptr && scene->skins()->size() > 0)
                        shape = scene->skins()->Get(0)->shape();

                    if (shape != nullptr && shape->mesh_resource() != nullptr && shape->mesh_resource()->size() > 0)
                    {
                        outMeshResource = shape->mesh_resource()->c_str();
                        outPrimGroup = shape->prim_group();
                        found = true;
                    }
                }
            }
            stream->MemoryUnmap();
        }
        stream->Close();
    }
    return found;
}

//------------------------------------------------------------------------------
/**
    Accepts model assets dragged from the asset browser onto the viewport. An asset that has a mesh
    becomes a new entity named after the asset, with a model, a static body and a mesh collider,
    placed where the mouse ray hits the ground plane.
*/
static void
AcceptAssetDrop(Modules::Viewport& viewport)
{
    Math::vec2 const imagePos = viewport.lastViewportImagePositionAbsolute;
    Math::vec2 const imageSize = viewport.lastViewportImageSizeAbsolute;
    ImRect const imageRect(ImVec2(imagePos.x, imagePos.y), ImVec2(imagePos.x + imageSize.x, imagePos.y + imageSize.y));
    if (!ImGui::BeginDragDropTargetCustom(imageRect, ImGui::GetID("##SceneViewportDrop")))
        return;

    // peek at the payload first so that only model assets highlight the viewport
    Util::String resource;
    const ImGuiPayload* peek = ImGui::GetDragDropPayload();
    if (peek != nullptr && peek->IsDataType("resource") && IO::Path::Parse((const char*)peek->Data).GetType() == "mdl")
    {
        const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("resource");
        if (payload != nullptr)
        {
            // the payload is cleared by EndDragDropTarget once it has been delivered
            resource = (const char*)payload->Data;
        }
    }
    ImGui::EndDragDropTarget();

    if (resource.IsEmpty())
        return;

    IO::Path const asset = IO::Path::Parse(resource);
    Util::String meshResource;
    uint primGroup = 0;
    if (!asset.IsFile() || !FindAssetMesh(asset, meshResource, primGroup))
    {
        n_warning("'%s' has no mesh, no entity is created for it\n", resource.AsCharPtr());
        return;
    }

    ImVec2 const mouse = ImGui::GetMousePos();
    Math::vec2 const mousePos = { (mouse.x - imagePos.x) / imageSize.x, (mouse.y - imagePos.y) / imageSize.y };
    Math::mat4 const camTransform = Math::inverse(viewport.camera.GetViewTransform());
    Math::mat4 const invProj = Math::inverse(viewport.camera.GetProjectionTransform());
    Math::line const ray = viewport.camera.GetProjectionMode() == Editor::Camera::ProjectionMode::ORTHOGRAPHIC
        ? RenderUtil::MouseRayUtil::ComputeWorldMouseRayOrtho(mousePos, 10000, camTransform, invProj, 0.01f)
        : RenderUtil::MouseRayUtil::ComputeWorldMouseRay(mousePos, 10000, camTransform, invProj, 0.01f);

    // a ray pointing above the horizon never hits the ground, so the entity is put in front of the camera instead
    Math::plane groundPlane(Math::point(0, 0, 0), Math::vector(0, 1, 0));
    Math::point dropPoint;
    if (!groundPlane.intersect(ray, dropPoint))
        dropPoint = ray.start() + Math::normalize(ray.vec()) * 10.0f;

    GraphicsFeature::Model model;
    model.resource = resource;

    // the collider reads the same mesh URN as the model
    PhysicsFeature::MeshCollider collider;
    collider.resource = meshResource;
    collider.primGroup = primGroup;

    PhysicsFeature::StaticBody body;

    Edit::CommandManager::BeginMacro("Create entity from asset", true);
    Editor::Entity const entity = Edit::CreateEntity();
    Edit::SetEntityName(entity, asset.GetFile());
    Edit::SetWorldPosition(entity, Game::Position(Math::xyz(dropPoint)));
    Edit::AddComponent(entity, Game::GetComponentId<GraphicsFeature::Model>(), &model);
    Edit::AddComponent(entity, Game::GetComponentId<PhysicsFeature::StaticBody>(), &body);
    Edit::AddComponent(entity, Game::GetComponentId<PhysicsFeature::MeshCollider>(), &collider);
    Edit::SetSelection({ entity });
    Edit::CommandManager::EndMacro();
}

//------------------------------------------------------------------------------
/**
*/
void
Scene::Run(SaveMode save)
{
    Ptr<Input::Keyboard> keyboard = Input::InputServer::Instance()->GetDefaultKeyboard();
    Ptr<Input::Mouse> mouse = Input::InputServer::Instance()->GetDefaultMouse();

    this->viewPort.Render();

    AcceptAssetDrop(this->viewPort);

    if (this->currentTool != nullptr)
    {
        this->currentTool->Render(
            &this->viewPort
        );
    }

    DrawOutlines();

    if (this->viewPort.IsFocused())
    {
        if (this->currentTool != nullptr)
        {
            this->currentTool->Update(
                &this->viewPort
            );

            if (!this->currentTool->IsModifying() && keyboard->KeyDown(Input::Key::Escape))
            {
                this->currentTool = this->allTools[TOOL_SELECTION];
            }
        }
    }

    if (!mouse->ButtonPressed(Input::MouseButton::RightButton))
    {
        if (keyboard->KeyDown(Input::Key::W))
        {
            this->currentTool = this->allTools[TOOL_TRANSLATE];
        }
        else if (keyboard->KeyDown(Input::Key::E))
        {
            this->currentTool = this->allTools[TOOL_ROTATE];
        }
        else if (keyboard->KeyDown(Input::Key::R))
        {
            this->currentTool = this->allTools[TOOL_SCALE];
        }
    }

    auto const& selection = Tools::SelectionContext::Selection();
    for (auto const editorEntity : selection)
    {
        Game::Entity const gameEntity = Editor::state.editables[editorEntity.index].gameEntity;
        Game::World* defaultWorld = Game::GetWorld(gameEntity.world);
        if (defaultWorld->HasComponent<GraphicsFeature::Terrain>(gameEntity))
        {
            this->currentTool = this->allTools[TOOL_TERRAIN];
        }
    }

    // placing navigation area points is started from the inspector, the tool ends itself
    if (Tools::NavAreaPointTool::Target() != Editor::Entity::Invalid())
    {
        this->currentTool = this->allTools[TOOL_NAVAREA_POINTS];
    }
    // so is testing paths, from the navigation window
    else if (Tools::NavPathTool::Active())
    {
        this->currentTool = this->allTools[TOOL_NAV_PATH];
    }
    else if (this->currentTool == this->allTools[TOOL_NAVAREA_POINTS] || this->currentTool == this->allTools[TOOL_NAV_PATH])
    {
        this->currentTool = this->allTools[TOOL_SELECTION];
    }
}

//------------------------------------------------------------------------------
/**
*/
void
Scene::FocusCamera()
{
    auto selection = Tools::SelectionContext::Selection();

    if (selection.IsEmpty())
        return;

    //TODO: Use bboxes to more accurately calculate the distance offset to the object after moving the camera

    Math::vec3 centerPoint = Math::vec3(0);
    for (auto const& entity : selection)
    {
        Game::Position pos = Editor::state.editorWorld->GetComponent<Game::Position>(entity);
        centerPoint += pos;
    }

    centerPoint = centerPoint * (1.0f / (float)selection.Size());

    this->viewPort.camera.SetTargetPosition(centerPoint + Math::xyz(Math::inverse(this->viewPort.camera.GetViewTransform()).get_z() * 5.0f));
}

//------------------------------------------------------------------------------
/**
*/
static void
DrawEntityOutline(Game::Entity const gameEntity, Math::vec4 const& color)
{
    Game::World* defaultWorld = Game::GetWorld(gameEntity.world);

    if (defaultWorld->HasComponent<GraphicsFeature::Model>(gameEntity))
    {
        // TODO: Outline maybe?
        Graphics::GraphicsEntityId const gfxEntity =
            defaultWorld->GetComponent<GraphicsFeature::Model>(gameEntity).graphicsEntityId;
        if (Models::ModelContext::IsEntityRegistered(gfxEntity))
        {
            Math::bbox const bbox = Models::ModelContext::ComputeBoundingBox(gfxEntity);
            Math::mat4 const transform = Models::ModelContext::GetTransform(gfxEntity);
            Im3d::Im3dContext::DrawOrientedBox(Math::mat4::identity, bbox, color);
        }
    }
    else if (defaultWorld->HasComponent<GraphicsFeature::Terrain>(gameEntity))
    {
    }
    else
    {
        Math::vec3 location = defaultWorld->GetComponent<Game::Position>(gameEntity);
        Math::vec3 scale = defaultWorld->GetComponent<Game::Scale>(gameEntity);
        const Math::bbox box = Math::bbox(location, scale);
        Im3d::Im3dContext::DrawOrientedBox(Math::mat4::identity, box, color);
    }

    if (defaultWorld->HasComponent<GraphicsFeature::PointLight>(gameEntity))
    {
        Math::vec3 location = defaultWorld->GetComponent<Game::Position>(gameEntity);
        const GraphicsFeature::PointLight& light = defaultWorld->GetComponent<GraphicsFeature::PointLight>(gameEntity);
        Math::mat4 sphere = Math::mat4::identity;
        sphere.scale(Math::vec3(light.range, light.range, light.range));
        sphere.translate(location);
        Im3d::Im3dContext::DrawSphere(sphere, color);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
Scene::DrawOutlines()
{
    Math::vec4 const selectionColor = {1.0f, 0.30f, 0.0f, 1.0f};
    Math::vec4 const hoverColor = {0.5f, 0.15f, 0.0f, 1.0f};

    Editor::Entity const hovered = Tools::SelectionContext::Hovered();

    auto const& selection = Tools::SelectionContext::Selection();
    for (auto const editorEntity : selection)
    {
        // draw the hovered entity once below, in the hover color
        if (editorEntity == hovered)
        {
            continue;
        }
        Game::Entity const gameEntity = Editor::state.editables[editorEntity.index].gameEntity;
        DrawEntityOutline(gameEntity, selectionColor);
    }

    if (hovered != Editor::Entity::Invalid())
    {
        Game::Entity const gameEntity = Editor::state.editables[hovered.index].gameEntity;
        DrawEntityOutline(gameEntity, hoverColor);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
Scene::SetTool(ToolType type)
{
    this->currentTool = this->allTools[type];
}

} // namespace Presentation
