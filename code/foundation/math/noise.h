#pragma once
#ifndef MATH_NOISE_H
#define MATH_NOISE_H
//------------------------------------------------------------------------------
/**
    @class Math::noise
    
    Perlin noise class.
    
    See http://mrl.nyu.edu/~perlin/noise/ for details.

    (C) 2006 RadonLabs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/types.h"

//------------------------------------------------------------------------------
namespace Math
{
class noise
{
public:
    /// generate noise value
    static float gen(float x, float y, float z);

private:
    /// compute fade curve
    static float fade(float t);
    /// lerp between a and b
    static float lerp(float t, float a, float b);
    /// convert into gradient direction
    static float grad(int hash, float x, float y, float z);

    static int perm[512];
};

int noise::perm[512] =  {60,137,91,90,15,
   131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
   190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
   88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,
   77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
   102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,
   135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,
   5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
   223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167, 43,172,9,
   129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
   251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
   49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
   138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
   };

//------------------------------------------------------------------------------
/**
*/
inline
float
noise::fade(float t)
{
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

//------------------------------------------------------------------------------
/**
*/
inline
float
noise::lerp(float t, float a, float b)
{
    return a + t * (b - a);
}

//------------------------------------------------------------------------------
/**
*/
inline
float
noise::grad(int hash, float x, float y, float z)
{
    int h = hash & 15;
    float u = h < 8 ? x : y;
    float v = h < 4 ? y : ((h == 12) || (h==14)) ? x : z;
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

//------------------------------------------------------------------------------
/**
*/
inline
float
noise::gen(float x, float y, float z)
{
    float floorX = floorf(x);
    float floorY = floorf(y);
    float floorZ = floorf(z);

    // find unit cube that contains point
    int X = int(floorX) & 255;
    int Y = int(floorY) & 255;
    int Z = int(floorZ) & 255;

    // find relative x,y,z of point in cube
    x -= floorX;
    y -= floorY;
    z -= floorZ;

    // compute fade curves for x, y, z
    float u = fade(x);
    float v = fade(y);
    float w = fade(z);

    // hash coords of 8 cube corners
    int A  = perm[X] + Y;
    int AA = perm[A] + Z;
    int AB = perm[A+1] + Z;
    int B  = perm[X+1] + Y;
    int BA = perm[B] + Z;
    int BB = perm[B+1] + Z;

    // add blended results from 8 corners of cube
    return lerp(w, lerp(v, lerp(u, grad(perm[AA  ], x  , y  , z   ),
                                   grad(perm[BA  ], x-1, y  , z   )),
                           lerp(u, grad(perm[AB  ], x  , y-1, z   ),
                                   grad(perm[BB  ], x-1, y-1, z   ))),
                   lerp(v, lerp(u, grad(perm[AA+1], x  , y  , z-1 ),
                                   grad(perm[BA+1], x-1, y  , z-1 )),
                           lerp(u, grad(perm[AB+1], x  , y-1, z-1 ),
                                   grad(perm[BB+1], x-1, y-1, z-1 ))));
}

} // namespace Math
//------------------------------------------------------------------------------
#endif