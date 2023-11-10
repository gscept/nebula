#pragma once
#include "core/types.h"
#include "game/entity.h"
#include "math/vec3.h"
//------------------------------------------------------------------------------
namespace Game
{

struct Scale : public Math::vec3
{
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
