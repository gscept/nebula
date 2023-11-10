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
        "x"
        "y"
        "z"
    };
};

} // namespace Game
