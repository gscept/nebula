#pragma once
//------------------------------------------------------------------------------
/**
    @class Math::float4

    A 4-component float vector class. This is the basis class for points
    and vectors.
    
    (C) 2007 RadonLabs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/

#if __USE_XNA
#include "math/xnamath/xna_float4.h"
#elif __USE_VECMATH
#include "math/vecmath/vec_float4.h"
#else
#error "float4 class not implemented!"
#endif

//------------------------------------------------------------------------------