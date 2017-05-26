#pragma once
#ifndef MATH_PLANE_H
#define MATH_PLANE_H
//------------------------------------------------------------------------------
/**
    @class Math::plane

    Nebula's plane class.

    (C) 2007 RadonLabs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file

*/

#if __USE_MATH_DIRECTX
#include "math/xnamath/xna_plane.h"
#elif __USE_VECMATH
#include "math/vecmath/vec_plane.h"
#else
#error "plane class not implemented!"
#endif
//------------------------------------------------------------------------------
#endif
