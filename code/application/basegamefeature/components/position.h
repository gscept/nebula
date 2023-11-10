#pragma once
#include "core/types.h"
#include "game/entity.h"
#include "math/vec3.h"
//------------------------------------------------------------------------------
namespace Game
{

struct Position : public Math::vec3
{
    struct Traits;
};

//------------------------------------------------------------------------------
/**
*/
struct Position::Traits
{
    Traits() = delete;
    using type = Position;
    static constexpr auto name = "Position";
    static constexpr auto fully_qualified_name = "Game.Position";
    static constexpr size_t num_fields = 3;
    static constexpr const char* field_names[num_fields] = {
        "x"
        "y"
        "z"
    };
};

} // namespace Game
