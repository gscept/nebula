#pragma once
#ifndef MATH_QUATERNION_H
#define MATH_QUATERNION_H
//------------------------------------------------------------------------------
/**
    @class Math::quaternion

    Nebula's quaternion class.

    (C) 2004 RadonLabs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file

*/

#if __USE_MATH_DIRECTX
#include "math/xnamath/xna_quaternion.h"
#elif __USE_VECMATH
#include "math/vecmath/vec_quaternion.h"
#else
#error "quaternion class not implemented!"
#endif
//-------------------------------------------------------------------
#endif
