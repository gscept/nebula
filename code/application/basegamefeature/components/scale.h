#pragma once
#include "core/types.h"
#include "game/entity.h"
#include "math/vec3.h"
//------------------------------------------------------------------------------
namespace Game
{

struct Scale : public Math::vec3
{
    Scale()
    {
        this->set(1.0f,1.0f,1.0f);
    }

    Scale(Math::vec3 const& v)
    {
        this->load(&v.x);
    }

    struct Traits;
};

//------------------------------------------------------------------------------
/**
*/
struct Scale::Traits
{
    Traits() = delete;
    using type = Scale;
    static constexpr auto name = "Scale";
    static constexpr auto fully_qualified_name = "Game.Scale";
    static constexpr size_t num_fields = 3;
    static constexpr const char* field_names[num_fields] = {
        "x",
        "y",
        "z"
    };
    static constexpr const char* field_typenames[num_fields] = {
        "float",
        "float",
        "float"
    };
    using field_types = std::tuple<float, float, float>;
    static constexpr size_t field_byte_offsets[num_fields] = {
        offsetof(Scale, x),
        offsetof(Scale, y),
        offsetof(Scale, z)
    };
    /// This is the column that the entity scale will reside in, in every table.
    /// NOTE: This can never be changed, due to assumptions that have been made.
    static constexpr uint32_t fixed_column_index = 3;
};

} // namespace Game
