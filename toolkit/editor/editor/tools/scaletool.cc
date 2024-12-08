//------------------------------------------------------------------------------
//  scaletool.cc
//  (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "util/array.h"
#include "scaletool.h"
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

static Math::vec3 gizmoScale;

namespace Tools
{

//------------------------------------------------------------------------------
/**
*/
void
ScaleTool::Update(Math::vec2 const& viewPortPosition, Math::vec2 const& viewPortSize, Editor::Camera const* camera)
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
        Tools::SelectionContext::PerformPicking(viewPortPosition, viewPortSize, camera);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
ScaleTool::Render(Math::vec2 const& viewPortPosition, Math::vec2 const& viewPortSize, Editor::Camera const* camera)
{
    SelectionContext::ValidateSelection();

    if (SelectionContext::Selection().IsEmpty())
        return;

    auto const& selection = SelectionContext::Selection();

    Game::World* defaultWorld = Game::GetWorld(WORLD_DEFAULT);
    Ptr<Input::Mouse> mouse = Input::InputServer::Instance()->GetDefaultMouse();
    Im3d::Im3dContext::SetViewportRect(viewPortPosition, viewPortSize);

    if (!this->translation.isDirty)
    {
        Game::Entity const gameEntity = selection.Back();
        auto scale = Editor::state.editorWorld->GetComponent<Game::Scale>(gameEntity);

        gizmoScale = scale;
    }

    auto pos = Editor::state.editorWorld->GetComponent<Game::Position>(selection.Back());
    auto orientation = Editor::state.editorWorld->GetComponent<Game::Orientation>(selection.Back());
    auto scale = Editor::state.editorWorld->GetComponent<Game::Scale>(selection.Back());
    Math::mat4 t = Math::trs(pos, orientation, scale);
    Im3d::PushMatrix(t);
    this->translation.gizmoTransforming = Im3d::GizmoScale("GizmoEntity", gizmoScale.v);
    Im3d::PopMatrix();

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
        defaultWorld->SetComponent<Game::Scale>(gameEntity, gizmoScale);
        defaultWorld->MarkAsModified(gameEntity);
    }
    else if (this->translation.isDirty)
    {
        // User has release gizmo, we can set real transform and add to undo queue
        Edit::CommandManager::BeginMacro("Scale", false);
        Edit::SetComponent(selection.Back(), Game::GetComponentId<Game::Scale>(), &gizmoScale);
        Edit::CommandManager::EndMacro();
        this->translation.gizmoTransforming = false;
        this->translation.isDirty = false;
        SelectionContext::Unpause();
    }
}

//------------------------------------------------------------------------------
/**
*/
bool
ScaleTool::IsModifying() const
{
    return this->translation.gizmoTransforming;
}

//------------------------------------------------------------------------------
/**
*/
void
ScaleTool::Abort()
{
    // abort transformations
    this->translation.gizmoTransforming = false;
    this->translation.isDirty = false;
}

} // namespace Tools
