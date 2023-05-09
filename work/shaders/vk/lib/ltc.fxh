//------------------------------------------------------------------------------
//  @file ltc.fxh
//  @copyright (C) 2023 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

const float LUT_SIZE = 64.0;
const float LUT_SCALE = (LUT_SIZE - 1.0f) / LUT_SIZE;
const float LUT_BIAS = 0.5f / LUT_SIZE;

//------------------------------------------------------------------------------
/**
    Integration of p0 with distance
*/
float Fp0(float d, float l)
{
    return l / (d * (d * d + l * l)) + atan(1 / d) / (d * d);
}

//------------------------------------------------------------------------------
/**
    Integration of tangent with distance
*/
float Fwt(float d, float l)
{
    return l * l / (d * (d * d + l * l));
}

//------------------------------------------------------------------------------
/**
    Calculate linearly transformed cosines for a line going from points p1 to p2
*/
float LtcLineIntegrate(vec3 p1, vec3 p2)
{
    vec3 wt = normalize(p2 - p1);

    // Clamp to line ends
    if (p1.z <= 0.0f && p2.z <= 0.0f) return 0.0f;
    if (p1.z < 0.0f) p1 = (+p1 * p2.z - p2 * p1.z) / (+p2.z - p1.z);
    if (p2.z < 0.0f) p2 = (-p1 * p2.z + p2 * p1.z) / (-p2.z + p1.z);

    float l1 = dot(p1, wt);
    float l2 = dot(p2, wt);

    // Project point on surface
    vec3 p0 = p1 - l1 * wt;

    // The distance from surface to the line is then
    float d = length(p0);

    // Now integrate diffuse over the hemisphere
    float I = (Fp0(d, l2) - Fp0(d, l1)) * p0.z +
        (Fwt(d, l2) - Fwt(d, l1)) * wt.z;

    return I / PI;
}

//------------------------------------------------------------------------------
/**
*/
vec3
LtcEdgeIntegrate(vec3 p1, vec3 p2)
{
    float x = dot(p1, p2);
    float y = abs(x);

    float a = 0.8543985 + (0.4965155 + 0.0145206 * y) * y;
    float b = 3.4175940 + (4.1616724 + y) * y;
    float v = a / b;

    float theta_sintheta = (x > 0.0) ? v : 0.5 * inversesqrt(max(1.0 - x * x, 1e-7)) - v;

    return cross(p1, p2) * theta_sintheta;
}

//------------------------------------------------------------------------------
/**
*/
float
LtcRectIntegrate(vec3 n, vec3 v, vec3 p, mat3 minv, vec3 corners[4], bool specular, bool twoSided)
{
    vec3 t1, t2;
    t1 = normalize(v - n * dot(v, n));
    t2 = cross(n, t1);

    vec3 L[4];
    L[0] = corners[0] - p;
    L[1] = corners[1] - p;
    L[2] = corners[2] - p;
    L[3] = corners[3] - p;
    if (specular)
    {
        minv = minv * transpose(mat3(t1, t2, n));
        L[0] = minv * L[0];
        L[1] = minv * L[1];
        L[2] = minv * L[2];
        L[3] = minv * L[3];
    }

    float sum = 0.0f;

    vec3 dir = corners[0] - p;
    vec3 lightNormal = cross(corners[1] - corners[0], corners[3] - corners[0]);
    bool behind = dot(dir, lightNormal) < 0.0f;
    if (behind && !twoSided)
        return 0.0f;

    L[0] = normalize(L[0]);
    L[1] = normalize(L[1]);
    L[2] = normalize(L[2]);
    L[3] = normalize(L[3]);

    vec3 vsum = vec3(0.0f);
    vsum += LtcEdgeIntegrate(L[0], L[1]);
    vsum += LtcEdgeIntegrate(L[1], L[2]);
    vsum += LtcEdgeIntegrate(L[2], L[3]);
    vsum += LtcEdgeIntegrate(L[3], L[0]);

    float len = length(vsum);
    float z = vsum.z / len;

    z *= behind ? -1.0f : 1.0f;

    vec2 uv = vec2(z * 0.5f + 0.5f, len);
    uv = uv * LUT_SCALE + LUT_BIAS;

    float scale = sample2D(ltcLUT1, LinearSampler, uv).w;

    sum = len * scale;
    return sum;
}

// An extended version of the implementation from
// "How to solve a cubic equation, revisited"
// http://momentsingraphics.de/?p=105
vec3 
LtcSolveCubic(vec4 Coefficient)
{
    // Normalize the polynomial
    Coefficient.xyz /= Coefficient.w;
    // Divide middle coefficients by three
    Coefficient.yz /= 3.0;

    float A = Coefficient.w;
    float B = Coefficient.z;
    float C = Coefficient.y;
    float D = Coefficient.x;
    const float pi = 3.14159265;

    // Compute the Hessian and the discriminant
    vec3 Delta = vec3(
        -Coefficient.z * Coefficient.z + Coefficient.y,
        -Coefficient.y * Coefficient.z + Coefficient.x,
        dot(vec2(Coefficient.z, -Coefficient.y), Coefficient.xy)
    );

    float Discriminant = dot(vec2(4.0 * Delta.x, -Delta.y), Delta.zy);

    vec3 RootsA, RootsD;

    vec2 xlc, xsc;

    // Algorithm A
    {
        float A_a = 1.0;
        float C_a = Delta.x;
        float D_a = -2.0 * B * Delta.x + Delta.y;

        // Take the cubic root of a normalized complex number
        float Theta = atan(sqrt(Discriminant), -D_a) / 3.0;

        float x_1a = 2.0 * sqrt(-C_a) * cos(Theta);
        float x_3a = 2.0 * sqrt(-C_a) * cos(Theta + (2.0 / 3.0) * pi);

        float xl;
        if ((x_1a + x_3a) > 2.0 * B)
            xl = x_1a;
        else
            xl = x_3a;

        xlc = vec2(xl - B, A);
    }

    // Algorithm D
    {
        float A_d = D;
        float C_d = Delta.z;
        float D_d = -D * Delta.y + 2.0 * C * Delta.z;

        // Take the cubic root of a normalized complex number
        float Theta = atan(D * sqrt(Discriminant), -D_d) / 3.0;

        float x_1d = 2.0 * sqrt(-C_d) * cos(Theta);
        float x_3d = 2.0 * sqrt(-C_d) * cos(Theta + (2.0 / 3.0) * pi);

        float xs;
        if (x_1d + x_3d < 2.0 * C)
            xs = x_1d;
        else
            xs = x_3d;

        xsc = vec2(-D, xs + C);
    }

    float E = xlc.y * xsc.y;
    float F = -xlc.x * xsc.y - xlc.y * xsc.x;
    float G = xlc.x * xsc.x;

    vec2 xmc = vec2(C * F - B * G, -B * F + C * E);

    vec3 Root = vec3(xsc.x / max(xsc.y, 0.0001f), xmc.x / max(xmc.y, 0.0001f), xlc.x / max(xlc.y, 0.0001f));

    if (Root.x < Root.y && Root.x < Root.z)
        Root.xyz = Root.yxz;
    else if (Root.z < Root.x && Root.z < Root.y)
        Root.xyz = Root.xzy;

    return Root;
}

//------------------------------------------------------------------------------
/**
*/
float
LtcDiskIntegrate(vec3 n, vec3 v, vec3 p, mat3 minv, vec3 corners[3], bool specular, bool twoSided)
{
    vec3 t1, t2;
    t1 = normalize(v - n * dot(v, n));
    t2 = cross(n, t1);

    mat3 r = transpose(mat3(t1, t2, n));

    vec3 L[3];
    L[0] = r * (corners[0] - p);
    L[1] = r * (corners[1] - p);
    L[2] = r * (corners[2] - p);

    vec3 c = 0.5f * (L[0] + L[2]);
    vec3 v1 = 0.5f * (L[1] - L[2]);
    vec3 v2 = 0.5f * (L[1] - L[0]);
    if (specular)
    {
        c = minv * c;
        v1 = minv * v1;
        v2 = minv * v2;
    }

    if (!twoSided && dot(cross(v1, v2), c) < 0.0001f)
        return 0.0f;

    // Compute ellipsis eigen vectors
    float a, b;
    float d11 = dot(v1, v1);
    float d22 = dot(v2, v2);
    float d12 = dot(v1, v2);

    if (abs(d12) / sqrt(d11 * d22) > 0.0f)
    {
        float tr = d11 + d22;
        float det = -d12 * d12 + d11 * d22;

        det = sqrt(det);
        float u = 0.5f * sqrt(tr - 2.0f * det);
        float v = 0.5f * sqrt(tr + 2.0f * det);

        float e_max = pow(u + v, 2);
        float e_min = pow(u - v, 2);

        vec3 _v1, _v2;

        if (d11 > d22)
        {
            _v1 = d12 * v1 + (e_max - d11) * v2;
            _v2 = d12 * v1 + (e_min - d11) * v2;
        }
        else
        {
            _v1 = d12 * v2 + (e_max - d22) * v1;
            _v2 = d12 * v2 + (e_min - d22) * v1;
        }

        a = 1.0f / e_max;
        b = 1.0f / e_min;
        v1 = normalize(_v1);
        v2 = normalize(_v2);
    }
    else
    {
        a = 1.0f / dot(v1, v1);
        b = 1.0f / dot(v2, v2);
        v1 *= sqrt(a);
        v2 *= sqrt(b);
    }

    vec3 v3 = cross(v1, v2);
    v3 *= dot(c, v3) < 0.0f ? -1.0f : 1.0f;

    float l = dot(v3, c);
    float x0 = dot(v1, c) / l;
    float y0 = dot(v2, c) / l;

    a *= l * l;
    b *= l * l;

    float c0 = a * b;
    float c1 = a * b * (1.0f + x0 * x0 + y0 * y0) - a - b;
    float c2 = 1.0f - a * (1.0f + x0 * x0) - b * (1.0f + y0 * y0);
    float c3 = 1.0f;

    vec3 roots = LtcSolveCubic(vec4(c0, c1, c2, c3));
    float e1 = roots.x;
    float e2 = roots.y;
    float e3 = roots.z;

    vec3 avgDir = vec3(a * x0 / (a - e2), b * y0 / (b - e2), 1.0f);
    mat3 rotate = mat3(v1, v2, v3);

    avgDir = normalize(rotate * avgDir);

    float L1 = sqrt(-e2 / e3);
    float L2 = sqrt(-e2 / e1);
    float formFactor = L1 * L2 * inversesqrt((1.0f + L1 * L1) * (1.0f + L2 * L2));

    vec2 uv = vec2(avgDir.z * 0.5f + 0.5f, formFactor);
    uv = uv * LUT_SCALE + LUT_BIAS;
    float scale = sample2D(ltcLUT1, LinearSampler, uv).w;

    return formFactor * scale;
}