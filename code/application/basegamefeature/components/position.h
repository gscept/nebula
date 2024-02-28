#pragma once
#include "core/types.h"
#include "game/entity.h"
#include "math/vec3.h"
//------------------------------------------------------------------------------
namespace Game
{

struct Position : public Math::vec3
{
    using Math::vec3::vec3; // default constructor
    Position(Math::vec3 const& v)
    {
        this->load(&v.x);
    }

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
    /// This is the column that the entity position will reside in, in every table.
    /// NOTE: This can never be changed, due to assumptions that have been made.
    static constexpr uint32_t fixed_column_index = 1;
};

} // namespace Game
