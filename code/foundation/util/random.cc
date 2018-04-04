//------------------------------------------------------------------------------
//  random.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "random.h"

namespace Util
{

//------------------------------------------------------------------------------
/**
	XorShift128 implementation.
*/
uint FastRandom()
{
	// These are predefined to give us the largest
	// possible sequence of random numbers
    static uint x = 123456789;
    static uint y = 362436069;
    static uint z = 521288629;
    static uint w = 88675123;
    uint t;
    t = x ^ (x << 11);
    x = y;
	y = z;
	z = w;
    return w = w ^ (w >> 19) ^ (t ^ (t >> 8));
}

} // namespace Util



