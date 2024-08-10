#pragma once
//------------------------------------------------------------------------------
/**
    Different curves

    @copyright
    (C) 2024 Individual contributors, see AUTHORS file
*/

namespace Math
{

template<class POINT>
struct BezierCubic
{
    BezierCubic(POINT pp0, POINT pp1, POINT pp2, POINT pp3) { Set(pp0, pp1, pp2, pp3); }
    void Set(POINT pp0, POINT pp1, POINT pp2, POINT pp3) { p0 = pp0; p3 = pp3; s1 = pp1*3.0f - pp0 - pp3; s2 = pp2*3.0f - pp3 - pp0; }
    POINT Eval(float t)
    {
        POINT p03 = lerp(p0, p3, t);
        POINT s12 = lerp(s1, s2, t);
        return lerp(p03, s12, 1.0f - t);
    }

    POINT p0, p3, s1, s2;
};

};
