//------------------------------------------------------------------------------
//  @file hierarchymanager.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "hierarchymanager.h"
#include "basegamefeature/components/basegamefeature.h"
#include "basegamefeature/components/position.h"
#include "basegamefeature/components/orientation.h"
#include "basegamefeature/components/scale.h"
#include "game/gameserver.h"
#include "math/mat4.h"
#include "util/hashtable.h"

namespace Game
{
__ImplementClass(Game::HierarchyManager, 'HiMa', Game::Manager);

namespace
{
    static Game::Filter filter;
}

//--------------------------------------------------------------------------
/**
*/
void
HierarchyManager::OnActivate()
{
    filter = Game::FilterBuilder().Including<Game::Entity, Game::HTransform>().Build();
}

//--------------------------------------------------------------------------
/**
*/
void
HierarchyManager::OnDeactivate()
{
    Game::DestroyFilter(filter);
}

//------------------------------------------------------------------------------
/**
    @todo This currently re-calculates all transforms every frame, which might
          be expensive and unnecessary if many entities use hierarchies. Needs
          profiling and maybe a rework later.
*/
void
HierarchyManager::Resolve(World* world)
{
    Game::Dataset data = world->Query(filter);
    Util::Array<Game::Entity> entities;
    Util::Array<Game::HTransform> transforms;
    Util::HashTable<Game::Entity, IndexT> entityIndices;

    for (int viewIndex = 0; viewIndex < data.numViews; viewIndex++)
    {
        Game::Dataset::View const& view = data.views[viewIndex];
        Game::Entity const* const viewEntities = (Game::Entity*)view.buffers[0];
        Game::HTransform const* const viewTransforms = (Game::HTransform*)view.buffers[1];
        for (IndexT i = 0; i < view.numInstances; i++)
        {
            if (!view.validInstances.IsSet(i))
            {
                continue;
            }
            entityIndices.Add(viewEntities[i], entities.Size());
            entities.Append(viewEntities[i]);
            transforms.Append(viewTransforms[i]);
        }
    }

    Util::Array<uint8_t> states;
    Util::Array<uint8_t> resolvable;
    for (IndexT i = 0; i < entities.Size(); i++)
    {
        states.Append(0);
        resolvable.Append(1);
    }

    Util::Array<IndexT> ordered;
    Util::Array<IndexT> chain;
    auto detach = [world, &entities, &transforms](IndexT entityIndex)
    {
        Game::HTransform& transform = transforms[entityIndex];
        transform.parent = Game::Entity::Invalid();
        transform.localPosition = world->GetComponent<Game::Position>(entities[entityIndex]);
        transform.localOrientation = world->GetComponent<Game::Orientation>(entities[entityIndex]);
        transform.localScale = world->GetComponent<Game::Scale>(entities[entityIndex]);
        world->SetComponent<Game::HTransform>(entities[entityIndex], transform);
        world->MarkAsModified(entities[entityIndex]);
    };
    for (IndexT start = 0; start < entities.Size(); start++)
    {
        if (states[start] != 0)
        {
            continue;
        }

        chain.Clear();
        IndexT current = start;
        while (states[current] == 0)
        {
            states[current] = 1;
            chain.Append(current);

            Game::HTransform& transform = transforms[current];
            Game::Entity const parent = transform.parent;
            if (parent == Game::Entity::Invalid())
            {
                break;
            }
            if (parent.world != world->GetWorldId())
            {
                detach(current);
                break;
            }
            if (!world->IsValid(parent) || !world->HasInstance(parent))
            {
                detach(current);
                break;
            }

            IndexT const parentMapIndex = entityIndices.FindIndex(parent);
            if (parentMapIndex == InvalidIndex)
            {
                break;
            }
            IndexT const parentIndex = entityIndices.ValueAtIndex(parent, parentMapIndex);
            if (states[parentIndex] == 1)
            {
                n_warning("Cycle detected in transform hierarchy; detaching entity %llu\n", (unsigned long long)(uint64_t)entities[current]);
                detach(current);
                break;
            }
            if (states[parentIndex] == 2)
            {
                break;
            }
            current = parentIndex;
        }

        for (IndexT i = chain.Size(); i > 0; i--)
        {
            IndexT const entityIndex = chain[i - 1];
            states[entityIndex] = 2;
            ordered.Append(entityIndex);
        }
    }

    for (IndexT const entityIndex : ordered)
    {
        if (resolvable[entityIndex] == 0)
        {
            continue;
        }

        Game::HTransform const& transform = transforms[entityIndex];
        Math::vec3 position = transform.localPosition;
        Math::quat orientation = transform.localOrientation;
        Math::vec3 scale = transform.localScale;
        if (transform.parent != Game::Entity::Invalid())
        {
            IndexT const parentMapIndex = entityIndices.FindIndex(transform.parent);
            if (parentMapIndex != InvalidIndex)
            {
                IndexT const parentIndex = entityIndices.ValueAtIndex(transform.parent, parentMapIndex);
                if (resolvable[parentIndex] == 0)
                {
                    resolvable[entityIndex] = 0;
                    continue;
                }
            }
            Game::Position const parentPosition = world->GetComponent<Game::Position>(transform.parent);
            Game::Orientation const parentOrientation = world->GetComponent<Game::Orientation>(transform.parent);
            Game::Scale const parentScale = world->GetComponent<Game::Scale>(transform.parent);
            Math::mat4 const parentTransform = Math::trs(parentPosition, parentOrientation, parentScale);
            position = Math::vec3(parentTransform * Math::point(transform.localPosition));
            orientation = transform.localOrientation * parentOrientation;
            scale = transform.localScale * parentScale;
        }

        Game::Entity const entity = entities[entityIndex];
        Game::Position const oldPosition = world->GetComponent<Game::Position>(entity);
        Game::Orientation const oldOrientation = world->GetComponent<Game::Orientation>(entity);
        Game::Scale const oldScale = world->GetComponent<Game::Scale>(entity);
        if (oldPosition != position || oldOrientation != orientation || oldScale != scale)
        {
            world->SetComponent<Game::Position>(entity, Game::Position(position));
            world->SetComponent<Game::Orientation>(entity, Game::Orientation(orientation));
            world->SetComponent<Game::Scale>(entity, Game::Scale(scale));
            world->MarkAsModified(entity);
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
HierarchyManager::OnBeginFrame()
{
    for (uint32_t worldIndex = 0; worldIndex < Game::GameServer::Instance()->state.numWorlds; worldIndex++)
    {
        Game::World* world = Game::GameServer::Instance()->state.worlds[worldIndex];
        if (world != nullptr)
        {
            HierarchyManager::Resolve(world);
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
HierarchyManager::OnEndFrame()
{
    for (uint32_t worldIndex = 0; worldIndex < Game::GameServer::Instance()->state.numWorlds; worldIndex++)
    {
        Game::World* world = Game::GameServer::Instance()->state.worlds[worldIndex];
        if (world != nullptr)
        {
            HierarchyManager::Resolve(world);
        }
    }
}



} // namespace Game
