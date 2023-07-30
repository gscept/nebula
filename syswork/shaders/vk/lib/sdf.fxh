//------------------------------------------------------------------------------
//  @file sdf.fxh
//  @copyright (C) 2022 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
*/
float
sdRect(vec2 p, vec2 size)
{
    vec2 d = abs(p) - size;
    return min(max(d.x, d.y), 0.0) + length(max(d, 0.0));
}

//------------------------------------------------------------------------------
/**
    Returns two values:
    [0] = distance of p to line segment.
    [1] = closest t on line segment, clamped to [0, 1]
*/
vec2 
sdSegment(in vec2 p, in vec2 a, in vec2 b)
{
    vec2 pa = p - a, ba = b - a;
    float t = dot(pa, ba) / dot(ba, ba);
    return vec2(length(pa - ba * t), t);
}

//------------------------------------------------------------------------------
/**
*/
float 
testCross(vec2 a, vec2 b, vec2 p) 
{
    return (b.y - a.y) * (p.x - a.x) - (b.x - a.x) * (p.y - a.y);
}

//------------------------------------------------------------------------------
/**
*/
float 
sdLine(in vec2 a, in vec2 b, in vec2 p)
{
    vec2 pa = p - a, ba = b - a;
    float t = dot(pa, ba) / dot(ba, ba);
    return length(pa - ba * t) * sign(testCross(a, b, p));
}

//------------------------------------------------------------------------------
/**
*/
float
sdEllipse(vec2 p, in vec2 ab) 
{
    if (abs(ab.x - ab.y) < 0.1)
        return length(p) - ab.x;
    p = abs(p); if (p.x > p.y) { p = p.yx; ab = ab.yx; }

    float l = ab.y * ab.y - ab.x * ab.x;

    float m = ab.x * p.x / l;
    float n = ab.y * p.y / l;
    float m2 = m * m;
    float n2 = n * n;

    float c = (m2 + n2 - 1.0) / 3.0;
    float c3 = c * c * c;
    float q = c3 + m2 * n2 * 2.0;
    float d = c3 + m2 * n2;
    float g = m + m * n2;
    float co;
    if (d < 0.0)
    {
        float p = acos(q / c3) / 3.0;
        float s = cos(p);
        float t = sin(p) * sqrt(3.0);
        float rx = sqrt(-c * (s + t + 2.0) + m2);
        float ry = sqrt(-c * (s - t + 2.0) + m2);
        co = (ry + sign(l) * rx + abs(g) / (rx * ry) - m) / 2.0;
    }
    else 
    {
        float h = 2.0 * m * n * sqrt(d);
        float s = sign(q + h) * pow(abs(q + h), 1.0 / 3.0);
        float u = sign(q - h) * pow(abs(q - h), 1.0 / 3.0);
        float rx = -s - u - c * 4.0 + 2.0 * m2;
        float ry = (s - u) * sqrt(3.0);
        float rm = sqrt(rx * rx + ry * ry);
        float p = ry / sqrt(rm - rx);
        co = (p + 2.0 * g / rm - m) / 2.0;
    }
    float si = sqrt(1.0 - co * co);

    vec2 r = vec2(ab.x * co, ab.y * si);

    return length(r - p) * sign(p.y - r.y);
}

//------------------------------------------------------------------------------
/**
*/
float 
sdRoundRect(vec2 p, vec2 size, vec4 rx, vec4 ry)
{
    size *= 0.5;
    vec2 corner;
    corner = vec2(-size.x + rx.x, -size.y + ry.x);  // Top-Left
    vec2 local = p - corner;
    if (dot(rx.x, ry.x) > 0.0 && p.x < corner.x && p.y <= corner.y)
        return sdEllipse(local, vec2(rx.x, ry.x));
    corner = vec2(size.x - rx.y, -size.y + ry.y);   // Top-Right
    local = p - corner;
    if (dot(rx.y, ry.y) > 0.0 && p.x >= corner.x && p.y <= corner.y)
        return sdEllipse(local, vec2(rx.y, ry.y));
    corner = vec2(size.x - rx.z, size.y - ry.z);  // Bottom-Right
    local = p - corner;
    if (dot(rx.z, ry.z) > 0.0 && p.x >= corner.x && p.y >= corner.y)
        return sdEllipse(local, vec2(rx.z, ry.z));
    corner = vec2(-size.x + rx.w, size.y - ry.w); // Bottom-Left
    local = p - corner;
    if (dot(rx.w, ry.w) > 0.0 && p.x < corner.x && p.y > corner.y)
        return sdEllipse(local, vec2(rx.w, ry.w));
    return sdRect(p, size);
}
