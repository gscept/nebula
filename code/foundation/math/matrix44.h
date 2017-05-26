#pragma once
#ifndef MATH_MATRIX44_H
#define MATH_MATRIX44_H
//------------------------------------------------------------------------------
/**
    @file math/matrix44.h

    Frontend header for matrix classes.

    (C) 2006 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#if __USE_MATH_DIRECTX
#include "math/xnamath/xna_matrix44.h"
#elif __USE_VECMATH
#include "math/vecmath/vec_matrix44.h"
#else
#error "matrix44 class not implemented!"
#endif
//-------------------------------------------------------------------
#endif
