//------------------------------------------------------------------------------
//  @file selectioncontext.cc
//  @copyright (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "selectioncontext.h"
#include "input/keyboard.h"
#include "input/mouse.h"
#include "input/inputserver.h"
#include "util/bvh.h"
#include "editor/components/editorcomponents.h"
#include "renderutil/mouserayutil.h"
#include "camera.h"
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

namespace Tools
{

__ImplementSingleton(SelectionContext)

enum class SelectionMode
{
    Replace = 0,
    Append = 1,
};

//------------------------------------------------------------------------------
/**
*/
SelectionContext::SelectionContext()
{
    __ConstructSingleton
}

//------------------------------------------------------------------------------
/**
*/
SelectionContext::~SelectionContext()
{
    __DestructSingleton
}

//------------------------------------------------------------------------------
/**
*/
void
SelectionContext::Create()
{
    Singleton = new SelectionContext;
}

//------------------------------------------------------------------------------
/**
*/
void
SelectionContext::Destroy()
{
    delete Singleton;
}

//------------------------------------------------------------------------------
/**
*/
Util::Array<Editor::Entity> const&
SelectionContext::Selection()
{
    return Singleton->selection;
}

//------------------------------------------------------------------------------
/**
*/
void
SelectionContext::PerformPicking(Math::vec2 const& viewPortPosition, Math::vec2 const& viewPortSize, Editor::Camera const* camera)
{
    if (Singleton->pauseCounter > 0)
    {
        return;
    }

    Ptr<Input::Mouse> mouse = Input::InputServer::Instance()->GetDefaultMouse();
    Ptr<Input::Keyboard> keyboard = Input::InputServer::Instance()->GetDefaultKeyboard();

    SelectionMode mode = (keyboard->KeyPressed(Input::Key::LeftControl) || keyboard->KeyPressed(Input::Key::RightControl))
                             ? SelectionMode::Append
                             : SelectionMode::Replace;

    Math::vec2 mousePos = mouse->GetPixelPosition();
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
        Game::FilterBuilder().Including<const Game::Entity, const GraphicsFeature::Model, const Editor::EditorEntity>().Build();

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
            newSelection = Singleton->selection;

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

        if (!newSelection.IsEmpty() || !Singleton->selection.IsEmpty())
        {
            Edit::SetSelection(newSelection);
        }
    }

    Game::DestroyFilter(filter);
}

//------------------------------------------------------------------------------
/**
*/
void
SelectionContext::ValidateSelection()
{
    for (SizeT i = 0; i < Singleton->selection.Size(); i++)
    {
        if (!Editor::state.editorWorld->IsValid(Singleton->selection[i]) ||
            !Editor::state.editorWorld->HasInstance(Singleton->selection[i]))
        {
            Singleton->selection.EraseIndex(i);
            i--;
            continue;
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
SelectionContext::Pause()
{
    Singleton->pauseCounter++;
}

//------------------------------------------------------------------------------
/**
*/
void
SelectionContext::Unpause()
{
    Singleton->pauseCounter--;
}

//------------------------------------------------------------------------------
/**
*/
bool
SelectionContext::IsPaused()
{
    return Singleton->pauseCounter > 0;
}

//------------------------------------------------------------------------------
/**
*/
Editor::Entity
SelectionContext::GetSelectedEntityUnderMouse(
    Math::vec2 const& viewPortPosition, Math::vec2 const& viewPortSize, Editor::Camera const* camera
)
{
    Game::World* defaultWorld = Game::GetWorld(WORLD_DEFAULT);

    float closestDistance = 1e30f;
    Editor::Entity closestEntity = Editor::Entity::Invalid();

    Ptr<Input::Mouse> mouse = Input::InputServer::Instance()->GetDefaultMouse();
    Math::vec2 mousePos = mouse->GetPixelPosition();

    mousePos -= viewPortPosition;
    mousePos = {mousePos.x / viewPortSize.x, mousePos.y / viewPortSize.y};

    Math::mat4 const camTransform = Math::inverse(camera->GetViewTransform());
    Math::mat4 const invProj = Math::inverse(camera->GetProjectionTransform());

    Math::line ray = RenderUtil::MouseRayUtil::ComputeWorldMouseRay(mousePos, 10000, camTransform, invProj, 0.01f);

    for (IndexT i = 0; i < Singleton->selection.Size(); i++)
    {
        Editor::Entity editorEntity = Singleton->selection[i];
        Game::Entity const gameEntity = Editor::state.editables[editorEntity.index].gameEntity;

        Math::vec3 location = defaultWorld->GetComponent<Game::Position>(gameEntity);
        Math::bbox bbox = Math::bbox(location, Math::vector(0.5f, 0.5f, 0.5f));

        if (defaultWorld->HasComponent<GraphicsFeature::Model>(gameEntity))
        {
            Graphics::GraphicsEntityId const gfxEntity =
                defaultWorld->GetComponent<GraphicsFeature::Model>(gameEntity).graphicsEntityId;
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

} // namespace Tools
