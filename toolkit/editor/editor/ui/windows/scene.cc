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
#include "editor/tools/selectiontool.h"
#include "editor/ui/windowserver.h"

#include "basegamefeature/components/position.h"

using namespace Editor;

namespace Presentation
{
__ImplementClass(Presentation::Scene, 'SCWn', Presentation::BaseWindow);

//------------------------------------------------------------------------------
/**
*/
Scene::Scene()
{
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
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
Scene::Update()
{
    viewPort.Update();
}

//------------------------------------------------------------------------------
/**
*/
void
Scene::Run(SaveMode save)
{
    viewPort.Render();
    Tools::SelectionTool::RenderGizmo(
        this->viewPort.lastViewportImagePosition, this->viewPort.lastViewportImageSize, &this->viewPort.camera
    );
    if (viewPort.IsHovered())
    {
        Tools::SelectionTool::Update(
            this->viewPort.lastViewportImagePosition, this->viewPort.lastViewportImageSize, &this->viewPort.camera
        );
    }
}

//------------------------------------------------------------------------------
/**
*/
void
Scene::FocusCamera()
{
    auto selection = Tools::SelectionTool::Selection();

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

} // namespace Presentation
