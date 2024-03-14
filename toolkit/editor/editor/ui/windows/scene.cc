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
Scene::Run()
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

} // namespace Presentation
