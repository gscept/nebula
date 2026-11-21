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
#include "navmeshsources.h"
#include "navigationfeature/components/navigation.h"

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
Editor::Entity const&
SelectionContext::Hovered()
{
    return Singleton->hovered;
}

//------------------------------------------------------------------------------
/**
*/
void
SelectionContext::SetHovered(Editor::Entity entity)
{
    Singleton->hovered = entity;
}

//------------------------------------------------------------------------------
/**
*/
void
SelectionContext::ClearHovered()
{
    Singleton->hovered = Editor::Entity::Invalid();
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

    // ctrl or shift toggle the picked entity in the selection, a plain click replaces it
    SelectionMode mode = (keyboard->KeyPressed(Input::Key::LeftControl) || keyboard->KeyPressed(Input::Key::RightControl) ||
                          keyboard->KeyPressed(Input::Key::LeftShift) || keyboard->KeyPressed(Input::Key::RightShift))
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

    // navigation area volumes have no model, they are picked by their box
    for (const Editor::NavAreaVolume& volume : Editor::GetNavAreaVolumes())
    {
        Editor::EditorEntity editorEntity;
        editorEntity.id = (uint64_t)volume.entity;
        editorEntities.Append(editorEntity);
        bboxes.Append(Editor::NavAreaWorldBounds(volume));
    }

    if (bboxes.Size() > 0)
    {
        // TODO: pretty unnecessary to build a bvh for just a single raycast. Maybe cache it and do incremental updates when moving and adding things.
        bvh.Build(bboxes.Begin(), bboxes.Size());
        Util::Array<uint32_t> intersectionIndices = bvh.Intersect(ray);

        Util::Array<Editor::Entity> newSelection;

        if (mode == SelectionMode::Append)
            newSelection = Singleton->selection;

        // the boxes the ray hits, with the distance it enters them at
        Util::Array<uint32_t> hits;
        Util::Array<float> hitDistances;
        for (IndexT i = 0; i < intersectionIndices.Size(); i++)
        {
            uint32_t const idx = intersectionIndices[i];
            float t;
            if (bboxes[idx].intersects(ray, t))
            {
                hits.Append(idx);
                hitDistances.Append(t);
            }
        }

        // A large box, like the one of a terrain, holds the boxes of everything placed on it and is
        // usually entered first, or holds the camera. Pick the nearest of the boxes which hold no other
        // hit box, so an object wins over the ground it stands on. Holding is checked by the center, an
        // object may reach out of the ground's box, below a valley floor for instance
        auto volume = [](Math::bbox const& box)
        {
            Math::vec3 const size = box.size();
            return size.x * size.y * size.z;
        };
        float closestDist = 1e30f;
        Editor::Entity closestEntity = Editor::Entity::Invalid();
        for (IndexT i = 0; i < hits.Size(); i++)
        {
            Math::bbox const& box = bboxes[hits[i]];
            bool holdsOther = false;
            for (IndexT j = 0; j < hits.Size() && !holdsOther; j++)
            {
                Math::bbox const& other = bboxes[hits[j]];
                Math::point const center = other.center();
                holdsOther = j != i && box.contains(Math::vec3(center.x, center.y, center.z)) && volume(box) > volume(other);
            }
            if (!holdsOther && hitDistances[i] < closestDist)
            {
                closestEntity = editorEntities[hits[i]].id;
                closestDist = hitDistances[i];
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
        else if (defaultWorld->HasComponent<NavigationFeature::NavArea>(gameEntity))
        {
            for (const Editor::NavAreaVolume& volume : Editor::GetNavAreaVolumes())
            {
                if (volume.gameEntity == gameEntity)
                {
                    bbox = Editor::NavAreaWorldBounds(volume);
                }
            }
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
