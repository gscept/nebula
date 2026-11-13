//------------------------------------------------------------------------------
//  @file hierarchytransformtest.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "hierarchytransformtest.h"
#include "memory/memory.h"
#include "memdb/attributeregistry.h"
#include "memdb/database.h"
#include "memdb/table.h"
#include "basegamefeature/components/basegamefeature.h"
#include "basegamefeature/components/position.h"
#include "basegamefeature/components/orientation.h"
#include "basegamefeature/components/scale.h"
#include "basegamefeature/managers/transformhierarchy.h"
#include "game/world.h"

namespace Test
{
__ImplementClass(Test::HierarchyTransformTest, 'HTTs', Test::TestCase);

namespace
{
    const float Epsilon = 0.0001f;

    // Math::nearequal(vec3, vec3, float) is true when any lane is within the epsilon rather than when all of them
    // are, so the comparisons here are written one lane at a time
    bool
    Near(Math::vec3 const& v, float x, float y, float z)
    {
        return Math::nearequal(v.x, x, Epsilon) && Math::nearequal(v.y, y, Epsilon) && Math::nearequal(v.z, z, Epsilon);
    }

    bool
    Near(Math::quat const& q, float x, float y, float z, float w)
    {
        return Math::nearequal(q.x, x, Epsilon) && Math::nearequal(q.y, y, Epsilon) &&
               Math::nearequal(q.z, z, Epsilon) && Math::nearequal(q.w, w, Epsilon);
    }

    // The transform an entity holds before anything is computed. It is written here so the numbers the test
    // compares against do not depend on what an uninitialized component column holds
    void
    SetStart(Game::World& world, Game::Entity entity, Math::vec3 const& position, Math::quat const& orientation, Math::vec3 const& scale)
    {
        world.SetComponent<Game::Position>(entity, Game::Position(position));
        world.SetComponent<Game::Orientation>(entity, Game::Orientation(orientation));
        world.SetComponent<Game::Scale>(entity, Game::Scale(scale));
    }

    Game::HTransform
    Local(Game::Entity parent, Math::vec3 const& position, Math::quat const& orientation, Math::vec3 const& scale)
    {
        Game::HTransform transform;
        transform.parent = parent;
        transform.localPosition = position;
        transform.localOrientation = orientation;
        transform.localScale = scale;
        return transform;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
HierarchyTransformTest::Run()
{
    Game::World world(Game::WorldHash('HTST'), 10);
    Game::TransformHierarchy* hierarchy = world.GetHierarchy();

    Math::quat const identity;
    Math::vec3 const unit(1.0f, 1.0f, 1.0f);

    Game::Entity const root = world.CreateEntity(true);
    Game::Entity const child = world.CreateEntity(true);
    Game::Entity const grandchild = world.CreateEntity(true);

    SetStart(world, root, Math::vec3(10.0f, 0.0f, 0.0f), identity, unit);

    // An entity which has no local transform is not in the hierarchy, and a write on it is just a write
    VERIFY(!hierarchy->IsInHierarchy(root));

    world.AddComponent<Game::HTransform>(child, Local(root, Math::vec3(1.0f, 0.0f, 0.0f), identity, unit));
    world.AddComponent<Game::HTransform>(grandchild, Local(child, Math::vec3(2.0f, 0.0f, 0.0f), identity, unit));
    world.ExecuteAddComponentCommands();

    // The add propagates when the command executes, because the instance did not exist when the values were staged
    VERIFY(Near(world.GetComponent<Game::Position>(child), 11.0f, 0.0f, 0.0f));
    VERIFY(Near(world.GetComponent<Game::Position>(grandchild), 13.0f, 0.0f, 0.0f));

    // The root is in the hierarchy because something below it is, even though it has no local transform of its own
    VERIFY(hierarchy->IsInHierarchy(root));
    VERIFY(hierarchy->IsInHierarchy(child));
    VERIFY(hierarchy->GetFirstChild(hierarchy->GetInstance(root)) == hierarchy->GetInstance(child));

    // A write on a hierarchy member propagates at the time it happens, without a resolve
    world.SetComponent<Game::Position>(root, Game::Position(20.0f, 0.0f, 0.0f));
    VERIFY(Near(world.GetComponent<Game::Position>(child), 21.0f, 0.0f, 0.0f));
    VERIFY(Near(world.GetComponent<Game::Position>(grandchild), 23.0f, 0.0f, 0.0f));

    // Moving an entity to another table does not move it out of the hierarchy, because the instance is keyed by the
    // entity and the components are read through the table the entity is in
    world.AddComponent<Game::IsActive>(child);
    world.ExecuteAddComponentCommands();
    VERIFY(Near(world.GetComponent<Game::Position>(child), 21.0f, 0.0f, 0.0f));

    // An entity which is not part of a hierarchy pays nothing for one
    Game::Entity const plain = world.CreateEntity(true);
    SetStart(world, plain, Math::vec3(7.0f, 8.0f, 9.0f), identity, unit);
    VERIFY(!hierarchy->IsInHierarchy(plain));
    VERIFY(hierarchy->GetInstance(plain) == Game::TransformHierarchy::InvalidInstance);
    VERIFY(Near(world.GetComponent<Game::Position>(plain), 7.0f, 8.0f, 9.0f));

    // A rotated and uniformly scaled parent. The composition is done in translation, rotation and scale lanes
    // rather than with a matrix, and for a uniform parent scale that is the same as the matrix composition
    Math::quat const rootRotation(Math::from_euler(Math::vec3(0.0f, 0.0f, N_PI * 0.5f)));
    world.SetComponent<Game::Orientation>(root, Game::Orientation(rootRotation));
    world.SetComponent<Game::Scale>(root, Game::Scale(Math::vec3(2.0f, 2.0f, 2.0f)));

    // Applying the local first and then the parent is what a node in a parent is, which in Nebula's multiplication
    // order is the parent matrix times the local matrix
    Math::mat4 const expectedChild =
        Math::trs(Math::vec3(20.0f, 0.0f, 0.0f), rootRotation, Math::vec3(2.0f, 2.0f, 2.0f)) *
        Math::trs(Math::vec3(1.0f, 0.0f, 0.0f), identity, unit);
    VERIFY(Near(Math::vec3(expectedChild.r[3]), 20.0f, 2.0f, 0.0f));
    VERIFY(Near(world.GetComponent<Game::Position>(child), 20.0f, 2.0f, 0.0f));
    VERIFY(Near(world.GetComponent<Game::Orientation>(child), rootRotation.x, rootRotation.y, rootRotation.z, rootRotation.w));

    // points are scaled along the local axes, then rotated, then translated
    VERIFY(Near(world.GetComponent<Game::Position>(grandchild), 20.0f, 6.0f, 0.0f));

    // A world transform written on a member is what the member keeps, and the local it implies is derived through
    // the inverse of the parent, which is what makes the two agree
    world.SetComponent<Game::Position>(child, Game::Position(30.0f, 0.0f, 0.0f));
    VERIFY(Near(world.GetComponent<Game::Position>(child), 30.0f, 0.0f, 0.0f));
    VERIFY(Near(world.GetComponent<Game::HTransform>(child).localPosition, 0.0f, -5.0f, 0.0f));
    VERIFY(Near(world.GetComponent<Game::Position>(grandchild), 30.0f, 4.0f, 0.0f));

    // Deriving the local and composing it back gives the world that was written, which is what makes the
    // composition and its inverse the same algebra
    Math::mat4 const written = Math::trs(Math::vec3(30.0f, 0.0f, 0.0f), rootRotation, Math::vec3(2.0f, 2.0f, 2.0f));
    hierarchy->SetWorldTransform(hierarchy->GetInstance(child), written);
    VERIFY(Near(Math::vec3(hierarchy->GetLocalTransform(child).r[3]), 0.0f, -5.0f, 0.0f));
    VERIFY(Near(world.GetComponent<Game::Position>(child), 30.0f, 0.0f, 0.0f));
    VERIFY(Near(world.GetComponent<Game::Position>(grandchild), 30.0f, 4.0f, 0.0f));

    // Taking the component away takes the entity out of the hierarchy, and its children become roots with the world
    // transform they have
    world.RemoveComponent<Game::HTransform>(child);
    world.ExecuteRemoveComponentCommands();

    VERIFY(!hierarchy->IsInHierarchy(child));
    VERIFY(hierarchy->GetInstance(child) == Game::TransformHierarchy::InvalidInstance);
    VERIFY(world.GetComponent<Game::HTransform>(grandchild).parent == Game::Entity::Invalid());
    VERIFY(Near(world.GetComponent<Game::HTransform>(grandchild).localPosition, 30.0f, 4.0f, 0.0f));
    VERIFY(Near(world.GetComponent<Game::Position>(grandchild), 30.0f, 4.0f, 0.0f));

    // An entity whose parent chain comes back to it is refused when the link is made. The component is the authority
    // for the parent, so it keeps the name it was written with, and the entity which closed the cycle becomes a root
    Game::Entity const cycleA = world.CreateEntity(true);
    Game::Entity const cycleB = world.CreateEntity(true);
    SetStart(world, cycleA, Math::vec3(0.0f, 0.0f, 0.0f), identity, unit);
    SetStart(world, cycleB, Math::vec3(2.0f, 1.0f, 0.0f), identity, unit);

    world.AddComponent<Game::HTransform>(cycleA, Local(cycleB, Math::vec3(1.0f, 1.0f, 0.0f), identity, unit));
    world.AddComponent<Game::HTransform>(cycleB, Local(cycleA, Math::vec3(2.0f, 1.0f, 0.0f), identity, unit));
    world.ExecuteAddComponentCommands();

    VERIFY(hierarchy->GetParent(hierarchy->GetInstance(cycleA)) == hierarchy->GetInstance(cycleB));
    VERIFY(hierarchy->GetParent(hierarchy->GetInstance(cycleB)) == Game::TransformHierarchy::InvalidInstance);
    VERIFY(world.GetComponent<Game::HTransform>(cycleA).parent == cycleB);
    VERIFY(world.GetComponent<Game::HTransform>(cycleB).parent == cycleA);
    VERIFY(Near(world.GetComponent<Game::Position>(cycleB), 2.0f, 1.0f, 0.0f));
    VERIFY(Near(world.GetComponent<Game::Position>(cycleA), 3.0f, 2.0f, 0.0f));

    // Erasing an instance moves the last instance of the hierarchy into the slot it frees, and the links which named
    // that slot have to name the slot the moved instance lands in. The instance erased here is not the last one, and
    // the last one is a parent, so both its sibling links and the instances below it name the slot it came from
    Game::Entity const branch = world.CreateEntity(true);
    Game::Entity const first = world.CreateEntity(true);
    Game::Entity const second = world.CreateEntity(true);
    Game::Entity const third = world.CreateEntity(true);
    SetStart(world, branch, Math::vec3(50.0f, 0.0f, 0.0f), identity, unit);

    world.AddComponent<Game::HTransform>(first, Local(branch, Math::vec3(1.0f, 0.0f, 0.0f), identity, unit));
    world.AddComponent<Game::HTransform>(second, Local(branch, Math::vec3(2.0f, 0.0f, 0.0f), identity, unit));
    world.AddComponent<Game::HTransform>(third, Local(branch, Math::vec3(3.0f, 0.0f, 0.0f), identity, unit));
    world.ExecuteAddComponentCommands();

    // children are prepended, so the last added child is the first one in the child list
    uint32_t const branchInstance = hierarchy->GetInstance(branch);
    VERIFY(hierarchy->GetFirstChild(branchInstance) == hierarchy->GetInstance(third));
    VERIFY(hierarchy->GetNextSibling(hierarchy->GetInstance(third)) == hierarchy->GetInstance(second));
    VERIFY(hierarchy->GetNextSibling(hierarchy->GetInstance(second)) == hierarchy->GetInstance(first));
    VERIFY(hierarchy->GetPreviousSibling(hierarchy->GetInstance(first)) == hierarchy->GetInstance(second));
    VERIFY(Near(world.GetComponent<Game::Position>(first), 51.0f, 0.0f, 0.0f));
    VERIFY(Near(world.GetComponent<Game::Position>(second), 52.0f, 0.0f, 0.0f));
    VERIFY(Near(world.GetComponent<Game::Position>(third), 53.0f, 0.0f, 0.0f));

    world.RemoveComponent<Game::HTransform>(first);
    world.ExecuteRemoveComponentCommands();

    uint32_t const movedInstance = hierarchy->GetInstance(third);
    uint32_t const secondInstance = hierarchy->GetInstance(second);
    uint32_t const branchAfterRemoval = hierarchy->GetInstance(branch);
    VERIFY(hierarchy->GetInstance(first) == Game::TransformHierarchy::InvalidInstance);
    VERIFY(hierarchy->GetFirstChild(branchAfterRemoval) == movedInstance);
    VERIFY(hierarchy->GetNextSibling(movedInstance) == secondInstance);
    VERIFY(hierarchy->GetPreviousSibling(secondInstance) == movedInstance);
    VERIFY(hierarchy->GetParent(movedInstance) == branchAfterRemoval);
    VERIFY(hierarchy->GetParent(secondInstance) == branchAfterRemoval);
    VERIFY(hierarchy->GetPreviousSibling(movedInstance) == Game::TransformHierarchy::InvalidInstance);

    world.SetComponent<Game::Position>(branch, Game::Position(60.0f, 0.0f, 0.0f));
    VERIFY(Near(world.GetComponent<Game::Position>(second), 62.0f, 0.0f, 0.0f));
    VERIFY(Near(world.GetComponent<Game::Position>(third), 63.0f, 0.0f, 0.0f));

    // An instance which only exists to be a parent has no component whose decay could remove it, so an instance can
    // outlive its entity. The walk which finds it queues it for the flush, because the erasure moves another instance
    // into the slot it frees while the walk is using the slots
    Game::Entity const deadParent = world.CreateEntity(false);
    Game::Entity const below = world.CreateEntity(true);
    SetStart(world, below, Math::vec3(10.0f, 0.0f, 0.0f), identity, unit);

    world.AddComponent<Game::HTransform>(below, Local(deadParent, Math::vec3(4.0f, 0.0f, 0.0f), identity, unit));
    world.ExecuteAddComponentCommands();

    // A parent which is not in a table cannot be composed with, which is what the pending set is for
    uint32_t const parentInstance = hierarchy->GetInstance(deadParent);
    VERIFY(parentInstance != Game::TransformHierarchy::InvalidInstance);
    VERIFY(hierarchy->GetParent(hierarchy->GetInstance(below)) == parentInstance);
    VERIFY(hierarchy->NumPending() > 0);
    VERIFY(Near(world.GetComponent<Game::Position>(below), 10.0f, 0.0f, 0.0f));

    world.DeleteEntity(deadParent);

    // The write which walks past the stale instance is what finds it, and the flush takes it out at the end of the
    // write. The component names a parent which is gone, so the entity becomes a root with the world it was written
    // with
    world.SetComponent<Game::Position>(below, Game::Position(20.0f, 0.0f, 0.0f));
    VERIFY(hierarchy->GetInstance(deadParent) == Game::TransformHierarchy::InvalidInstance);
    VERIFY(hierarchy->GetParent(hierarchy->GetInstance(below)) == Game::TransformHierarchy::InvalidInstance);
    VERIFY(world.GetComponent<Game::HTransform>(below).parent == Game::Entity::Invalid());
    VERIFY(Near(world.GetComponent<Game::HTransform>(below).localPosition, 20.0f, 0.0f, 0.0f));
    VERIFY(Near(world.GetComponent<Game::Position>(below), 20.0f, 0.0f, 0.0f));

    // below is still in the pending set from before its parent was deleted. Computing it again gives the world it
    // has, because a root composes its local with nothing
    hierarchy->ResolvePending();
    VERIFY(hierarchy->NumPending() == 0);
    VERIFY(Near(world.GetComponent<Game::Position>(below), 20.0f, 0.0f, 0.0f));

    // An entity id is reused when the entity which had it is deleted, and the mapping from an entity to its instance
    // is keyed by the index alone, so an instance which outlived its entity has to be checked against the entity it
    // names. Otherwise a new entity with the same id inherits the instance of the one which is gone
    Game::Entity const oldParent = world.CreateEntity(false);
    Game::Entity const reusedChild = world.CreateEntity(true);
    SetStart(world, reusedChild, Math::vec3(10.0f, 0.0f, 0.0f), identity, unit);

    world.AddComponent<Game::HTransform>(reusedChild, Local(oldParent, Math::vec3(5.0f, 0.0f, 0.0f), identity, unit));
    world.ExecuteAddComponentCommands();

    uint32_t const oldInstance = hierarchy->GetInstance(oldParent);
    VERIFY(oldInstance != Game::TransformHierarchy::InvalidInstance);

    world.DeleteEntity(oldParent);

    // The pool does not have to hand the freed index back, so the entity checked here is built with the same index
    // and another generation, which is what a reused id is. The instance still belongs to the entity it was made for
    Game::Entity reusedId = oldParent;
    reusedId.generation++;
    VERIFY(hierarchy->GetInstance(oldParent) == oldInstance);
    VERIFY(hierarchy->GetInstance(reusedId) == Game::TransformHierarchy::InvalidInstance);

    // The parent the component names gets an instance of its own, and the child is linked under that one instead of
    // the stale one
    Game::Entity const newParent = world.CreateEntity(true);
    SetStart(world, newParent, Math::vec3(100.0f, 0.0f, 0.0f), identity, unit);
    Game::HTransform updated = world.GetComponent<Game::HTransform>(reusedChild);
    updated.parent = newParent;
    world.SetComponent<Game::HTransform>(reusedChild, updated);

    uint32_t const newInstance = hierarchy->GetInstance(newParent);
    VERIFY(newInstance != Game::TransformHierarchy::InvalidInstance);
    VERIFY(newInstance != oldInstance);
    VERIFY(hierarchy->GetParent(hierarchy->GetInstance(reusedChild)) == newInstance);
    VERIFY(Near(world.GetComponent<Game::Position>(reusedChild), 105.0f, 0.0f, 0.0f));

    // An entity which has no instance yet is put in the pending set, and is computed when its components and its
    // parent's are in a table
    Game::Entity const lateParent = world.CreateEntity(false);
    Game::Entity const lateChild = world.CreateEntity(true);
    SetStart(world, lateChild, Math::vec3(10.0f, 0.0f, 0.0f), identity, unit);
    world.AddComponent<Game::HTransform>(lateChild, Local(lateParent, Math::vec3(3.0f, 0.0f, 0.0f), identity, unit));
    world.ExecuteAddComponentCommands();
    VERIFY(hierarchy->NumPending() > 0);

    world.AllocateInstance(lateParent);
    world.InitializeInstance(lateParent);
    SetStart(world, lateParent, Math::vec3(100.0f, 0.0f, 0.0f), identity, unit);
    hierarchy->ResolvePending();
    VERIFY(hierarchy->NumPending() == 0);
    VERIFY(Near(world.GetComponent<Game::Position>(lateChild), 103.0f, 0.0f, 0.0f));

    // The order the composition is in. Math::quat::operator* applies the left operand first, which is the opposite
    // of how mat4 multiplies, so composing a child with its parent is local * parent here. This is the order the
    // point above agrees with, the order trs(parent) * trs(local) gives, the order the editor reads a local
    // transform through a parent, and the order the old resolve used. In the other order the orientation is a
    // different rotation and it does not match its own translation
    {
        Game::Entity const rotatedRoot = world.CreateEntity(true);
        Game::Entity const rotatedChild = world.CreateEntity(true);
        SetStart(world, rotatedRoot, Math::vec3(0.0f, 0.0f, 0.0f), identity, unit);
        SetStart(world, rotatedChild, Math::vec3(0.0f, 0.0f, 0.0f), identity, unit);

        Math::quat const parentRotation(0.0f, 0.0f, Math::sin(N_PI * 0.25f), Math::cos(N_PI * 0.25f));   // 90 degrees about z
        Math::quat const childRotation(Math::sin(N_PI * 0.25f), 0.0f, 0.0f, Math::cos(N_PI * 0.25f));     // 90 degrees about x
        Math::vec3 const x(1.0f, 0.0f, 0.0f);

        world.SetComponent<Game::Orientation>(rotatedRoot, Game::Orientation(parentRotation));
        world.AddComponent<Game::HTransform>(rotatedChild, Local(rotatedRoot, x, childRotation, unit));
        world.ExecuteAddComponentCommands();

        // A point in the child is rotated by the child and then by the parent, which puts x on y
        VERIFY(Near(world.GetComponent<Game::Position>(rotatedChild), 0.0f, 1.0f, 0.0f));
        VERIFY(Near(Math::rotate(world.GetComponent<Game::Orientation>(rotatedChild), x), 0.0f, 1.0f, 0.0f));

        // The same rotation as a quaternion. Written in the other order it is (0.5, -0.5, 0.5, 0.5), which turns x
        // into z, and that is what the composition and the translation disagree about
        Math::vec3 outScale;
        Math::quat outRotation;
        Math::vec3 outTranslation;
        Math::decompose(
            Math::trs(Math::vec3(0.0f, 0.0f, 0.0f), parentRotation, unit) * Math::trs(x, childRotation, unit),
            outScale, outRotation, outTranslation);
        VERIFY(Near(world.GetComponent<Game::Orientation>(rotatedChild), outRotation.x, outRotation.y, outRotation.z, outRotation.w));
        VERIFY(Near(world.GetComponent<Game::Position>(rotatedChild), outTranslation.x, outTranslation.y, outTranslation.z));

        // A world transform written on a hierarchy member is derived back through the parent, so the local it
        // derives has to compose to the value that was written
        world.SetComponent<Game::Orientation>(rotatedChild, Game::Orientation(outRotation));
        Game::HTransform const derived = world.GetComponent<Game::HTransform>(rotatedChild);
        VERIFY(Near(derived.localOrientation, childRotation.x, childRotation.y, childRotation.z, childRotation.w));
    }

    // An instance which gets its components by being copied rather than by a command is put in the hierarchy when
    // the instance is initialized, which is the path an entity is mirrored into the sim world by. Copying bytes
    // into a row is not a component write, so the copy on its own does not put the entity in the hierarchy, and a
    // write on an entity which is not in it moves only that entity
    {
        Game::Entity const copied = world.CreateEntity(true);
        SetStart(world, copied, Math::vec3(103.0f, 0.0f, 0.0f), identity, unit);
        world.AddComponent<Game::HTransform>(copied, Local(lateParent, Math::vec3(3.0f, 0.0f, 0.0f), identity, unit));
        world.ExecuteAddComponentCommands();

        Game::HTransform const authored = world.GetComponent<Game::HTransform>(copied);
        Game::Position const authoredPosition = world.GetComponent<Game::Position>(copied);
        Game::Orientation const authoredOrientation = world.GetComponent<Game::Orientation>(copied);
        Game::Scale const authoredScale = world.GetComponent<Game::Scale>(copied);

        Game::EntityMapping const mapping = world.GetEntityMapping(copied);
        MemDb::TableId const tableId = mapping.table;
        world.DeallocateInstance(copied);
        VERIFY(!hierarchy->IsInHierarchy(copied));

        MemDb::Table& table = world.GetDatabase()->GetTable(tableId);
        MemDb::RowId const row = world.AllocateInstance(copied, tableId);

        const Game::ComponentId transformId = Game::GetComponentId<Game::HTransform>();
        const Game::ComponentId positionId = Game::GetComponentId<Game::Position>();
        const Game::ComponentId orientationId = Game::GetComponentId<Game::Orientation>();
        const Game::ComponentId scaleId = Game::GetComponentId<Game::Scale>();

        Memory::Copy(&authored, table.GetValuePointer(table.GetAttributeIndex(transformId), row), MemDb::AttributeRegistry::TypeSize(transformId));
        Memory::Copy(&authoredPosition, table.GetValuePointer(table.GetAttributeIndex(positionId), row), MemDb::AttributeRegistry::TypeSize(positionId));
        Memory::Copy(&authoredOrientation, table.GetValuePointer(table.GetAttributeIndex(orientationId), row), MemDb::AttributeRegistry::TypeSize(orientationId));
        Memory::Copy(&authoredScale, table.GetValuePointer(table.GetAttributeIndex(scaleId), row), MemDb::AttributeRegistry::TypeSize(scaleId));

        VERIFY(!hierarchy->IsInHierarchy(copied));
        VERIFY(hierarchy->GetInstance(copied) == Game::TransformHierarchy::InvalidInstance);

        world.InitializeInstance(copied, tableId, row);
        VERIFY(hierarchy->IsInHierarchy(copied));
        VERIFY(Near(world.GetComponent<Game::Position>(copied), 103.0f, 0.0f, 0.0f));

        // With the instance in the hierarchy, a write on the parent is a hierarchy write, which is what makes the
        // entity follow a parent that is moved in the editor
        world.SetComponent<Game::Position>(lateParent, Game::Position(120.0f, 0.0f, 0.0f));
        VERIFY(Near(world.GetComponent<Game::Position>(copied), 123.0f, 0.0f, 0.0f));
    }
}

} // namespace Test
