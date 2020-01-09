#pragma once
#ifndef MATH_VECTOR_H
#define MATH_VECTOR_H
//------------------------------------------------------------------------------
/**
    @class Math::vector
    
    A vector in homogenous space. A point describes a direction and length
    in 3d space and always has a w component of 0.0.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/

#if __USE_MATH_DIRECTX
#include "math/xnamath/xna_vector.h"
#elif __USE_VECMATH
#include "math/vecmath/vec_vector.h"
#else
#error "vector class not implemented!"
#endif

//------------------------------------------------------------------------------
#endif
