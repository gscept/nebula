#pragma once
//------------------------------------------------------------------------------
/**
    @class Util::Color

    For now just a wrapper around Math::vec4 for type safety

    @copyright
    (C) 2025 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "math/vec4.h"

//------------------------------------------------------------------------------
namespace Util
{
class Color : public Math::vec4
{
public:
    Color() = default;
    Color(const Color& c) = default;
    ///
    Color(float r, float g, float b, float a);
    /// copy constructor from vec4
    Color(const Math::vec4& v);
    /// copy constructor from vec3, alpha is set to 1
    Color(const Math::vec3& v);
    /// converts byte order A|R|G|B to 0..1 floats
    Color(uint32_t argb);
    ///
    Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a);

    // predefined, common colors. Note: These are X11 color names.
    
    static const Color red;
    static const Color green;
    static const Color blue;
    static const Color yellow;
    static const Color purple;
    static const Color orange;
    static const Color black;
    static const Color white;
    static const Color gray;
};

//------------------------------------------------------------------------------
/**
*/
__forceinline
Color::Color(float r, float g, float b, float a)
    : Math::vec4(r, g, b, a)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
Color::Color(const Math::vec4& v)
    : Math::vec4(v)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
Color::Color(const Math::vec3& v)
    : Math::vec4(v, 1.0f)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
__forceinline Color::Color(uint32_t argb)
{
    const float scale = 1.0f / 255.0f;
    this->vec = _mm_setr_ps(
        scale * ((argb >> 16) & 0xFF), scale * ((argb >> 8) & 0xFF), scale * (argb & 0xFF), scale * ((argb >> 24) & 0xFF)
    );
}
//------------------------------------------------------------------------------
/**
*/
__forceinline Color::Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    const float scale = 1.0f / 255.0f;
    this->vec = _mm_setr_ps(scale * r, scale * g, scale * b, scale * a);
}
} // namespace Util