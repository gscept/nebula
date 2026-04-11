//------------------------------------------------------------------------------
//  charactermanager.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "charactermanager.h"
#include "basegamefeature/managers/timemanager.h"
#include "game/gameserver.h"
#include "physics/charactercontext.h"
#include "physicsfeature/physicsfeatureunit.h"
#include "physicsinterface.h"
#include "physics/actorcontext.h"
#include "resources/resourceserver.h"
#include "components/physicsfeature.h"
#include "basegamefeature/components/basegamefeature.h"
#include "basegamefeature/components/position.h"
#include "basegamefeature/components/orientation.h"
#include "basegamefeature/components/scale.h"
#include "basegamefeature/components/velocity.h"

namespace PhysicsFeature
{

__ImplementClass(PhysicsFeature::CharacterManager, 'ChMa', Game::Manager);
__ImplementSingleton(CharacterManager)

// physics timesource
Game::TimeSource* time;

//------------------------------------------------------------------------------
/**
*/
CharacterManager::CharacterManager()
{
    __ConstructSingleton
}

//------------------------------------------------------------------------------
/**
*/
CharacterManager::~CharacterManager()
{
    __DestructSingleton
}

//--------------------------------------------------------------------------
/**
*/
void
MoveCharacters(Game::World* world, Game::Position& position, Game::Velocity& velocity, Character& character)
{
    character.displacement = velocity * time->frameTime;
    Physics::CharacterCollision collision = Physics::CharacterContext::MoveCharacter(character.characterId, character.displacement, 0.00001f, time->frameTime);
    if (collision.IsSet(Physics::CharacterCollisionBits::Down))
    {
        velocity.y = 0;
        character.isGrounded = true;
    }
    else
    {
        character.isGrounded = false;
    }

    // Stop vertical velocity if the character bonks their head
    if (collision.IsSet(Physics::CharacterCollisionBits::Up) && velocity.y > 0)
    {
        velocity.y = 0;
    }

    Math::vec3 previousPosition = position;

    Physics::Character& c = Physics::CharacterContext::GetCharacter(character.characterId);
    auto p = c.controller->getFootPosition();
    position.x = p.x;
    position.y = p.y;
    position.z = p.z;

    // Adjust velocity to account for the actual moved direction.
    Math::vec3 travelled = position - previousPosition;
    character.displacement = travelled;
    travelled.y = 0;
    travelled = Math::normalize(travelled);
    Math::vec3 horizontalVelocity = velocity;
    horizontalVelocity.y = 0;
    float d = Math::dot(horizontalVelocity, travelled);
    float yVel = velocity.y;
    velocity = travelled * d;
    velocity.y = yVel;
}

//------------------------------------------------------------------------------
/**
*/
void
CharacterManager::OnActivate()
{
    Game::Manager::OnActivate();

    time = Game::Time::GetTimeSource(TIMESOURCE_PHYSICS);
    Game::World* world = Game::GetWorld(WORLD_DEFAULT);

    Game::ProcessorBuilder(world, "CharacterManager.MoveCharacters").Func(MoveCharacters).Excluding<Game::Static, PhysicsFeature::IsKinematic>().Build();
}

//------------------------------------------------------------------------------
/**
*/
void
CharacterManager::OnDeactivate()
{
    Game::Manager::OnDeactivate();
}

//------------------------------------------------------------------------------
/**
*/
void
CharacterManager::OnDecay()
{
    Game::World* world = Game::GetWorld(WORLD_DEFAULT);
    Game::ComponentDecayBuffer const decayBuffer = world->GetDecayBuffer(Game::GetComponentId<Character>());
    PhysicsFeature::Character* data = (PhysicsFeature::Character*)decayBuffer.buffer;
    for (int i = 0; i < decayBuffer.size; i++)
    {
        Physics::CharacterContext::DestroyCharacter(data[i].characterId);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
CharacterManager::OnCleanup(Game::World* world)
{
    n_assert(CharacterManager::HasInstance());

    Game::FilterBuilder::FilterCreateInfo filterInfo;
    filterInfo.inclusive[0] = Game::GetComponentId<Character>();
    filterInfo.access[0] = Game::AccessMode::WRITE;
    filterInfo.numInclusive = 1;

    Game::Filter filter = Game::FilterBuilder::CreateFilter(filterInfo);
    Game::Dataset data = world->Query(filter);
    for (int v = 0; v < data.numViews; v++)
    {
        Game::Dataset::View const& view = data.views[v];
        Physics::Character* const characters = (Physics::Character*)view.buffers[0];

        for (IndexT i = 0; i < view.numInstances; ++i)
        {
            Physics::Character const& character = characters[i];
            Physics::CharacterContext::DestroyCharacter(character.id);
        }
    }

    Game::DestroyFilter(filter);
}

} // namespace PhysicsFeature
