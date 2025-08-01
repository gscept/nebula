#pragma once
//------------------------------------------------------------------------------
/**
    @file  charactercontext.h

    @copyright
    (C) 2025 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "physicsinterface.h"
#include "util/bitfield.h"

namespace Physics
{

class CharacterContext
{
public:
    ///
    static CharacterId CreateCharacter(Math::vec3 const& position, CharacterCreateInfo const& info, IndexT scene = 0);
    ///
    static void DestroyCharacter(CharacterId characterId);
    ///
    static Character& GetCharacter(CharacterId id);

    /// Sets the characters half extents. Character needs to be of box type.
    static void SetCharacterBoxSize(CharacterId characterId, Math::vec3 const& halfExtents);
    
    /// Sets the characters height, and adjusts the position so that it doesn't "pop" into the air.
    static void SetCharacterHeight(CharacterId characterId, float newHeight);

    /// Immediately changes the character world position without doing any collision checks.
    static void SetCharacterPosition(CharacterId characterId, Math::vec3 const& newPosition);

    /// Get the character center point. @see GetCharacterFootPosition for getting lowest point of the collider
    static Math::vec3 GetCharacterCenterPosition(CharacterId characterId);

    /// Get the position of the lowest point of the characters collider in worldspace.
    static Math::vec3 GetCharacterFootPosition(CharacterId characterId);


    /// Displace a character, and check/resolve collisions along the way.
    /// @param minDist The minimum travelled distance to consider. If travelled distance is smaller, the character doesn’t move.
    /// @param timeSinceLastMove is the amount of time that passed since the last call to the move function.
    static CharacterCollision MoveCharacter(CharacterId characterId, Math::vector const& displacement, float minDist, float timeSinceLastMove);

    
};

} // namespace Physics