#pragma once
#include "core/types.h"
#include "game/entity.h"
#include "math/vec3.h"
//------------------------------------------------------------------------------
namespace Game
{

//------------------------------------------------------------------------------
/**
    @struct Game::Velocity

    @brief A component that stores the linear velocity of an entity.

    @ingroup BaseGameComponents
*/
struct Velocity : public Math::vec3
{
    using Math::vec3::vec3;// default constructor
    Velocity(Math::vec3 const& v)
    {
        this->load(&v.x);
    }

    struct Traits;
};

struct AngularVelocity : public Math::vec3
{
    using Math::vec3::vec3; // default constructor
    AngularVelocity(Math::vec3 const& v)
    {
        this->load(&v.x);
    }

    struct Traits;
};

//------------------------------------------------------------------------------
/**
*/
struct Velocity::Traits
{
    Traits() = delete;
    using type = Velocity;
    static constexpr auto name = "Velocity";
    static constexpr auto fully_qualified_name = "Game.Velocity";
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
    static constexpr const char* field_descriptions[num_fields] = {
        nullptr,
        nullptr,
        nullptr
    };
    using field_types = std::tuple<float, float, float>;
    static constexpr size_t field_byte_offsets[num_fields] = {
        offsetof(Velocity, x),
        offsetof(Velocity, y),
        offsetof(Velocity, z)
    };
    static constexpr bool field_hide_in_inspector[num_fields] = {
        false,
        false,
        false
    };
};

//------------------------------------------------------------------------------
/**
*/
struct AngularVelocity::Traits
{
    Traits() = delete;
    using type = AngularVelocity;
    static constexpr auto name = "AngularVelocity";
    static constexpr auto fully_qualified_name = "Game.AngularVelocity";
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
    static constexpr const char* field_descriptions[num_fields] = {
        nullptr,
        nullptr,
        nullptr
    };
    using field_types = std::tuple<float, float, float>;
    static constexpr size_t field_byte_offsets[num_fields] = {
        offsetof(AngularVelocity, x),
        offsetof(AngularVelocity, y),
        offsetof(AngularVelocity, z)
    };
    static constexpr bool field_hide_in_inspector[num_fields] = {
        false,
        false,
        false
    };
};

} // namespace Game
