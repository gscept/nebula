//------------------------------------------------------------------------------
//  rotatetool.cc
//  (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "util/array.h"
#include "rotatetool.h"
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

static Im3d::Mat3 gizmoRotation;

namespace Tools
{

//------------------------------------------------------------------------------
/**
*/
void
RotateTool::Update(Presentation::Modules::Viewport* viewport)
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
RotateTool::Render(Presentation::Modules::Viewport* viewport)
{
    SelectionContext::ValidateSelection();

    if (SelectionContext::Selection().IsEmpty())
        return;

    auto const& selection = SelectionContext::Selection();

    Game::World* defaultWorld = Game::GetWorld(WORLD_DEFAULT);
    Ptr<Input::Mouse> mouse = Input::InputServer::Instance()->GetDefaultMouse();
    Im3d::Im3dContext::SetViewportRect(viewport->lastViewportImagePosition, viewport->lastViewportImageSize);

    if (!this->translation.isDirty)
    {
        Game::Entity const editorEntity = selection.Back();
        auto orientation = Editor::state.editorWorld->GetComponent<Game::Orientation>(editorEntity);

        Math::mat4 m = Math::rotationquat(orientation);
        gizmoRotation = {
            Im3d::Vec3(m.row0.x, m.row0.y, m.row0.z),
            Im3d::Vec3(m.row1.x, m.row1.y, m.row1.z),
            Im3d::Vec3(m.row2.x, m.row2.y, m.row2.z),
        };
    }

    auto pos = Editor::state.editorWorld->GetComponent<Game::Position>(selection.Back());
    auto orientation = Editor::state.editorWorld->GetComponent<Game::Orientation>(selection.Back());
    auto scale = Editor::state.editorWorld->GetComponent<Game::Scale>(selection.Back());
    Math::mat4 t = Math::trs(pos, orientation, scale);
    Im3d::PushMatrix(t);
    this->translation.gizmoTransforming = Im3d::GizmoRotation("GizmoEntity", gizmoRotation, true);
    Im3d::PopMatrix();

    Math::vec3 r0 = gizmoRotation.getRow(0);
    Math::vec3 r1 = gizmoRotation.getRow(1);
    Math::vec3 r2 = gizmoRotation.getRow(2);
    Math::mat4 r = Math::transpose(Math::mat4(
        Math::vec4(r0, 0),
        Math::vec4(r1, 0),
        Math::vec4(r2, 0),
        Math::vec4(0, 0, 0, 1)
    ));
    Math::quat q = Math::rotationmatrix(r);

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
        
        defaultWorld->SetComponent<Game::Orientation>(gameEntity, q);
        defaultWorld->MarkAsModified(gameEntity);
    }
    else if (this->translation.isDirty)
    {
        // User has release gizmo, we can set real transform and add to undo queue
        Edit::CommandManager::BeginMacro("Rotate", false);
        Edit::SetComponent(selection.Back(), Game::GetComponentId<Game::Orientation>(), &q);
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
RotateTool::Abort()
{
    // abort transformations
    this->translation.gizmoTransforming = false;
    this->translation.isDirty = false;
}


//------------------------------------------------------------------------------
/**
*/
bool
RotateTool::IsModifying() const
{
    return this->translation.gizmoTransforming;
}

} // namespace Tools
