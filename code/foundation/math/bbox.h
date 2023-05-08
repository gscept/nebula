#pragma once
//------------------------------------------------------------------------------
/**
    @class Math::bbox

    Nebula's bounding box class.

    @copyright
    (C) 2004 RadonLabs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "math/vec3.h"
#include "math/vec4.h"
#include "math/mat4.h"
#include "math/clipstatus.h"
#include "math/sse.h"
#include "util/array.h"

//------------------------------------------------------------------------------
namespace Math
{
class bbox
{
public:
    /// clip codes
    enum 
    {
        ClipLeft   = N_BIT(1),
        ClipRight  = N_BIT(2),
        ClipBottom = N_BIT(3),
        ClipTop    = N_BIT(4),
        ClipNear   = N_BIT(5),
        ClipFar    = N_BIT(6),
    };

    /// constructor 1
    bbox();
    /// constructor 3
    bbox(const point& center, const vector& extents);
    /// construct bounding box from mat4
    bbox(const mat4& m);
    /// get center of box
    point center() const;
    /// get extents of box
    vector extents() const;
    /// get size of box
    vec3 size() const;
    /// get diagonal size of box
    scalar diagonal_size() const;
    /// set from mat4
    void set(const mat4& m);
    /// set from center point and extents
    void set(const point& center, const vector& extents);
    /// begin extending the box
    void begin_extend();
    /// extend the box
    void extend(const vec3& p);
    /// extend the box
    void extend(const bbox& box);
    /// this resets the bounding box size to zero if no extend() method was called after begin_extend()
    void end_extend();
    /// transform bounding box
    void transform(const mat4& m);
    /// affine transform bounding box, does not allow for projections
    void affine_transform(const mat4& m);
    /// check for intersection with axis aligned bounding box
    bool intersects(const bbox& box) const;
    /// check if this box completely contains the parameter box
    bool contains(const bbox& box) const;
    /// return true if this box contains the position
    bool contains(const vec3& p) const;
    /// check for intersection with other bounding box
    ClipStatus::Type clipstatus(const bbox& other) const;
    /// check for intersection with projection volume
    ClipStatus::Type clipstatus(const mat4& viewProjection, const bool isOrtho = false) const;
    /// check for intersection with pre-calculated columns 
    ClipStatus::Type clipstatus(const vec4* x_columns, const vec4* y_columns, const vec4* z_columns, const vec4* w_columns, const bool isOrtho = false) const;
    /// create a matrix which transforms a unit cube to this bounding box
    mat4 to_mat4() const;
    /// return one of the 8 corner points
    point corner_point(int index) const;
    /// return side planes in clip space
    void get_clipplanes(const mat4& viewProjection, Util::Array<vec4>& outPlanes) const;
    /// convert to any type
    template<typename T> T as() const;

    point pmin;
    point pmax;
};

//------------------------------------------------------------------------------
/**
*/
inline
bbox::bbox() :
    pmin(-0.5f, -0.5f, -0.5f),
    pmax(+0.5f, +0.5f, +0.5f)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
bbox::bbox(const point& center, const vector& extents)
{
    this->pmin = center - extents;
    this->pmax = center + extents;
}

//------------------------------------------------------------------------------
/**
    Construct a bounding box around a 4x4 matrix. The translational part
    defines the center point, and the x,y,z vectors of the matrix
    define the extents.
*/
inline void
bbox::set(const mat4& m)
{
    // get extents
    vec3 extents = xyz( abs(m.r[0]) + abs(m.r[1]) + abs(m.r[2]) ) * 0.5f;
    vec3 center = xyz(m.r[3]);
    this->pmin = center - extents;
    this->pmax = center + extents;
}

//------------------------------------------------------------------------------
/**
*/
inline
bbox::bbox(const mat4& m)
{
    this->set(m);
}

//------------------------------------------------------------------------------
/**
*/
inline point
bbox::center() const
{
    return this->pmin + ((this->pmax - this->pmin) * 0.5f);
}

//------------------------------------------------------------------------------
/**
*/
inline vector
bbox::extents() const
{
    return (this->pmax - this->pmin) * 0.5f;
}

//------------------------------------------------------------------------------
/**
*/
inline vec3
bbox::size() const
{
    return this->pmax - this->pmin;
}

//------------------------------------------------------------------------------
/**
*/
inline void
bbox::set(const point& center, const vector& extents)
{
    this->pmin = center - extents;
    this->pmax = center + extents;
}

//------------------------------------------------------------------------------
/**
*/
inline void
bbox::begin_extend()
{
    this->pmin.set(+1000000.0f, +1000000.0f, +1000000.0f);
    this->pmax.set(-1000000.0f, -1000000.0f, -1000000.0f);
}

//------------------------------------------------------------------------------
/**
    This just checks whether the extend() method has actually been called after
    begin_extend() and just sets vmin and vmax to the null vector if it hasn't.
*/
inline
void
bbox::end_extend()
{
    if ((this->pmin == point(+1000000.0f, +1000000.0f, +1000000.0f)) &&
        (this->pmax == point(-1000000.0f, -1000000.0f, -1000000.0f)))
    {
        this->pmin.set(0.0f, 0.0f, 0.0f);
        this->pmax.set(0.0f, 0.0f, 0.0f);
    }
}

//------------------------------------------------------------------------------
/**
*/
inline void
bbox::extend(const vec3& p)
{
    this->pmin = minimize(this->pmin, p);
    this->pmax = maximize(this->pmax, p);
}

//------------------------------------------------------------------------------
/**
*/
inline
void
bbox::extend(const bbox& box)
{
    this->pmin = minimize(this->pmin, box.pmin);
    this->pmax = maximize(this->pmax, box.pmax);
}

//------------------------------------------------------------------------------
/**
    Transforms this axis aligned bounding by the 4x4 matrix. This bounding
    box must be axis aligned with the matrix, the resulting bounding
    will be axis aligned in the matrix' "destination" space.

    E.g. if you have a bounding box in model space 'modelBox', and a
    'modelView' matrix, the operation
    
    modelBox.transform(modelView)

    would transform the bounding box into view space.
*/
inline void
bbox::transform(const mat4& m)
{
    point temp;
    point minP(1000000, 1000000,1000000);
    point maxP(-1000000, -1000000, -1000000);        
    IndexT i; 
        
    for(i = 0; i < 8; ++i)
    {
        // Transform and check extents
        point temp_f = m * corner_point(i);
        temp = perspective_div(temp_f);
        maxP = maximize(temp, maxP);
        minP = minimize(temp, minP);        
    }    

    this->pmin = minP;
    this->pmax = maxP;
}

//------------------------------------------------------------------------------
/**
    
*/
inline void
bbox::affine_transform(const mat4& m)
{
    n_warn2(m.r[0].w == 0 && m.r[1].w == 0 && m.r[2].w == 0 && m.r[3].w == 1, "Matrix is not affine");

    vec4 xa = m.x_axis * this->pmin.x;
    vec4 xb = m.x_axis * this->pmax.x;

    vec4 ya = m.y_axis * this->pmin.y;
    vec4 yb = m.y_axis * this->pmax.y;

    vec4 za = m.z_axis * this->pmin.z;
    vec4 zb = m.z_axis * this->pmax.z;
    
    this->pmin = xyz(minimize(xa, xb) + minimize(ya, yb) + minimize(za, zb) + m.position);
    this->pmax = xyz(maximize(xa, xb) + maximize(ya, yb) + maximize(za, zb) + m.position);
}

//------------------------------------------------------------------------------
/**
    Check for intersection of 2 axis aligned bounding boxes. The 
    bounding boxes must live in the same coordinate space.
*/
inline bool
bbox::intersects(const bbox& box) const
{
    bool lt = less_any(this->pmax, box.pmin);
    bool gt = greater_any(this->pmin, box.pmax);
    return !(lt || gt);
}

//------------------------------------------------------------------------------
/**
    Check if the parameter bounding box is completely contained in this
    bounding box.
*/
inline bool
bbox::contains(const bbox& box) const
{
    bool lt = less_all(this->pmin, box.pmin);
    bool ge = greaterequal_all(this->pmax, box.pmax);
    return lt && ge;
}

//------------------------------------------------------------------------------
/**
    Check if position is inside bounding box.
*/
inline bool
bbox::contains(const vec3& p) const
{
    bool lt = less_all(this->pmin, p);
    bool ge = greaterequal_all(this->pmax, p);
    return lt && ge;
}

//------------------------------------------------------------------------------
/**
    Create a transform matrix which would transform a unit cube to this
    bounding box.
*/
inline mat4
bbox::to_mat4() const
{
    mat4 m = scaling(this->size());
    point pos = this->center();
    m.position = pos;
    return m;
}

//------------------------------------------------------------------------------
/**
*/
inline scalar
bbox::diagonal_size() const
{
    return length(this->pmax - this->pmin);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline ClipStatus::Type
bbox::clipstatus(const mat4& viewProjection, const bool isOrtho) const
{
    vec4 m_col_x[4];
    vec4 m_col_y[4];
    vec4 m_col_z[4];
    vec4 m_col_w[4];

    // splat the matrix such that all _x, _y, ... will contain the column values of x, y, ...
    m_col_x[0] = splat_x(viewProjection.r[0]);
    m_col_x[1] = splat_x(viewProjection.r[1]);
    m_col_x[2] = splat_x(viewProjection.r[2]);
    m_col_x[3] = splat_x(viewProjection.r[3]);

    m_col_y[0] = splat_y(viewProjection.r[0]);
    m_col_y[1] = splat_y(viewProjection.r[1]);
    m_col_y[2] = splat_y(viewProjection.r[2]);
    m_col_y[3] = splat_y(viewProjection.r[3]);

    m_col_z[0] = splat_z(viewProjection.r[0]);
    m_col_z[1] = splat_z(viewProjection.r[1]);
    m_col_z[2] = splat_z(viewProjection.r[2]);
    m_col_z[3] = splat_z(viewProjection.r[3]);

    m_col_w[0] = splat_w(viewProjection.r[0]);
    m_col_w[1] = splat_w(viewProjection.r[1]);
    m_col_w[2] = splat_w(viewProjection.r[2]);
    m_col_w[3] = splat_w(viewProjection.r[3]);

    return this->clipstatus(m_col_x, m_col_y, m_col_z, m_col_w, isOrtho);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline ClipStatus::Type
bbox::clipstatus(const vec4* x_columns, const vec4* y_columns, const vec4* z_columns, const vec4* w_columns, const bool isOrtho) const
{
    using namespace Math;
    int andFlags = 0xffff;
    int orFlags = 0;

    vec4 xs[2];
    vec4 ys[2];
    vec4 zs[2];
    vec4 ws[2];

    // create vectors for each dimension of each point, xxxx, yyyy, zzzz
    xs[0] = splat_x(this->pmin);
    ys[0] = splat_y(this->pmin);
    zs[0] = splat_z(this->pmin);
    ws[0] = vec4(1.0f);

    xs[1] = splat_x(this->pmax);
    ys[1] = splat_y(this->pmax);
    zs[1] = splat_z(this->pmax);
    ws[1] = vec4(1.0f);

    vec4 px[2];
    vec4 py[2];
    vec4 pz[2];

    // this corresponds to the permute phase in the original function
    /*
        the points would be:

            P1  P2  P3  P4
        X0: x1, x1, x0, x0
        Y0: y0, y0, y0, y0
        Z0: z0, z1, z1, z0

            P5  P6  P7  P8
        X1: x0, x0, x1, x1
        Y1: y1, y1, y1, y1
        Z1: z1, z0, z0, z1

        Meaning P1, P4, P6 and P7 are near plane, P2, P3, P5, P8 are far plane
    */
    px[0] = xs[1] * vec4(1, 1, 0, 0) + xs[0] * vec4(0, 0, 1, 1);
    px[1] = xs[1] * vec4(0, 0, 1, 1) + xs[0] * vec4(1, 1, 0, 0);

    py[0] = ys[0];
    py[1] = ys[1];

    pz[0] = zs[1] * vec4(1, 0, 0, 1) + zs[0] * vec4(0, 1, 1, 0);
    pz[1] = zs[1] * vec4(0, 1, 1, 0) + zs[0] * vec4(1, 0, 0, 1);

    vec4 res1;
    const vec4 xLeftFlags = vec4(ClipLeft);
    const vec4 xRightFlags = vec4(ClipRight);
    const vec4 yBottomFlags = vec4(ClipBottom);
    const vec4 yTopFlags = vec4(ClipTop);
    const vec4 zFarFlags = vec4(ClipFar);
    const vec4 zNearFlags = vec4(ClipNear);

    // check two loops of points arranged as xxxx yyyy zzzz
    IndexT i;
    for (i = 0; i < 2; ++i)
    {
        // transform the x component of 4 points simultaneously 
        xs[i] = 
            multiplyadd(x_columns[2], pz[i],
                multiplyadd(x_columns[1], py[i],
                    multiplyadd(x_columns[0], px[i], x_columns[3])));

        // transform the y component of 4 points simultaneously 
        ys[i] = 
            multiplyadd(y_columns[2], pz[i],
                multiplyadd(y_columns[1], py[i],
                    multiplyadd(y_columns[0], px[i], y_columns[3])));

        // transform the z component of 4 points simultaneously 
        zs[i] = 
            multiplyadd(z_columns[2], pz[i],
                multiplyadd(z_columns[1], py[i],
                    multiplyadd(z_columns[0], px[i], z_columns[3])));

        if (isOrtho)
            ws[i] = vec4(1);
        else
        {
            // transform the w component of 4 points simultaneously 
            ws[i] =
                multiplyadd(w_columns[2], pz[i],
                    multiplyadd(w_columns[1], py[i],
                            multiplyadd(w_columns[0], px[i], w_columns[3])));
        }

        {
            const vec4 nws = -ws[i];
            const vec4 pws = ws[i];

            // add all flags together into one big vector of flags for all 4 points
            res1 = 
                multiplyadd(less(xs[i], nws), xLeftFlags,
                    multiplyadd(greater(xs[i], pws), xRightFlags,
                        multiplyadd(less(ys[i], nws), yBottomFlags,
                            multiplyadd(greater(ys[i], pws), yTopFlags,
                                multiplyadd(less(zs[i], nws), zFarFlags,
                                    (greater(zs[i], pws) * zNearFlags)
                            )
                        )
                    )
                )
            );
        }

        // read to stack and convert to uint in one swoop
        alignas(16) uint res1_u[4];
        __m128i result = _mm_cvttps_epi32(res1.vec);
        _mm_storeu_si128(reinterpret_cast<__m128i*>(res1_u), result);

        // update flags by or-ing and and-ing all 4 points individually
        andFlags &= res1_u[0];
        orFlags |= res1_u[0];
        andFlags &= res1_u[1];
        orFlags |= res1_u[1];
        andFlags &= res1_u[2];
        orFlags |= res1_u[2];
        andFlags &= res1_u[3];
        orFlags |= res1_u[3];

    }
    if (0 == orFlags)       return ClipStatus::Inside;
    else if (0 != andFlags) return ClipStatus::Outside;
    else                    return ClipStatus::Clipped;
}

} // namespace Math
//------------------------------------------------------------------------------
