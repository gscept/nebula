#pragma once
#include "core/types.h"
#include "game/entity.h"
#include "math/vec3.h"
//------------------------------------------------------------------------------
namespace Game
{

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
};

} // namespace Game
