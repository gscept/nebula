#pragma once
#ifndef MATH_POINT_H
#define MATH_POINT_H
//------------------------------------------------------------------------------
/**
    @class Math::point
    
    A point in homogeneous space. A point describes a position in space,
    and has its W component set to 1.0.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#if __USE_MATH_DIRECTX
#include "math/xnamath/xna_point.h"
#elif __USE_VECMATH
#include "math/vecmath/vec_point.h"
#else
#error "point class not implemented!"
#endif

//------------------------------------------------------------------------------
#endif
    