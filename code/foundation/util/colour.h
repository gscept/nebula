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
       Colour() = default;
       Colour(const Colour& c) = default;
       /// 
       Colour(float r, float g, float b, float a);
       /// copy constructor from vec4
       Colour(const Math::vec4& v);
       /// copy constructor from vec3, alpha is set to 1
       Colour(const Math::vec3& v);
       /// converts byte order A|R|G|B to 0..1 floats
       Colour(uint32_t argb);
       /// 
       Colour(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
};

//------------------------------------------------------------------------------
/**
*/
__forceinline
Colour::Colour(float r, float g, float b, float a) : Math::vec4(r,g,b,a)
{
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
Colour::Colour(const Math::vec4& v) : Math::vec4(v)
{
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
Colour::Colour(const Math::vec3& v) : Math::vec4(v,1.0f)
{
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
Colour::Colour(uint32_t argb)
{
    const float scale = 1.0f / 255.0f;
    this->vec = _mm_setr_ps(scale * ((argb >> 16) & 0xFF), scale * ((argb >> 8) & 0xFF), scale * (argb & 0xFF), scale * ((argb >> 24) & 0xFF));
}
//------------------------------------------------------------------------------
/**
*/
__forceinline
Colour::Colour(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    const float scale = 1.0f / 255.0f;
    this->vec = _mm_setr_ps(scale * r, scale * g, scale * b, scale * a);
}
}