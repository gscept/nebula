//------------------------------------------------------------------------------
//  physics.cc
//  (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "physics.h"
#include "editor/editor.h"
#include "editor/commandmanager.h"
#include "editor/ui/uimanager.h"
#include "editor/tools/selectiontool.h"
#include "editor/cmds.h"
#include "physics/debugui.h"
#include "editor/ui/windowserver.h"
#include "editor/ui/windows/scene.h"
#include "graphics/cameracontext.h"

using namespace Editor;

namespace Presentation
{
__ImplementClass(Presentation::Physics, 'PtPh', Presentation::BaseWindow);

//------------------------------------------------------------------------------
/**
*/
Physics::Physics()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
Physics::~Physics()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
Physics::Run(SaveMode save)
{
    if (this->defaultCamera == Graphics::GraphicsEntityId::Invalid())
    {
        Ptr<Scene> sceneWindow = Presentation::WindowServer::Instance()->GetWindow("Scene View").downcast<Presentation::Scene>();
        if (sceneWindow.isvalid())
        {
            this->defaultCamera = sceneWindow->viewPort.GetView()->GetCamera();
        }
    }
    if (this->defaultCamera == Graphics::GraphicsEntityId::Invalid())
    {
        return;
    }
    ::Physics::RenderUI(Graphics::CameraContext::GetView(this->defaultCamera));
}

} // namespace Presentation
