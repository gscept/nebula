#pragma once
//------------------------------------------------------------------------------
/**
    @class Util::Color

    Represents colors by four single point (32-bit) floating point numbers.

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
    ///
    Color() = default;
    ///
    Color(const Color& c) = default;
    /// Construct from individual single precision floats
    Color(float r, float g, float b, float a);
    /// Copy constructor from vec4
    Color(const Math::vec4& v);
    /// Copy constructor from vec3, alpha is set to 1
    Color(const Math::vec3& v);
    /// converts byte order A|R|G|B to 0..1 floats
    Color(uint32_t argb);
    /// Construct from individual 8 bit channels
    explicit Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    /// Create color from unsigned int (A|R|G|B) -> 0..1 floats.
    static Color RGBA(uint32_t rgba);
    /// Create color from unsigned int (R|G|B|A) -> 0..1 floats.
    static Color ARGB(uint32_t argb);

    // predefined, common colors.

    /// Predefined color. (0xFF0000)
    static const Color red;
    /// Predefined color. (0x00FF00)
    static const Color green;
    /// Predefined color. (0x0000FF)
    static const Color blue;
    /// Predefined color. (0xFFFF00)
    static const Color yellow;
    /// Predefined color. (0xA020F0)
    static const Color purple;
    /// Predefined color. (0xFFA500)
    static const Color orange;
    /// Predefined color. (0x000000)
    static const Color black;
    /// Predefined color. (0xFFFFFF)
    static const Color white;
    /// Predefined color. (0xBEBEBE)
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
__forceinline
Color::Color(uint32_t argb)
{
    const float scale = 1.0f / 255.0f;
    this->vec = _mm_setr_ps(
        scale * ((argb >> 16) & 0xFF), scale * ((argb >> 8) & 0xFF), scale * (argb & 0xFF), scale * ((argb >> 24) & 0xFF)
    );
}
//------------------------------------------------------------------------------
/**
*/
__forceinline
Color::Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    const float scale = 1.0f / 255.0f;
    this->vec = _mm_setr_ps(scale * r, scale * g, scale * b, scale * a);
}

//--------------------------------------------------------------------------
/**
*/
__forceinline Color
Color::RGBA(uint32_t rgba)
{
    return Color((uint8_t)((rgba >> 24) & 0xFF), (uint8_t)((rgba >> 16) & 0xFF), (uint8_t)((rgba >> 8) & 0xFF), (uint8_t)(rgba & 0xFF));
}

//--------------------------------------------------------------------------
/**
*/
__forceinline Color
Color::ARGB(uint32_t argb)
{
    return Color(argb);
}

} // namespace Util