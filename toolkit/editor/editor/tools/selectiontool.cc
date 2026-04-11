//------------------------------------------------------------------------------
//  selectiontool.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "util/array.h"
#include "selectiontool.h"
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

namespace Tools
{

//------------------------------------------------------------------------------
/**
*/
void
SelectionTool::Update(Presentation::Modules::Viewport* viewport)
{
    Ptr<Input::Mouse> mouse = Input::InputServer::Instance()->GetDefaultMouse();
    Ptr<Input::Keyboard> keyboard = Input::InputServer::Instance()->GetDefaultKeyboard();

    // Deselect if we press escape
    if (keyboard->KeyDown(Input::Key::Escape) && !SelectionContext::Selection().IsEmpty())
    {
        Edit::SetSelection({});
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
SelectionTool::Render(Presentation::Modules::Viewport* viewport)
{
    SelectionContext::ValidateSelection();

    if (SelectionContext::Selection().IsEmpty())
        return;

    auto const& selection = SelectionContext::Selection();

    Game::World* defaultWorld = Game::GetWorld(WORLD_DEFAULT);
    Ptr<Input::Mouse> mouse = Input::InputServer::Instance()->GetDefaultMouse();
    Im3d::Im3dContext::SetViewportRect(viewport->lastViewportImagePosition, viewport->lastViewportImageSize);

    Ptr<Input::Keyboard> keyboard = Input::InputServer::Instance()->GetDefaultKeyboard();

    // Perform translation if necessary

    if (mouse->ButtonDown(Input::MouseButton::Code::LeftButton))
    {
        this->translation.mousePosOnStart = mouse->GetPixelPosition();
    }

    if (!this->translation.dragTimer.Running() && SelectionContext::IsPaused())
    {
        SelectionContext::Unpause();
    }

    // Only start translating if the mouse has moved more that a small distance
    if (!this->translation.dragTimer.Running())
    {
        float mouseDistance = (mouse->GetPixelPosition() - this->translation.mousePosOnStart).length();
        if (mouseDistance > 0.0015f && mouse->ButtonPressed(Input::MouseButton::Code::LeftButton))
        {
            this->translation.dragTimer.Reset();
            this->translation.dragTimer.Start();
            SelectionContext::Pause();
        }
    }

    Math::vec2 mousePos = mouse->GetPixelPosition();
    mousePos -= viewport->lastViewportImagePosition;
    mousePos = { mousePos.x / viewport->lastViewportImageSize.x, mousePos.y / viewport->lastViewportImageSize.y };

    Math::mat4 const camTransform = Math::inverse(viewport->camera.GetViewTransform());
    Math::mat4 const invProj = Math::inverse(viewport->camera.GetProjectionTransform());

    Math::line ray = RenderUtil::MouseRayUtil::ComputeWorldMouseRay(mousePos, 10000, camTransform, invProj, 0.01f);

    if (this->translation.dragTimer.Running() && !this->translation.isDirty)
    {
        this->translation.originEntity = SelectionContext::GetSelectedEntityUnderMouse(viewport->lastViewportImagePosition, viewport->lastViewportImageSize, &viewport->camera);
        if (this->translation.originEntity == Editor::Entity::Invalid())
        {
            // failed to find entity. Cancel translation attempt
            this->translation.dragTimer.Stop();
        }
        else
        {
            Game::Position entityPos = Editor::state.editorWorld->GetComponent<Game::Position>(this->translation.originEntity);
            this->translation.plane = Math::plane(entityPos, Math::vector(0, 1, 0));
            Math::point startPos;
            if (this->translation.plane.intersect(ray, startPos))
            {
                this->translation.startPos = startPos.vec;
                this->translation.isDirty = true;
            }
            else
            {
                // failed to find plane. Cancel translation attempt
                this->translation.dragTimer.Stop();
            }
        }
    }

    bool const applyTransform = mouse->ButtonUp(Input::MouseButton::Code::LeftButton) && this->translation.dragTimer.Running();
    if (applyTransform)
    {
        this->translation.dragTimer.Stop();
    }

    if (this->translation.dragTimer.Running())
    {
        bool xzAxis = !keyboard->KeyPressed(Input::Key::LeftMenu);

        if (xzAxis)
        {
            Math::point mousePosOnWorldPlane;
            if (this->translation.plane.intersect(ray, mousePosOnWorldPlane))
            {
                this->translation.delta = mousePosOnWorldPlane.vec - this->translation.startPos;
                if (this->translation.useGridIncrements)
                {
                    this->translation.delta.x = Math::round(this->translation.delta.x / this->grid.size) * this->grid.size;
                    this->translation.delta.y = Math::round(this->translation.delta.y / this->grid.size) * this->grid.size;
                    this->translation.delta.z = Math::round(this->translation.delta.z / this->grid.size) * this->grid.size;
                }
            }
        }
        else // y axis translation
        {
            // find a good plane
            Game::Position gameEntityPos = defaultWorld->GetComponent<Game::Position>(
                Editor::state.editables[this->translation.originEntity.index].gameEntity
            );

            Math::vector planeNormal = Math::cross(Math::normalize(ray.m), Math::vector(0, 1, 0));
            planeNormal = Math::cross(planeNormal, Math::vector(0, 1, 0));

            Math::plane plane = Math::plane(gameEntityPos, planeNormal);
            Math::point mousePosOnWorldPlane;
            if (plane.intersect(ray, mousePosOnWorldPlane))
            {
                this->translation.delta.y = (mousePosOnWorldPlane.vec - this->translation.startPos).y;
                if (this->translation.useGridIncrements)
                {
                    this->translation.delta.y = Math::round(this->translation.delta.y / this->grid.size) * this->grid.size;
                }
                this->translation.plane =
                    Math::plane(this->translation.startPos + this->translation.delta, Math::vector(0, 1, 0));
            }
        }

        for (IndexT i = 0; i < selection.Size(); i++)
        {
            Game::Position pos = Editor::state.editorWorld->GetComponent<Game::Position>(selection[i]);
            pos += this->translation.delta;

            Game::Entity const gameEntity = Editor::state.editables[selection[i].index].gameEntity;
            defaultWorld->SetComponent<Game::Position>(gameEntity, pos);
            defaultWorld->MarkAsModified(gameEntity);
        }
    }
    else if (this->translation.isDirty)
    {
        // Only apply the translation if the translation has happened for more than some threshold duration
        constexpr float minDragTime = 0.1f;
        if (this->translation.delta != Math::vec3(0) && this->translation.dragTimer.GetTime() > minDragTime)
        {
            // User has release gizmo, we can set real transform and add to undo queue
            Edit::CommandManager::BeginMacro("Translate entities", false);
            for (IndexT i = 0; i < selection.Size(); i++)
            {
                Game::Position pos = Editor::state.editorWorld->GetComponent<Game::Position>(selection[i]);
                pos += this->translation.delta;
                Edit::SetComponent(selection[i], Game::GetComponentId<Game::Position>(), &pos);
            }
            Edit::CommandManager::EndMacro();
            this->translation.isDirty = false;
            this->translation.delta = Math::vec3(0);
        }
        else
        {
            // Reset game entity position to be same as editors
            for (IndexT i = 0; i < selection.Size(); i++)
            {
                Game::Position pos = Editor::state.editorWorld->GetComponent<Game::Position>(selection[i]);
                Game::Entity const gameEntity = Editor::state.editables[selection[i].index].gameEntity;
                defaultWorld->SetComponent<Game::Position>(gameEntity, pos);
                defaultWorld->MarkAsModified(gameEntity);
            }
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
bool
SelectionTool::IsModifying() const
{
    return this->translation.dragTimer.Running();
}

//------------------------------------------------------------------------------
/**
*/
void
SelectionTool::SnapToGridIncrements(bool value)
{
    this->translation.useGridIncrements = value;
}

//------------------------------------------------------------------------------
/**
*/
bool
SelectionTool::SnapToGridIncrements()
{
    return this->translation.useGridIncrements;
}

} // namespace Tools
