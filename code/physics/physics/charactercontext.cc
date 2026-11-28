//------------------------------------------------------------------------------
//  @file charactercontext.cc
//  @copyright (C) 2025 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "charactercontext.h"
#include "ids/idgenerationpool.h"
#include "physics/actorcontext.h"
#include "physics/physxstate.h"
#include "physics/utils.h"
#include "physicsinterface.h"
#include "io/jsonreader.h"
#include "io/jsonwriter.h"

using namespace physx;

namespace Physics
{

Util::Array<Character> controllers;
Ids::IdGenerationPool controllerIdPool;

namespace
{

//------------------------------------------------------------------------------
/**
*/
class CharacterQueryFilterCallback : public PxQueryFilterCallback
{
public:
    const Character* character;

    PxQueryHitType::Enum preFilter(const PxFilterData& filterData, const PxShape* shape, const PxRigidActor* actor, PxHitFlags& queryFlags) override
    {
        // The controller's shape hangs off its own actor, and a character does not collide with itself
        if (actor != nullptr && actor == character->controller->getActor())
            return PxQueryHitType::eNONE;

        uint32_t const shapeGroup = shape->getQueryFilterData().word0;
        if (shapeGroup == 0 || character->collisionGroup == 0)
            return PxQueryHitType::eBLOCK;

        if ((shapeGroup & character->collisionMask) == 0)
            return PxQueryHitType::eNONE;

        return PxQueryHitType::eBLOCK;
    }

    PxQueryHitType::Enum postFilter(const PxFilterData& filterData, const PxQueryHit& hit, const PxShape* shape, const PxRigidActor* actor) override
    {
        return PxQueryHitType::eBLOCK;
    }
};

CharacterQueryFilterCallback moveFilter;

//------------------------------------------------------------------------------
/**
*/
class CharacterHitReport : public PxUserControllerHitReport
{
public:
    void onShapeHit(const PxControllerShapeHit& hit) override
    {
        Character const& character = CharacterContext::GetCharacter(
            CharacterId((uint32_t)(uintptr_t)hit.controller->getUserData()));

        if (character.reportCallback == nullptr)
            return;

        CharacterHitEvent event;
        event.character = character.id;
        event.actor = Physics::ActorId(Ids::InvalidId32);
        event.shape = Physics::ShapeId(Ids::InvalidId32);

        // A shape this context did not register belongs to something outside the game world
        ShapeId const shapeId((uint32_t)(uintptr_t)hit.shape->userData);
        if (ActorContext::IsValidShape(shapeId))
            event.shape = shapeId;

        ActorId const actorId((uint32_t)(uintptr_t)hit.actor->userData);
        if (ActorContext::IsValid(actorId))
            event.actor = actorId;

        event.position = Math::point(hit.worldPos.x, hit.worldPos.y, hit.worldPos.z);
        event.normal = Px2NebVec(hit.worldNormal);
        event.distance = hit.length;

        character.reportCallback(event);
    }

    void onControllerHit(const PxControllersHit& hit) override {}
    void onObstacleHit(const PxControllerObstacleHit& hit) override {}
};

CharacterHitReport hitReport;

//------------------------------------------------------------------------------
/**
*/
static void
ApplyCharacterFilterData(PxShape* shape, Character const& character)
{
    PxFilterData filter;
    filter.word0 = character.collisionGroup;
    filter.word1 = (uint32_t)CollisionFeedback_Full;
    shape->setSimulationFilterData(filter);
    shape->setQueryFilterData(filter);
}

} // anonymous namespace

//------------------------------------------------------------------------------
/**
*/
CharacterId
CharacterContext::CreateCharacter(Math::vec3 const& position, CharacterCreateInfo const& info, IndexT sceneIndex)
{
    Physics::Scene& scene = GetScene(sceneIndex);

    physx::PxController* controller;
    if (info.type == Physics::ColliderType_Box)
    {
        physx::PxBoxControllerDesc desc;
        desc.position.x = position.x;
        desc.position.y = position.y;
        desc.position.z = position.z;
        desc.contactOffset = info.contactOffset;
        desc.stepOffset = info.stepOffset;
        desc.slopeLimit = info.slopeLimit;
        desc.nonWalkableMode = info.slideOnSlopes ? physx::PxControllerNonWalkableMode::ePREVENT_CLIMBING_AND_FORCE_SLIDING : physx::PxControllerNonWalkableMode::ePREVENT_CLIMBING;

        if (info.materialId != InvalidIndex)
        {
            desc.material = GetMaterial(info.materialId).material;
        }
        else
        {
            desc.material = Physics::state.FallbackMaterial.material;
        }

        desc.halfForwardExtent = info.box.halfExtents.z;
        desc.halfSideExtent = info.box.halfExtents.x;
        desc.halfHeight = info.box.halfExtents.y;

        desc.reportCallback = &hitReport;
        controller = scene.controllerManager->createController(desc);
    }
    else if (info.type == Physics::ColliderType_Capsule)
    {
        physx::PxCapsuleControllerDesc desc;
        desc.position.x = position.x;
        desc.position.y = position.y;
        desc.position.z = position.z;
        desc.contactOffset = info.contactOffset;
        desc.stepOffset = info.stepOffset;
        desc.slopeLimit = info.slopeLimit;
        desc.nonWalkableMode = info.slideOnSlopes ? physx::PxControllerNonWalkableMode::ePREVENT_CLIMBING_AND_FORCE_SLIDING : physx::PxControllerNonWalkableMode::ePREVENT_CLIMBING;

        if (info.materialId != InvalidIndex)
        {
            desc.material = GetMaterial(info.materialId).material;
        }
        else
        {
            desc.material = Physics::state.FallbackMaterial.material;
        }

        desc.height = info.capsule.height;
        desc.radius = info.capsule.radius;
        desc.climbingMode = info.capsule.constrainClimbing ? physx::PxCapsuleClimbingMode::eCONSTRAINED : physx::PxCapsuleClimbingMode::eEASY;

        desc.reportCallback = &hitReport;
        controller = scene.controllerManager->createController(desc);
    }
    else
    {
#if NEBULA_DEBUG
        n_error("Character type is not Box or Capsule! Nebula currently only support these two types.");
#else
        n_assert(info.type == Physics::ColliderType_Box || info.type == Physics::ColliderType_Capsule);
#endif
        return CharacterId(0xFFFFFFFF);
    }

    CharacterId characterId;
    bool newIndex = controllerIdPool.Allocate(characterId.id);

    Character character;
    character.controller = controller;
    character.type = info.type;
    character.id = characterId;
    character.collisionGroup = info.collisionGroup;
    character.collisionMask = info.collisionMask;
    character.reportCallback = info.reportCallback;
    character.userData = info.userData;
#if NEBULA_DEBUG
    character.debugName = "Character";
#endif

    if (newIndex)
    {
        controllers.Append(std::move(character));
    }
    else
    {
        controllers[Ids::Index(characterId.id)] = std::move(character);
    }

    Character& record = controllers[Ids::Index(characterId.id)];

    // The hit report resolves the record from the controller, and the controller is only known by the
    // id this pool gave it
    controller->setUserData((void*)(uintptr_t)characterId.id);

    PxRigidActor* pxActor = controller->getActor();
    record.actor = ActorContext::RegisterActor(pxActor, info.userData);

    PxShape* pxShapes[1];
    if (pxActor->getShapes(pxShapes, 1) > 0)
    {
        record.shape = ActorContext::RegisterShape(pxShapes[0], record.actor, info.userData);
        ApplyCharacterFilterData(pxShapes[0], record);
    }

    return characterId;
}

//------------------------------------------------------------------------------
/**
*/
void
CharacterContext::DestroyCharacter(CharacterId id)
{
    if (!controllerIdPool.IsValid(id.id))
    {
        return;
    }

    Character& character = controllers[Ids::Index(id.id)];

    if (ActorContext::IsValid(character.actor))
    {
        ActorContext::UnregisterActor(character.actor);
    }

    character.controller->release();
    character.controller = nullptr;
    character.actor = Physics::ActorId(Ids::InvalidId32);
    character.shape = Physics::ShapeId(Ids::InvalidId32);
    controllerIdPool.Deallocate(id.id);
    return;
}

//------------------------------------------------------------------------------
/**
*/
bool
CharacterContext::IsValid(CharacterId id)
{
    return id.id != Ids::InvalidId32 && controllerIdPool.IsValid(id.id);
}

//------------------------------------------------------------------------------
/**
*/
Character&
CharacterContext::GetCharacter(CharacterId id)
{
    n_assert(controllerIdPool.IsValid(id.id));
    return controllers[Ids::Index(id.id)];
}

//------------------------------------------------------------------------------
/**
*/
void
CharacterContext::SetCharacterBoxSize(CharacterId id, Math::vec3 const& halfExtents)
{
    n_assert(controllerIdPool.IsValid(id.id));

    Character& character = controllers[Ids::Index(id.id)];

    n_assert(character.type == ColliderType::ColliderType_Box);

    physx::PxBoxController* boxController = (physx::PxBoxController*)character.controller;
    boxController->setHalfSideExtent(halfExtents.x);
    boxController->setHalfHeight(halfExtents.y);
    boxController->setHalfForwardExtent(halfExtents.z);
}

//------------------------------------------------------------------------------
/**
*/
void
CharacterContext::SetCharacterHeight(CharacterId id, float newHeight)
{
    n_assert(controllerIdPool.IsValid(id.id));
    Character& character = controllers[Ids::Index(id.id)];
    character.controller->resize(newHeight);
}

//------------------------------------------------------------------------------
/**
*/
void
CharacterContext::SetCharacterPosition(CharacterId id, Math::vec3 const& newPosition)
{
    n_assert(controllerIdPool.IsValid(id.id));
    Character& character = controllers[Ids::Index(id.id)];
    character.controller->setPosition(Neb2PxExtentedVec3(newPosition));
}

//------------------------------------------------------------------------------
/**
*/
Math::vec3
CharacterContext::GetCharacterCenterPosition(CharacterId id)
{
    n_assert(controllerIdPool.IsValid(id.id));
    Character& character = controllers[Ids::Index(id.id)];
    auto pos = character.controller->getPosition();
    return Math::vec3(pos.x, pos.y, pos.z);
}

//------------------------------------------------------------------------------
/**
*/
Math::vec3
CharacterContext::GetCharacterFootPosition(CharacterId id)
{
    n_assert(controllerIdPool.IsValid(id.id));
    Character& character = controllers[Ids::Index(id.id)];
    auto pos = character.controller->getFootPosition();
    return Math::vec3(pos.x, pos.y, pos.z);
}

//------------------------------------------------------------------------------
/**
*/
Physics::ActorId
CharacterContext::GetCharacterActor(CharacterId id)
{
    n_assert(controllerIdPool.IsValid(id.id));
    return controllers[Ids::Index(id.id)].actor;
}

//------------------------------------------------------------------------------
/**
*/
Physics::ShapeId
CharacterContext::GetCharacterShape(CharacterId id)
{
    n_assert(controllerIdPool.IsValid(id.id));
    return controllers[Ids::Index(id.id)].shape;
}

//------------------------------------------------------------------------------
/**
*/
void
CharacterContext::SetCharacterFilter(CharacterId id, uint32_t group, uint32_t mask)
{
    n_assert(controllerIdPool.IsValid(id.id));

    Character& character = controllers[Ids::Index(id.id)];
    character.collisionGroup = group;
    character.collisionMask = mask;

    if (ActorContext::IsValidShape(character.shape))
    {
        ApplyCharacterFilterData(ActorContext::GetShape(character.shape).shape, character);
    }
}

//------------------------------------------------------------------------------
/**
*/
CharacterCollision
CharacterContext::MoveCharacter(CharacterId id, Math::vector const& displacement, float minDist, float timeSinceLastMove)
{

    n_assert(controllerIdPool.IsValid(id.id));
    Character& character = controllers[Ids::Index(id.id)];

    moveFilter.character = &character;

    PxControllerFilters filters(nullptr, &moveFilter);

    auto pxFlags = character.controller->move(Neb2PxVec(displacement), minDist, timeSinceLastMove, filters);
    CharacterCollision flags;
    flags.SetBitIf(CharacterCollisionBits::Sides, pxFlags.isSet(physx::PxControllerCollisionFlag::eCOLLISION_SIDES));
    flags.SetBitIf(CharacterCollisionBits::Up,    pxFlags.isSet(physx::PxControllerCollisionFlag::eCOLLISION_UP));
    flags.SetBitIf(CharacterCollisionBits::Down,  pxFlags.isSet(physx::PxControllerCollisionFlag::eCOLLISION_DOWN));

    return flags;
}

} // namespace Physics

template <>
void
IO::JsonReader::Get<Physics::CharacterId>(Physics::CharacterId& ret, char const* attr)
{
    // read nothing
    ret = Physics::CharacterId(0xFFFFFFFF);
}

template <>
void
IO::JsonWriter::Add<Physics::CharacterId>(Physics::CharacterId const& id, Util::String const& val)
{
    // Write nothing
    return;
}
