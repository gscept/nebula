#pragma once
//------------------------------------------------------------------------------
/**
    Half precision (16 bit) float implementation

    Based on https://github.com/acgessler/half_float

    @copyright
    (C) 2022 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include <limits>
#include "core/types.h"
namespace Math  
{



class half 
{

public:

    /// Constructor
    inline half();
    /// Construct from other half
    inline half(const half& h);
    /// Construct from float
    inline half(const float f);

    /// Convert to float
    inline operator float() const;

    /// Compare equals
    inline bool operator==(const half rhs) const;
    /// Compare equals
    inline bool operator==(const float rhs) const;
    /// Compare not equals
    inline bool operator!=(const half rhs) const;
    /// Compare not equals
    inline bool operator!=(const float rhs) const;
    /// Compare less
    inline bool operator<(const half rhs) const;
    /// Compare less
    inline bool operator<(const float rhs) const;
    /// Compare less equals
    inline bool operator<=(const half rhs) const;
    /// Compare less equals
    inline bool operator<=(const float rhs) const;
    /// Compare greater
    inline bool operator>(const half rhs) const;
    /// Compare greater
    inline bool operator>(const float rhs) const;
    /// Compare greater equals
    inline bool operator>=(const half rhs) const;
    /// Compare greater equals
    inline bool operator>=(const float rhs) const;

    inline half& operator+=(half other);
    inline half& operator-=(half other);
    inline half& operator*=(half other);
    inline half& operator/=(half other);

    inline half operator-() const;

private:

    union Float
    {
        float f;
        struct
        {
            uint32_t frac : 23;
            uint32_t exp : 8;
            uint32_t sign : 1;
        } ieee;
    };

    friend class std::numeric_limits<Math::half>;
    static const uint8_t BITS_MANTISSA = 10;
    static const uint8_t BITS_EXPONENT = 5;
    static const uint8_t MAX_EXPONENT_VALUE = 31;
    static const uint8_t BIAS = MAX_EXPONENT_VALUE / 2;
    static const uint8_t MAX_EXPONENT = BIAS;
    static const uint8_t MIN_EXPONENT = -BIAS;
    static const uint8_t MAX_EXPONENT10 = 9;
    static const uint8_t MIN_EXPONENT10 = -9;

    union
    {
        uint16_t bits;
        struct
        {
            uint16_t frac : 10;
            uint16_t exp : 5;
            uint16_t sign : 1;
        } ieee;
    };
};

//------------------------------------------------------------------------------
/**
*/
inline half 
operator+ (half one, half two)
{
    return half((float)one + (float)two);
}

//------------------------------------------------------------------------------
/**
*/
inline half 
operator- (half one, half two)
{
    return half(one + (-two));
}

//------------------------------------------------------------------------------
/**
*/
inline half 
operator* (half one, half two)
{
    return half((float)one * (float)two);
}

//------------------------------------------------------------------------------
/**
*/
inline half 
operator/ (half one, half two)
{
    return half((float)one / (float)two);
}

//------------------------------------------------------------------------------
/**
*/
inline half
operator+ (half one, float two)
{
    return half((float)one + two);
}

//------------------------------------------------------------------------------
/**
*/
inline half
operator- (half one, float two)
{
    return half(one + (-two));
}

//------------------------------------------------------------------------------
/**
*/
inline half
operator* (half one, float two)
{
    return half((float)one * two);
}

//------------------------------------------------------------------------------
/**
*/
inline half
operator/ (half one, float two)
{
    return half((float)one / two);
}

//------------------------------------------------------------------------------
/**
*/
inline float
operator+ (float one, half two)
{
    return one + (float)two;
}

//------------------------------------------------------------------------------
/**
*/
inline float
operator- (float one, half two)
{
    return one - (float)two;
}

//------------------------------------------------------------------------------
/**
*/
inline float
operator* (float one, half two)
{
    return one * (float)two;
}

//------------------------------------------------------------------------------
/**
*/
inline float
operator/ (float one, half two)
{
    return one / (float)two;
}

//------------------------------------------------------------------------------
/**
*/
inline 
Math::half::half()
{
    this->bits = 0x0;
}

//------------------------------------------------------------------------------
/**
*/
inline 
half::half(const half& h)
{
    this->bits = h.bits;
}

//------------------------------------------------------------------------------
/**
*/
inline 
half::half(const float f)
{
    Float fl;
    fl.f = f;

    this->ieee.sign = fl.ieee.sign;

    if (!fl.ieee.exp)
    {
        this->ieee.frac = 0;
        this->ieee.exp = 0;
    }
    else if (fl.ieee.exp == 0xFF)
    {
        this->ieee.frac = (fl.ieee.frac != 0) ? 1 : 0;
        this->ieee.exp = 31;
    }
    else
    {
        int exp = fl.ieee.exp - 127;
        if (exp < -24)
        {
            this->ieee.frac = 0;
            this->ieee.exp = 0;
        }
        else if (exp < -14)
        {
            this->ieee.exp = 0;
            uint expVal = (uint)(-14 - exp);
            switch (expVal)
            {
                case 0:
                    this->ieee.frac = 0;
                    break;
                case 1: this->ieee.frac = 512 + (fl.ieee.frac >> 14); break;
                case 2: this->ieee.frac = 256 + (fl.ieee.frac >> 15); break;
                case 3: this->ieee.frac = 128 + (fl.ieee.frac >> 16); break;
                case 4: this->ieee.frac = 64 + (fl.ieee.frac >> 17); break;
                case 5: this->ieee.frac = 32 + (fl.ieee.frac >> 18); break;
                case 6: this->ieee.frac = 16 + (fl.ieee.frac >> 19); break;
                case 7: this->ieee.frac = 8 + (fl.ieee.frac >> 20); break;
                case 8: this->ieee.frac = 4 + (fl.ieee.frac >> 21); break;
                case 9: this->ieee.frac = 2 + (fl.ieee.frac >> 22); break;
                case 10: this->ieee.frac = 1; break;
            }
        }
        else if (exp > 15)
        {
            this->ieee.frac = 0;
            this->ieee.exp = 31;
        }
        else
        {
            this->ieee.frac = (fl.ieee.frac >> 13);
            this->ieee.exp = exp + 15;
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
inline half::operator float() const
{
    Float f;
    f.ieee.sign = this->ieee.sign;

    if (!this->ieee.exp)
    {
        if (!this->ieee.frac)
        {
            f.ieee.frac = 0;
            f.ieee.exp = 0;
        }
        else
        {
            const float half_denorm = (1.0f / 16384.0f);
            float m = ((float)this->ieee.frac) / 1024.0f;
            float s = this->ieee.sign ? -1.0f : 1.0f;
            f.f = s * m * half_denorm;
        }
    }
    else if (this->ieee.exp == 31)
    {
        f.ieee.exp = 0xff;
        f.ieee.frac = this->ieee.frac != 0 ? 1 : 0;
    }
    else
    {
        f.ieee.exp = this->ieee.exp + 112;
        f.ieee.frac = this->ieee.frac << 13;
    }
    return f.f;
}

//------------------------------------------------------------------------------
/**
*/
inline bool 
half::operator==(const half rhs) const
{
    return this->bits == rhs.bits;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
half::operator==(const float rhs) const
{
    return (float)(*this) == rhs;
}

//------------------------------------------------------------------------------
/**
*/
inline bool 
half::operator!=(const half rhs) const
{
    return this->bits != rhs.bits;
}

//------------------------------------------------------------------------------
/**
*/
inline bool 
half::operator!=(const float rhs) const
{
    return (float)(*this) != rhs;
}

//------------------------------------------------------------------------------
/**
*/
inline bool 
half::operator<(const half rhs) const
{
    return (int16_t)this->bits < (int16_t)rhs.bits;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
half::operator<(const float rhs) const
{
    return (float)(*this) < rhs;
}

//------------------------------------------------------------------------------
/**
*/
inline bool 
half::operator<=(const half rhs) const
{
    return !(*this > rhs);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
half::operator<=(const float rhs) const
{
    return !(*this > rhs);
}

//------------------------------------------------------------------------------
/**
*/
inline bool 
half::operator>(const half rhs) const
{
    return (int16_t)this->bits > (int16_t)rhs.bits;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
half::operator>(const float rhs) const
{
    return (float)(*this) > rhs;
}

//------------------------------------------------------------------------------
/**
*/
inline bool 
half::operator>=(const half rhs) const
{
    return !(*this < rhs);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
half::operator>=(const float rhs) const
{
    return !(*this < rhs);
}

//------------------------------------------------------------------------------
/**
*/
inline half& 
half::operator+=(half other)
{
    *this = (*this) + other;
    return *this;
}

//------------------------------------------------------------------------------
/**
*/
inline half& 
half::operator-=(half other)
{
    *this = (*this) - other;
    return *this;
}

//------------------------------------------------------------------------------
/**
*/
inline half& 
half::operator*=(half other)
{
    *this = (float)(*this) * (float)other;
    return *this;
}

//------------------------------------------------------------------------------
/**
*/
inline half& 
half::operator/=(half other)
{
    *this = (float)(*this) / (float)other;
    return *this;
}

//------------------------------------------------------------------------------
/**
*/
inline half 
half::operator-() const
{
    half ret = *this;
    ret.ieee.sign = ~ret.ieee.sign;
    return ret;
}

static_assert(sizeof(half) == 2);


} // namespace Math

namespace std
{
template <>
class numeric_limits<Math::half>
{
public:

    // General -- meaningful for all specializations.
    static const bool is_specialized = true;
    static Math::half min()
    {
        Math::half ret;
        ret.ieee.frac = 0;
        ret.ieee.exp = 1;
        ret.ieee.sign = 0;
        return ret;
    }
    static Math::half max()
    {
        Math::half ret;
        ret.ieee.frac = ~0;
        ret.ieee.exp = Math::half::MAX_EXPONENT_VALUE - 1;
        ret.ieee.sign = 0;
        return ret;
    }
    static const int radix = 2;
    static const int digits = 10;   // conservative assumption
    static const int digits10 = 2;  // conservative assumption
    static const bool is_signed = true;
    static const bool is_integer = true;
    static const bool is_exact = false;
    static const bool traps = false;
    static const bool is_modulo = false;
    static const bool is_bounded = true;

    // Floating point specific.

    static Math::half epsilon()
    {
        return Math::half(0.00097656f);
    } // from OpenEXR, needs to be confirmed
    static Math::half round_error()
    {
        return Math::half(0.00097656f / 2);
    }
    static const int min_exponent10 = Math::half::MIN_EXPONENT10;
    static const int max_exponent10 = Math::half::MAX_EXPONENT10;
    static const int min_exponent = Math::half::MIN_EXPONENT;
    static const int max_exponent = Math::half::MAX_EXPONENT;

    static const bool has_infinity = true;
    static const bool has_quiet_NaN = true;
    static const bool has_signaling_NaN = true;
    static const bool is_iec559 = false;
    static const bool has_denorm = denorm_present;
    static const bool tinyness_before = false;
    static const float_round_style round_style = round_to_nearest;

    static Math::half denorm_min ()
    {
        Math::half ret;
        ret.ieee.frac = 1;
        ret.ieee.exp = 0;
        ret.ieee.sign = 1;
        return ret;
    }
    static Math::half infinity()
    {
        Math::half ret;
        ret.ieee.frac = 0;
        ret.ieee.exp = Math::half::MAX_EXPONENT_VALUE;
        ret.ieee.sign = 0;
        return ret;
    }
    static Math::half quiet_NaN()
    {
        Math::half ret;
        ret.ieee.frac = 1;
        ret.ieee.exp = Math::half::MAX_EXPONENT_VALUE;
        ret.ieee.sign = 0;
        return ret;
    }
    static Math::half signaling_NaN()
    {
        Math::half ret;
        ret.ieee.frac = 1;
        ret.ieee.exp = Math::half::MAX_EXPONENT_VALUE;
        ret.ieee.sign = 0;
        return ret;
    }
};


} // namespace std
