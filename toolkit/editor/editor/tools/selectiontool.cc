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

Tools::SelectionTool::State Tools::SelectionTool::state = {};
static Im3d::Mat4 gizmoTransform;
namespace Tools
{

enum class SelectionMode
{
    Replace = 0,
    Append = 1,
};

//------------------------------------------------------------------------------
/**
*/
Util::Array<Editor::Entity> const&
SelectionTool::Selection()
{
    return SelectionTool::state.selection;
}



//------------------------------------------------------------------------------
/**
*/
void
SelectionTool::Update(Math::vec2 const& viewPortPosition, Math::vec2 const& viewPortSize, Editor::Camera const* camera)
{
    Ptr<Input::Mouse> mouse = Input::InputServer::Instance()->GetDefaultMouse();
    Ptr<Input::Keyboard> keyboard = Input::InputServer::Instance()->GetDefaultKeyboard();

    bool performPicking = mouse->ButtonUp(Input::MouseButton::Code::LeftButton) && state.picking.pauseCounter == 0;
    SelectionMode mode = (keyboard->KeyPressed(Input::Key::LeftControl) || keyboard->KeyPressed(Input::Key::RightControl))
                             ? SelectionMode::Append
                             : SelectionMode::Replace;

    if (performPicking)
    {
        Math::vec2 mousePos = mouse->GetScreenPosition();
        //TODO: move mousepos to viewport space
        mousePos -= viewPortPosition;
        mousePos = {mousePos.x / viewPortSize.x, mousePos.y / viewPortSize.y};

        Math::mat4 const camTransform = Math::inverse(camera->GetViewTransform());
        Math::mat4 const invProj = Math::inverse(camera->GetProjectionTransform());

        Math::line ray = RenderUtil::MouseRayUtil::ComputeWorldMouseRay(mousePos, 10000, camTransform, invProj, 0.01f);

        Util::Bvh bvh;

        Util::Array<Editor::EditorEntity> editorEntities;
        Util::Array<Math::bbox> bboxes;

        Game::Filter filter =
            Game::FilterBuilder().Including<const Game::Entity, const GraphicsFeature::Model, const Editor::EditorEntity>().Build(
            );

        Game::Dataset dataset = Game::GetWorld(WORLD_DEFAULT)->Query(filter);

        for (IndexT v = 0; v < dataset.numViews; v++)
        {
            Game::Dataset::View const* view = dataset.views + v;
            for (IndexT i = 0; i < view->numInstances; i++)
            {
                Game::Entity const& gameEntity = *((Game::Entity*)view->buffers[0] + i);
                GraphicsFeature::Model const& model = *((GraphicsFeature::Model*)view->buffers[1] + i);
                Editor::EditorEntity const& editorEntity = *((Editor::EditorEntity*)view->buffers[2] + i);

                if (Models::ModelContext::IsEntityRegistered(model.graphicsEntityId))
                {
                    Math::bbox const bbox = Models::ModelContext::ComputeBoundingBox(model.graphicsEntityId);
                    editorEntities.Append(editorEntity);
                    bboxes.Append(bbox);
                }
            }
        }

        if (bboxes.Size() > 0)
        {
            // TODO: pretty unnecessary to build a bvh for just a single raycast. Maybe cache it and do incremental updates when moving and adding things.
            bvh.Build(bboxes.Begin(), bboxes.Size());
            Util::Array<uint32_t> intersectionIndices = bvh.Intersect(ray);

            Util::Array<Editor::Entity> newSelection;
            
            if (mode == SelectionMode::Append)
                newSelection = state.selection;

            float closestDist = 1e30f;
            Editor::Entity closestEntity = Editor::Entity::Invalid();

            for (IndexT i = 0; i < intersectionIndices.Size(); i++)
            {
                uint32_t const idx = intersectionIndices[i];
                Editor::EditorEntity const& editorEntity = editorEntities[idx];
                Math::bbox const& bbox = bboxes[idx];

                float t;
                if (bbox.intersects(ray, t))
                {
                    if (t < closestDist)
                    {
                        closestEntity = editorEntity.id;
                        closestDist = t;
                    }
                }
            }
            
            if (closestEntity != Editor::Entity::Invalid())
            {
                IndexT existsAtIndex = newSelection.FindIndex(closestEntity);
                if (existsAtIndex == InvalidIndex)
                {
                    newSelection.Append(closestEntity);
                }
                else
                {
                    newSelection.EraseIndex(existsAtIndex);
                }
            }
            
            if (!newSelection.IsEmpty() || !state.selection.IsEmpty())
            {
                Edit::SetSelection(newSelection);
            }
        }

        Game::DestroyFilter(filter);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
SelectionTool::RenderGizmo(Math::vec2 const& viewPortPosition, Math::vec2 const& viewPortSize, Editor::Camera const* camera)
{
    for (SizeT i = 0; i < state.selection.Size(); i++)
    {
        if (!Editor::state.editorWorld->IsValid(state.selection[i]) ||
            !Editor::state.editorWorld->HasInstance(state.selection[i]))
        {
            state.selection.EraseIndex(i);
            i--;
            continue;
        }
    }

    if (state.selection.IsEmpty())
        return;

    Game::World* defaultWorld = Game::GetWorld(WORLD_DEFAULT);
    Ptr<Input::Mouse> mouse = Input::InputServer::Instance()->GetDefaultMouse();
    Im3d::Im3dContext::SetViewportRect(viewPortPosition, viewPortSize);

    if (state.translation.useGizmo)
    {   
        if (!state.translation.isDirty)
        {
            Game::Entity const gameEntity = state.selection.Back();
            auto pos = Editor::state.editorWorld->GetComponent<Game::Position>(gameEntity);
            auto orientation = Editor::state.editorWorld->GetComponent<Game::Orientation>(gameEntity);
            auto scale = Editor::state.editorWorld->GetComponent<Game::Scale>(gameEntity);

            gizmoTransform = Math::trs(pos, orientation, scale);
        }
        state.translation.gizmoTransforming = Im3d::Gizmo("GizmoEntity", gizmoTransform);
        Math::vec3 pos;
        Math::quat rot;
        Math::vec3 scale;
        Math::decompose(gizmoTransform, scale, rot, pos);
        if (state.translation.gizmoTransforming)
        {
            Game::Entity const gameEntity = Editor::state.editables[state.selection.Back().index].gameEntity;
            if (!state.translation.isDirty)
            {
                state.picking.pauseCounter++;
                state.translation.pickingPaused = true;
                state.translation.startPos = Editor::state.editorWorld->GetComponent<Game::Position>(state.selection.Back());
            }
            state.translation.isDirty = true;
            
            state.translation.delta = pos - state.translation.startPos;
            if (state.translation.useGridIncrements)
            {
                state.translation.delta.x = Math::round(state.translation.delta.x / state.grid.size) * state.grid.size;
                state.translation.delta.y = Math::round(state.translation.delta.y / state.grid.size) * state.grid.size;
                state.translation.delta.z = Math::round(state.translation.delta.z / state.grid.size) * state.grid.size;

            }
            defaultWorld->SetComponent<Game::Position>(gameEntity, { state.translation.startPos + state.translation.delta });
            defaultWorld->SetComponent<Game::Orientation>(gameEntity, { rot });
            defaultWorld->SetComponent<Game::Scale>(gameEntity, { scale });
            defaultWorld->MarkAsModified(gameEntity);
        }
        else if (state.translation.isDirty)
        {
            // User has release gizmo, we can set real transform and add to undo queue
            Edit::CommandManager::BeginMacro("Modify transform", false);
            Edit::SetComponent(state.selection.Back(), Game::GetComponentId<Game::Position>(), &pos);
            Edit::SetComponent(state.selection.Back(), Game::GetComponentId<Game::Orientation>(), &rot);
            Edit::SetComponent(state.selection.Back(), Game::GetComponentId<Game::Scale>(), &scale);
            Edit::CommandManager::EndMacro();
            state.translation.gizmoTransforming = false;
            state.translation.isDirty = false;
            state.translation.pickingPaused = false;
            state.picking.pauseCounter--;
        }
    }
    else
    {
        
        Ptr<Input::Keyboard> keyboard = Input::InputServer::Instance()->GetDefaultKeyboard();

        if (mouse->ButtonDown(Input::MouseButton::Code::LeftButton))
        {
            state.translation.mousePosOnStart = mouse->GetScreenPosition();
        }

        if (!state.translation.dragTimer.Running() && state.translation.pickingPaused)
        {
            state.translation.pickingPaused = false;
            state.picking.pauseCounter--;
        }

        // Only start translating if the mouse has moved more that a small distance
        if (!state.translation.dragTimer.Running())
        {
            float mouseDistance = (mouse->GetScreenPosition() - state.translation.mousePosOnStart).length();
            if (mouseDistance > 0.0015f && mouse->ButtonPressed(Input::MouseButton::Code::LeftButton))
            {
                state.translation.dragTimer.Reset();
                state.translation.dragTimer.Start();
                state.picking.pauseCounter++;
                state.translation.pickingPaused = true;
            }
        }

        Math::vec2 mousePos = mouse->GetScreenPosition();
        mousePos -= viewPortPosition;
        mousePos = { mousePos.x / viewPortSize.x, mousePos.y / viewPortSize.y };

        Math::mat4 const camTransform = Math::inverse(camera->GetViewTransform());
        Math::mat4 const invProj = Math::inverse(camera->GetProjectionTransform());

        Math::line ray = RenderUtil::MouseRayUtil::ComputeWorldMouseRay(mousePos, 10000, camTransform, invProj, 0.01f);

        if (state.translation.dragTimer.Running() && !state.translation.isDirty)
        {
            state.translation.originEntity = GetSelectedEntityUnderMouse(viewPortPosition, viewPortSize, camera);
            if (state.translation.originEntity == Editor::Entity::Invalid())
            {
                // failed to find entity. Cancel translation attempt
                state.translation.dragTimer.Stop();
            }
            else
            {
                Game::Position entityPos = Editor::state.editorWorld->GetComponent<Game::Position>(state.translation.originEntity);
                state.translation.plane = Math::plane(entityPos, Math::vector(0, 1, 0));
                Math::point startPos;
                if (state.translation.plane.intersect(ray, startPos))
                {
                    state.translation.startPos = startPos.vec;
                    state.translation.isDirty = true;
                }
                else
                {
                    // failed to find plane. Cancel translation attempt
                    state.translation.dragTimer.Stop();
                }
            }
        }

        bool const applyTransform = mouse->ButtonUp(Input::MouseButton::Code::LeftButton) && state.translation.dragTimer.Running();
        if (applyTransform)
        {
            state.translation.dragTimer.Stop();
        }

        if (state.translation.dragTimer.Running())
        {
            bool xzAxis = !keyboard->KeyPressed(Input::Key::LeftMenu);

            if (xzAxis)
            {
                Math::point mousePosOnWorldPlane;
                if (state.translation.plane.intersect(ray, mousePosOnWorldPlane))
                {
                    state.translation.delta = mousePosOnWorldPlane.vec - state.translation.startPos;
                    if (state.translation.useGridIncrements)
                    {
                        state.translation.delta.x = Math::round(state.translation.delta.x / state.grid.size) * state.grid.size;
                        state.translation.delta.y = Math::round(state.translation.delta.y / state.grid.size) * state.grid.size;
                        state.translation.delta.z = Math::round(state.translation.delta.z / state.grid.size) * state.grid.size;
                    }
                }
            }
            else // y axis translation
            {
                // find a good plane
                Game::Position gameEntityPos = defaultWorld->GetComponent<Game::Position>(Editor::state.editables[state.translation.originEntity.index].gameEntity);

                Math::vector planeNormal = Math::cross(Math::normalize(ray.m), Math::vector(0, 1, 0));
                planeNormal = Math::cross(planeNormal, Math::vector(0, 1, 0));

                Math::plane plane = Math::plane(gameEntityPos, planeNormal);
                Math::point mousePosOnWorldPlane;
                if (plane.intersect(ray, mousePosOnWorldPlane))
                {
                    state.translation.delta.y = (mousePosOnWorldPlane.vec - state.translation.startPos).y;
                    if (state.translation.useGridIncrements)
                    {
                        state.translation.delta.y = Math::round(state.translation.delta.y / state.grid.size) * state.grid.size;
                    }
                    state.translation.plane = Math::plane(state.translation.startPos + state.translation.delta, Math::vector(0, 1, 0));
                }
            }

            for (IndexT i = 0; i < state.selection.Size(); i++)
            {
                Game::Position pos = Editor::state.editorWorld->GetComponent<Game::Position>(state.selection[i]);
                pos += state.translation.delta;

                Game::Entity const gameEntity = Editor::state.editables[state.selection[i].index].gameEntity;
                defaultWorld->SetComponent<Game::Position>(gameEntity, pos);
                defaultWorld->MarkAsModified(gameEntity);
            }
        }
        else if (state.translation.isDirty)
        {
            // Only apply the translation if the translation has happened for more than some threshold duration
            constexpr float minDragTime = 0.1f;
            if (state.translation.delta != Math::vec3(0) && state.translation.dragTimer.GetTime() > minDragTime)
            {
                // User has release gizmo, we can set real transform and add to undo queue
                Edit::CommandManager::BeginMacro("Translate entities", false);
                for (IndexT i = 0; i < state.selection.Size(); i++)
                {
                    Game::Position pos = Editor::state.editorWorld->GetComponent<Game::Position>(state.selection[i]);
                    pos += state.translation.delta;
                    Edit::SetComponent(state.selection[i], Game::GetComponentId<Game::Position>(), &pos);
                }
                Edit::CommandManager::EndMacro();
                state.translation.isDirty = false;
                state.translation.delta = Math::vec3(0);
            }
            else
            {
                // Reset game entity position to be same as editors
                for (IndexT i = 0; i < state.selection.Size(); i++)
                {
                    Game::Position pos = Editor::state.editorWorld->GetComponent<Game::Position>(state.selection[i]);
                    Game::Entity const gameEntity = Editor::state.editables[state.selection[i].index].gameEntity;
                    defaultWorld->SetComponent<Game::Position>(gameEntity, pos);
                    defaultWorld->MarkAsModified(gameEntity);
                }
            }
        }
    }

    for (auto const editorEntity : state.selection)
    {
        Game::Entity const gameEntity = Editor::state.editables[editorEntity.index].gameEntity;
        if (defaultWorld->HasComponent<GraphicsFeature::Model>(gameEntity))
        {
            // TODO: Outline maybe?
            Graphics::GraphicsEntityId const gfxEntity = defaultWorld->GetComponent<GraphicsFeature::Model>(gameEntity).graphicsEntityId;
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
            Im3d::Im3dContext::DrawSphere(sphere, { 1.0f, 0.30f, 0.0f, 1.0f });
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
bool
SelectionTool::IsTransforming()
{
    return state.translation.dragTimer.Running();
}

//------------------------------------------------------------------------------
/**
*/
Editor::Entity
SelectionTool::GetSelectedEntityUnderMouse(
    Math::vec2 const& viewPortPosition, Math::vec2 const& viewPortSize, Editor::Camera const* camera
)
{
    Game::World* defaultWorld = Game::GetWorld(WORLD_DEFAULT);

    float closestDistance = 1e30f;
    Editor::Entity closestEntity = Editor::Entity::Invalid();

    Ptr<Input::Mouse> mouse = Input::InputServer::Instance()->GetDefaultMouse();
    Math::vec2 mousePos = mouse->GetScreenPosition();
    
    mousePos -= viewPortPosition;
    mousePos = {mousePos.x / viewPortSize.x, mousePos.y / viewPortSize.y};

    Math::mat4 const camTransform = Math::inverse(camera->GetViewTransform());
    Math::mat4 const invProj = Math::inverse(camera->GetProjectionTransform());

    Math::line ray = RenderUtil::MouseRayUtil::ComputeWorldMouseRay(mousePos, 10000, camTransform, invProj, 0.01f);

    for (IndexT i = 0; i < state.selection.Size(); i++)
    {
        Editor::Entity editorEntity = state.selection[i];
        Game::Entity const gameEntity = Editor::state.editables[editorEntity.index].gameEntity;
        
        Math::vec3 location = defaultWorld->GetComponent<Game::Position>(gameEntity);
        Math::bbox bbox = Math::bbox(location, Math::vector(0.5f, 0.5f, 0.5f));
        
        if (defaultWorld->HasComponent<GraphicsFeature::Model>(gameEntity))
        {
            Graphics::GraphicsEntityId const gfxEntity = defaultWorld->GetComponent<GraphicsFeature::Model>(gameEntity).graphicsEntityId;
            bbox = Models::ModelContext::ComputeBoundingBox(gfxEntity);
        }
        
        float dist;
        if (bbox.intersects(ray, dist))
        {
            if (dist < closestDistance)
            {
                closestDistance = dist;
                closestEntity = editorEntity;
            }
        }
    }

    return closestEntity;
}

//------------------------------------------------------------------------------
/**
*/
void
SelectionTool::SnapToGridIncrements(bool value)
{
    state.translation.useGridIncrements = value;
}

//------------------------------------------------------------------------------
/**
*/
bool
SelectionTool::SnapToGridIncrements()
{
    return state.translation.useGridIncrements;
}

//------------------------------------------------------------------------------
/**
*/
void
SelectionTool::UseIm3DGizmo(bool value)
{
    state.translation.useGizmo = value;
}

//------------------------------------------------------------------------------
/**
*/
bool
SelectionTool::UseIm3DGizmo()
{
    return state.translation.useGizmo;
}

} // namespace Tools
