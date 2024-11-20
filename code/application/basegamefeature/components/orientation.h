#pragma once
#include "core/types.h"
#include "game/entity.h"
#include "math/quat.h"
//------------------------------------------------------------------------------
namespace Game
{

//------------------------------------------------------------------------------
/**
    @struct Game::Orientation
    
    @brief A component that stores the orientation of an entity.

    @ingroup BaseGameComponents
*/
struct Orientation : public Math::quat
{
    using Math::quat::quat; // default constructor
    Orientation(Math::quat const& q)
    {
        this->load(&q.x);
    }

    struct Traits;
};

//------------------------------------------------------------------------------
/**
*/
struct Orientation::Traits
{
    Traits() = delete;
    using type = Orientation;
    static constexpr auto name = "Orientation";
    static constexpr auto fully_qualified_name = "Game.Orientation";
    static constexpr size_t num_fields = 4;
    static constexpr const char* field_names[num_fields] = {
        "x",
        "y",
        "z",
        "w"
    };
    static constexpr const char* field_typenames[num_fields] = {
        "float",
        "float",
        "float",
        "float"
    };
    static constexpr const char* field_descriptions[num_fields] = {
        nullptr,
        nullptr,
        nullptr,
        nullptr
    };
    using field_types = std::tuple<float, float, float, float>;
    static constexpr size_t field_byte_offsets[num_fields] = {
        offsetof(Orientation, x),
        offsetof(Orientation, y),
        offsetof(Orientation, z),
        offsetof(Orientation, w),
    };
    static constexpr bool field_hide_in_inspector[num_fields] = {
        false,
        false,
        false,
        false
    };
    /// This is the column that the entity orientation will reside in, in every table.
    /// NOTE: This can never be changed, due to assumptions that have been made.
    static constexpr uint32_t fixed_column_index = 2;
};

} // namespace Game
