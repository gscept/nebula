#pragma once
//------------------------------------------------------------------------------
/**
    @class Math::Transform
    
    Transform based on an __mm256 which stores an quaternion and position.
    Frontend header
    
    (C) 2017-2018 Individual contributors, see AUTHORS file
*/

#if __USE_MATH_DIRECTX
#include "math/xnamath/xna_transform.h"
#elif __USE_VECMATH
#error "matrix44 class not implemented!"
#else
#error "matrix44 class not implemented!"
#endif
//-------------------------------------------------------------------
