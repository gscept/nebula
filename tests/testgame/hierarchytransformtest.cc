//------------------------------------------------------------------------------
//  @file hierarchytransformtest.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "hierarchytransformtest.h"
#include "basegamefeature/components/basegamefeature.h"
#include "basegamefeature/components/position.h"
#include "basegamefeature/components/orientation.h"
#include "basegamefeature/components/scale.h"
#include "basegamefeature/managers/hierarchymanager.h"
#include "game/world.h"

namespace Test
{
__ImplementClass(Test::HierarchyTransformTest, 'HTTs', Test::TestCase);

//------------------------------------------------------------------------------
/**
*/
void
HierarchyTransformTest::Run()
{
    Game::World world(Game::WorldHash('HTST'), 10);
    Game::Entity const root = world.CreateEntity(true);
    Game::Entity const child = world.CreateEntity(true);
    Game::Entity const grandchild = world.CreateEntity(true);

    world.SetComponent<Game::Position>(root, Game::Position(10.0f, 0.0f, 0.0f));

    Game::HTransform* childTransform = world.AddComponent<Game::HTransform>(child);
    childTransform->parent = root;
    childTransform->localPosition = Math::vec3(1.0f, 0.0f, 0.0f);

    Game::HTransform* grandchildTransform = world.AddComponent<Game::HTransform>(grandchild);
    grandchildTransform->parent = child;
    grandchildTransform->localPosition = Math::vec3(2.0f, 0.0f, 0.0f);
    world.ExecuteAddComponentCommands();

    Game::HierarchyManager::Resolve(&world);
    VERIFY(Math::nearequal(world.GetComponent<Game::Position>(child), Math::vec3(11.0f, 0.0f, 0.0f), 0.0001f));
    VERIFY(Math::nearequal(world.GetComponent<Game::Position>(grandchild), Math::vec3(13.0f, 0.0f, 0.0f), 0.0001f));

    world.SetComponent<Game::Position>(root, Game::Position(20.0f, 0.0f, 0.0f));
    Game::HierarchyManager::Resolve(&world);
    VERIFY(Math::nearequal(world.GetComponent<Game::Position>(child), Math::vec3(21.0f, 0.0f, 0.0f), 0.0001f));
    VERIFY(Math::nearequal(world.GetComponent<Game::Position>(grandchild), Math::vec3(23.0f, 0.0f, 0.0f), 0.0001f));

    world.AddComponent<Game::IsActive>(child);
    world.ExecuteAddComponentCommands();
    Game::HierarchyManager::Resolve(&world);
    VERIFY(Math::nearequal(world.GetComponent<Game::Position>(child), Math::vec3(21.0f, 0.0f, 0.0f), 0.0001f));

    Game::Orientation const rootOrientation(Math::from_euler(Math::vec3(0.0f, 0.0f, N_PI * 0.5f)));
    world.SetComponent<Game::Orientation>(root, rootOrientation);
    world.SetComponent<Game::Scale>(root, Game::Scale(Math::vec3(2.0f, 2.0f, 2.0f)));
    Game::HierarchyManager::Resolve(&world);
    Math::mat4 const expectedChildTransform = Math::trs(
        Math::vec3(1.0f, 0.0f, 0.0f),
        Math::quat(),
        Math::vec3(1.0f, 1.0f, 1.0f)
    ) * Math::trs(Math::vec3(20.0f, 0.0f, 0.0f), rootOrientation, Math::vec3(2.0f, 2.0f, 2.0f));
    VERIFY(Math::nearequal(world.GetComponent<Game::Position>(child), Math::vec3(expectedChildTransform.r[3]), 0.0001f));
    Math::mat4 const actualChildTransform = Math::trs(
        world.GetComponent<Game::Position>(child),
        world.GetComponent<Game::Orientation>(child),
        world.GetComponent<Game::Scale>(child)
    );
    VERIFY(Math::nearequal(
        Math::vec3(actualChildTransform * Math::vector(1.0f, 0.0f, 0.0f)),
        Math::vec3(expectedChildTransform * Math::vector(1.0f, 0.0f, 0.0f)),
        0.0001f
    ));

    world.SetComponent<Game::Scale>(root, Game::Scale(Math::vec3(0.0f, 0.0f, 0.0f)));
    Game::HierarchyManager::Resolve(&world);
    VERIFY(world.GetComponent<Game::Scale>(child) == Game::Scale(Math::vec3(0.0f, 0.0f, 0.0f)));

    Game::Entity const staleParent = world.AllocateEntityId();
    world.DeallocateEntityId(staleParent);
    Game::Entity const orphan = world.CreateEntity(true);
    world.SetComponent<Game::Position>(orphan, Game::Position(5.0f, 0.0f, 0.0f));
    Game::HTransform* orphanTransform = world.AddComponent<Game::HTransform>(orphan);
    orphanTransform->parent = staleParent;
    world.ExecuteAddComponentCommands();

    Game::HierarchyManager::Resolve(&world);
    Game::HTransform const resolvedOrphan = world.GetComponent<Game::HTransform>(orphan);
    VERIFY(resolvedOrphan.parent == Game::Entity::Invalid());
    VERIFY(Math::nearequal(resolvedOrphan.localPosition, Math::vec3(5.0f, 0.0f, 0.0f), 0.0001f));
    VERIFY(Math::nearequal(world.GetComponent<Game::Position>(orphan), Math::vec3(5.0f, 0.0f, 0.0f), 0.0001f));

    Game::HTransform rootTransform;
    rootTransform.parent = grandchild;
    rootTransform.localPosition = world.GetComponent<Game::Position>(root);
    world.AddComponent<Game::HTransform>(root, rootTransform);
    world.ExecuteAddComponentCommands();

    Game::HierarchyManager::Resolve(&world);
    uint detached = 0;
    detached += world.GetComponent<Game::HTransform>(root).parent == Game::Entity::Invalid() ? 1 : 0;
    detached += world.GetComponent<Game::HTransform>(child).parent == Game::Entity::Invalid() ? 1 : 0;
    detached += world.GetComponent<Game::HTransform>(grandchild).parent == Game::Entity::Invalid() ? 1 : 0;
    VERIFY(detached > 0);
}

} // namespace Test
