//------------------------------------------------------------------------------
//  @file charactercontext.cc
//  @copyright (C) 2025 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "charactercontext.h"
#include "ids/idgenerationpool.h"
#include "physics/physxstate.h"
#include "physics/utils.h"
#include "physicsinterface.h"
#include "io/jsonreader.h"
#include "io/jsonwriter.h"

namespace Physics
{

Util::Array<Character> controllers;
Ids::IdGenerationPool controllerIdPool;

//--------------------------------------------------------------------------
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

    return characterId;
}

//--------------------------------------------------------------------------
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
    character.controller->release();
    character.controller = nullptr;
    controllerIdPool.Deallocate(id.id);
    return;
}

//--------------------------------------------------------------------------
/**
*/
Character&
CharacterContext::GetCharacter(CharacterId id)
{
    n_assert(controllerIdPool.IsValid(id.id));
    return controllers[Ids::Index(id.id)];
}

//--------------------------------------------------------------------------
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

//--------------------------------------------------------------------------
/**
*/
void
CharacterContext::SetCharacterHeight(CharacterId id, float newHeight)
{
    n_assert(controllerIdPool.IsValid(id.id));
    Character& character = controllers[Ids::Index(id.id)];
    character.controller->resize(newHeight);
}

//--------------------------------------------------------------------------
/**
*/
void
CharacterContext::SetCharacterPosition(CharacterId id, Math::vec3 const& newPosition)
{
    n_assert(controllerIdPool.IsValid(id.id));
    Character& character = controllers[Ids::Index(id.id)];
    character.controller->setPosition(Neb2PxExtentedVec3(newPosition));
}

//--------------------------------------------------------------------------
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

//--------------------------------------------------------------------------
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

//--------------------------------------------------------------------------
/**
*/
CharacterCollision
CharacterContext::MoveCharacter(CharacterId id, Math::vector const& displacement, float minDist, float timeSinceLastMove)
{
    
    n_assert(controllerIdPool.IsValid(id.id));
    Character& character = controllers[Ids::Index(id.id)];
    
    // TODO: setup filters for characters
    static physx::PxControllerFilters defaultFilters;

    auto pxFlags = character.controller->move(Neb2PxVec(displacement), minDist, timeSinceLastMove, defaultFilters);
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