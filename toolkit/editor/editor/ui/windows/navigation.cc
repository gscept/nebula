//------------------------------------------------------------------------------
//  navigation.cc
//  (C) 2025 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "navigation.h"
#include "editor/editor.h"
#include "editor/commandmanager.h"
#include "editor/ui/uimanager.h"
#include "editor/tools/selectiontool.h"
#include "editor/cmds.h"
#include "physics/debugui.h"
#include "editor/ui/windowserver.h"
#include "editor/ui/windows/scene.h"
#include "navigationfeature/navigationfeatureunit.h"

using namespace Editor;

namespace Presentation
{
__ImplementClass(Presentation::Navigation, 'PtNa', Presentation::BaseWindow);

//------------------------------------------------------------------------------
/**
*/
Navigation::Navigation()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
Navigation::~Navigation()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
Navigation::Run(SaveMode save)
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
    NavigationFeature::RenderUI(this->defaultCamera);
}

} // namespace Presentation
