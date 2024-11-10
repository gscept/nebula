#pragma once
//------------------------------------------------------------------------------
/**
    @class Util::Colour

    For now just a wrapper around Math::vec4 for type safety

    @copyright
    (C) 2025 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "math/vec4.h"

//------------------------------------------------------------------------------
namespace Util
{
class Colour : public Math::vec4
{
   public:
};
}