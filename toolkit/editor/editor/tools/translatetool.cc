//------------------------------------------------------------------------------
//  translatetool.cc
//  (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "util/array.h"
#include "translatetool.h"
#include "dynui/im3d/im3d.h"
#include "editor/commandmanager.h"
#include "editor/cmds.h"
#include "models/modelcontext.h"
#include "graphicsfeature/managers/graphicsmanager.h"
#include "dynui/im3d/im3dcontext.h"
#include "basegamefeature/components/basegamefeature.h"
#include "basegamefeature/components/position.h"
#include "basegamefeature/components/orientation.h"
#include "basegamefeature/components/scale.h"
#include "graphicsfeature/graphicsfeatureunit.h"
#include "editor/components/editorcomponents.h"
#include "input/inputserver.h"
#include "input/mouse.h"
#include "input/keyboard.h"
#include "util/bvh.h"
#include "renderutil/mouserayutil.h"
#include "camera.h"
#include "selectioncontext.h"

static Math::vec3 gizmoTranslation;

namespace Tools
{

//------------------------------------------------------------------------------
/**
*/
void
TranslateTool::Update(Presentation::Modules::Viewport* viewport)
{
    Ptr<Input::Mouse> mouse = Input::InputServer::Instance()->GetDefaultMouse();
    Ptr<Input::Keyboard> keyboard = Input::InputServer::Instance()->GetDefaultKeyboard();

    // Abort action if we press escape
    if (keyboard->KeyDown(Input::Key::Escape) && IsModifying())
    {
        Abort();
    }

    if (mouse->ButtonUp(Input::MouseButton::Code::LeftButton))
    {
        Tools::SelectionContext::PerformPicking(viewport->lastViewportImagePosition, viewport->lastViewportImageSize, &viewport->camera);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
TranslateTool::Render(Presentation::Modules::Viewport* viewport)
{
    SelectionContext::ValidateSelection();

    if (SelectionContext::Selection().IsEmpty())
        return;

    auto const& selection = SelectionContext::Selection();

    Game::World* defaultWorld = Game::GetWorld(WORLD_DEFAULT);
    Im3d::Im3dContext::SetViewportRect(viewport->lastViewportImagePosition, viewport->lastViewportImageSize);
   
    if (!this->translation.isDirty)
    {
        Editor::Entity const editorEntity = selection.Back();
        auto pos = Editor::state.editorWorld->GetComponent<Game::Position>(editorEntity);
        
        gizmoTranslation = pos;
    }
    this->translation.gizmoTransforming = Im3d::GizmoTranslation("GizmoEntity", gizmoTranslation.v);
    
    if (this->translation.gizmoTransforming)
    {
        Game::Entity const gameEntity = Editor::state.editables[selection.Back().index].gameEntity;
        if (!this->translation.isDirty)
        {
            SelectionContext::Pause();
        }
        this->translation.isDirty = true;

        //if (this->translation.useGridIncrements)
        //{
        //    // TODO:
        //    //this->translation.delta.x = Math::round(this->translation.delta.x / state.grid.size) * state.grid.size;
        //    //this->translation.delta.y = Math::round(this->translation.delta.y / state.grid.size) * state.grid.size;
        //    //this->translation.delta.z = Math::round(this->translation.delta.z / state.grid.size) * state.grid.size;
        //}
        defaultWorld->SetComponent<Game::Position>(gameEntity, gizmoTranslation);
        defaultWorld->MarkAsModified(gameEntity);
    }
    else if (this->translation.isDirty)
    {
        // User has release gizmo, we can set real transform and add to undo queue
        Edit::CommandManager::BeginMacro("Translate", false);
        Edit::SetComponent(selection.Back(), Game::GetComponentId<Game::Position>(), &gizmoTranslation);
        Edit::CommandManager::EndMacro();
        this->translation.gizmoTransforming = false;
        this->translation.isDirty = false;
        SelectionContext::Unpause();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
TranslateTool::Abort()
{
    // abort transformations
    this->translation.gizmoTransforming = false;
    this->translation.isDirty = false;
}

//------------------------------------------------------------------------------
/**
*/
bool
TranslateTool::IsModifying() const
{
    return this->translation.gizmoTransforming;
}

} // namespace Tools
