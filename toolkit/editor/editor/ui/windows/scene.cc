//------------------------------------------------------------------------------
//  scene.cc
//  (C) 2021 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "scene.h"
#include "editor/editor.h"
#include "editor/commandmanager.h"
#include "editor/ui/uimanager.h"
#include "graphicsfeature/graphicsfeatureunit.h"
#include "editor/ui/windowserver.h"
#include "editor/tools/selectioncontext.h"

#include "basegamefeature/components/position.h"
#include "graphicsfeature/components/model.h"
#include "graphicsfeature/components/lighting.h"
#include "models/modelcontext.h"
#include "dynui/im3d/im3dcontext.h"

#include "editor/tools/selectiontool.h"
#include "editor/tools/translatetool.h"
#include "editor/tools/rotatetool.h"
#include "editor/tools/scaletool.h"

#include "input/keyboard.h"
#include "input/mouse.h"
#include "input/inputserver.h"

using namespace Editor;

namespace Presentation
{
__ImplementClass(Presentation::Scene, 'SCWn', Presentation::BaseWindow);

//------------------------------------------------------------------------------
/**
*/
Scene::Scene()
{
    allTools[0] = new Tools::SelectionTool();
    allTools[1] = new Tools::TranslateTool();
    allTools[2] = new Tools::RotateTool();
    allTools[3] = new Tools::ScaleTool();

    currentTool = allTools[0];

    viewPort.Init(GraphicsFeature::GraphicsFeatureUnit::Instance()->GetDefaultView());
    viewPort.SetStage(GraphicsFeature::GraphicsFeatureUnit::Instance()->GetDefaultStage());
    viewPort.SetFrameBuffer("ColorBufferNoGUI");

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
    delete allTools[0];
    delete allTools[1];
    delete allTools[2];
    delete allTools[3];
    currentTool = nullptr;
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
*/
void
Scene::Run(SaveMode save)
{
    Ptr<Input::Keyboard> keyboard = Input::InputServer::Instance()->GetDefaultKeyboard();
    Ptr<Input::Mouse> mouse = Input::InputServer::Instance()->GetDefaultMouse();

    this->viewPort.Render();

    if (this->currentTool != nullptr)
    {
        this->currentTool->Render(
            this->viewPort.lastViewportImagePosition, this->viewPort.lastViewportImageSize, &this->viewPort.camera
        );
    }

    DrawOutlines();

    if (viewPort.IsFocused())
    {
        if (this->currentTool != nullptr)
        {
            this->currentTool->Update(
                this->viewPort.lastViewportImagePosition, this->viewPort.lastViewportImageSize, &this->viewPort.camera
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

    viewPort.camera.SetTargetPosition(centerPoint + Math::xyz(Math::inverse(viewPort.camera.GetViewTransform()).get_z() * 5.0f));
}

//------------------------------------------------------------------------------
/**
*/
void
Scene::DrawOutlines()
{
    auto const& selection = Tools::SelectionContext::Selection();
    for (auto const editorEntity : selection)
    {
        Game::Entity const gameEntity = Editor::state.editables[editorEntity.index].gameEntity;
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
                Im3d::Im3dContext::DrawOrientedBox(Math::mat4::identity, bbox, {1.0f, 0.30f, 0.0f, 1.0f});
            }
        }
        else
        {
            Math::vec3 location = defaultWorld->GetComponent<Game::Position>(gameEntity);
            const Math::bbox box = Math::bbox(location, Math::vector(0.1f, 0.1f, 0.1f));
            Im3d::Im3dContext::DrawOrientedBox(Math::mat4::identity, box, {1.0f, 0.30f, 0.0f, 1.0f});
        }

        if (defaultWorld->HasComponent<GraphicsFeature::PointLight>(gameEntity))
        {
            Math::vec3 location = defaultWorld->GetComponent<Game::Position>(gameEntity);
            const GraphicsFeature::PointLight& light = defaultWorld->GetComponent<GraphicsFeature::PointLight>(gameEntity);
            Math::mat4 sphere = Math::mat4::identity;
            sphere.scale(Math::vec3(light.range, light.range, light.range));
            sphere.translate(location);
            Im3d::Im3dContext::DrawSphere(sphere, {1.0f, 0.30f, 0.0f, 1.0f});
        }
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
