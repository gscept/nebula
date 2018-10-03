#pragma once
//------------------------------------------------------------------------------
/**
    @class Math::bbox

    Nebula's bounding box class.

    @todo: UNTESTED!

    (C) 2004 RadonLabs GmbH
*/
#include "math/point.h"
#include "math/vector.h"
#include "math/matrix44.h"
#include "math/line.h"
#include "math/plane.h"
#include "math/clipstatus.h"
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
        ClipLeft   = (1<<0),
        ClipRight  = (1<<1),
        ClipBottom = (1<<2),
        ClipTop    = (1<<3),
        ClipNear   = (1<<4),
        ClipFar    = (1<<5),
    };

    /// constructor 1
    bbox();
    /// constructor 3
    bbox(const point& center, const vector& extents);
    /// construct bounding box from matrix44
    bbox(const matrix44& m);
    /// get center of box
    point center() const;
    /// get extents of box
    vector extents() const;
    /// get size of box
    vector size() const;
    /// get diagonal size of box
    scalar diagonal_size() const;
    /// set from matrix44
    void set(const matrix44& m);
    /// set from center point and extents
    void set(const point& center, const vector& extents);
    /// begin extending the box
    void begin_extend();
    /// extend the box
    void extend(const point& p);
    /// extend the box
    void extend(const bbox& box);
    /// this resets the bounding box size to zero if no extend() method was called after begin_extend()
    void end_extend();
    /// transform bounding box
    void transform(const matrix44& m);
    /// affine transform bounding box, does not allow for projections
    void affine_transform(const matrix44& m);
    /// check for intersection with axis aligned bounding box
    bool intersects(const bbox& box) const;
    /// check if this box completely contains the parameter box
    bool contains(const bbox& box) const;
    /// return true if this box contains the position
    bool contains(const point& p) const;
    /// check for intersection with other bounding box
    ClipStatus::Type clipstatus(const bbox& other) const;
    /// check for intersection with projection volume
    ClipStatus::Type clipstatus(const matrix44& viewProjection) const;
	/// check for intersection with projection volume in a SoA manner
	ClipStatus::Type clipstatus_soa(const matrix44& viewProjection) const;
    /// create a matrix which transforms a unit cube to this bounding box
    matrix44 to_matrix44() const;
    /// return one of the 8 corner points
    point corner_point(int index) const;
    /// return side planes in clip space
    void get_clipplanes(const matrix44& viewProjection, Util::Array<plane>& outPlanes) const;
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
bbox::set(const matrix44& m)
{
    // get extents
    vector extents = ( m.getrow0().abs() + m.getrow1().abs() + m.getrow2().abs() ) * 0.5f;
    point center = m.getrow3();
    this->pmin = center - extents;
    this->pmax = center + extents;
}

//------------------------------------------------------------------------------
/**
*/
inline
bbox::bbox(const matrix44& m)
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
inline vector
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
bbox::extend(const point& p)
{
    this->pmin = float4::minimize(this->pmin, p);
    this->pmax = float4::maximize(this->pmax, p);
}

//------------------------------------------------------------------------------
/**
*/
inline
void
bbox::extend(const bbox& box)
{
    this->pmin = float4::minimize(this->pmin, box.pmin);
    this->pmax = float4::maximize(this->pmax, box.pmax);
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
bbox::transform(const matrix44& m)
{
    Math::point temp;
    Math::point minP(1000000, 1000000,1000000);
    Math::point maxP(-1000000, -1000000, -1000000);        
    IndexT i; 
        
    for(i = 0; i < 8; ++i)
    {
        // Transform and check extents
        float4 temp_f = Math::matrix44::transform(corner_point(i), m);
        temp = float4::perspective_div(temp_f);
        maxP = float4::maximize(temp, maxP);
        minP = float4::minimize(temp, minP);        
    }    

    this->pmin = minP;
    this->pmax = maxP;
}

//------------------------------------------------------------------------------
/**
    
*/
inline void
bbox::affine_transform(const matrix44& m)
{
    n_assert2(m.getrow0().w() == 0 && m.getrow1().w() == 0 && m.getrow2().w() == 0 && m.getrow3().w() == 1, "Matrix is not affine");

    float4 xa = m.get_xaxis() * this->pmin.x();
    float4 xb = m.get_xaxis() * this->pmax.x();

    float4 ya = m.get_yaxis() * this->pmin.y();
    float4 yb = m.get_yaxis() * this->pmax.y();

    float4 za = m.get_zaxis() * this->pmin.z();
    float4 zb = m.get_zaxis() * this->pmax.z();
    
    this->pmin = float4::minimize(xa, xb) + float4::minimize(ya, yb) + float4::minimize(za, zb) + m.get_position();
    this->pmax = float4::maximize(xa, xb) + float4::maximize(ya, yb) + float4::maximize(za, zb) + m.get_position();
}

//------------------------------------------------------------------------------
/**
    Check for intersection of 2 axis aligned bounding boxes. The 
    bounding boxes must live in the same coordinate space.
*/
inline bool
bbox::intersects(const bbox& box) const
{
    bool lt = float4::less3_any(this->pmax, box.pmin);
    bool gt = float4::greater3_any(this->pmin, box.pmax);
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
    bool lt = float4::less3_all(this->pmin, box.pmin);
    bool ge = float4::greaterequal3_all(this->pmax, box.pmax);
    return lt && ge;
}

//------------------------------------------------------------------------------
/**
    Check if position is inside bounding box.
*/
inline bool
bbox::contains(const point& p) const
{
    bool lt = float4::less3_all(this->pmin, p);
    bool ge = float4::greaterequal3_all(this->pmax, p);
    return lt && ge;
}

//------------------------------------------------------------------------------
/**
    Create a transform matrix which would transform a unit cube to this
    bounding box.
*/
inline matrix44
bbox::to_matrix44() const
{
    matrix44 m = matrix44::scaling(this->size());
    float4 pos = this->center();
    m.set_position(pos);
    return m;
}

//------------------------------------------------------------------------------
/**
*/
inline scalar
bbox::diagonal_size() const
{
    return (this->pmax - this->pmin).length();
}

//------------------------------------------------------------------------------
/**
    Check for intersection with a view volume defined by a view-projection
    matrix.
*/
__forceinline ClipStatus::Type
bbox::clipstatus(const matrix44& viewProjection) const
{
    // @todo: needs optimization!
    int andFlags = 0xffff;
    int orFlags  = 0;

    // corner points
    // get points by using permute with min and max, that is some pretty math right there!
    point p[8];
    p[0] = this->pmin;
    p[1] = float4::permute(this->pmin, this->pmax, 0, 1, 6, 3);
    p[2] = float4::permute(this->pmin, this->pmax, 4, 1, 6, 3);
    p[3] = float4::permute(this->pmin, this->pmax, 4, 1, 2, 3);
    p[4] = float4::permute(this->pmin, this->pmax, 0, 5, 2, 3);
    p[5] = float4::permute(this->pmin, this->pmax, 0, 5, 6, 3);
    p[6] = float4::permute(this->pmin, this->pmax, 4, 5, 2, 3);
    p[7] = this->pmax;
    
    // check each corner point
    float4 p1;
    float4 res1, res2;
    const float4 lowerFlags(ClipLeft, ClipBottom, ClipFar, 0);
    const float4 upperFlags(ClipRight, ClipTop, ClipNear, 0);
    IndexT i;
    for (i = 0; i < 8; ++i)
    {
        int clip = 0;
        p1 = matrix44::transform(p[i], viewProjection);
        res1 = float4::less(p1, float4(-p1.w()));
        res2 = float4::greater(p1, float4(p1.w()));
        res1 = float4::multiply(res1, lowerFlags);
        res2 = float4::multiply(res2, upperFlags);

		alignas(16) uint res1_u[4];
		res1.storeu((scalar*)res1_u);
		alignas(16) uint res2_u[4];
		res2.storeu((scalar*)res2_u);

        clip |= res1_u[0];
        clip |= res2_u[0];
        clip |= res1_u[1];
        clip |= res2_u[1];
        clip |= res1_u[2];
        clip |= res2_u[2];

        andFlags &= clip;
        orFlags  |= clip;
    }
    if (0 == orFlags)       return ClipStatus::Inside;
    else if (0 != andFlags) return ClipStatus::Outside;
    else                    return ClipStatus::Clipped;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline ClipStatus::Type
bbox::clipstatus_soa(const matrix44& viewProjection) const
{
	int andFlags = 0xffff;
	int orFlags = 0;

	// create vectors for each dimension of each point, xxxx, yyyy, zzzz
	float4 xs[2];
	float4 ys[2];
	float4 zs[2];
	float4 ws[2];
	
	// just set the x, y and z for min and max
	xs[0] = float4(this->pmin.x());
	ys[0] = float4(this->pmin.y());
	zs[0] = float4(this->pmin.z());
	ws[0] = float4(1.0f);

	xs[1] = float4(this->pmax.x());
	ys[1] = float4(this->pmax.y());
	zs[1] = float4(this->pmax.z());
	ws[1] = float4(1.0f);

	// create vectors for each unique point, 0 is the first set of points, 1 is the second (total 8 points)
	float4 px[2];
	float4 py[2];
	float4 pz[2];

	// this corresponds to the permute phase in the original function
	px[0] = float4::select(xs[0], xs[1], 0, 0, 1, 1);
	px[1] = float4::select(xs[0], xs[1], 0, 0, 1, 1);
	py[0] = ys[0];
	py[1] = ys[1];
	pz[0] = float4::select(zs[0], zs[1], 0, 1, 1, 0);
	pz[1] = float4::select(zs[0], zs[1], 0, 1, 0, 1);

	float4 m_row_x[4];
	float4 m_row_y[4];
	float4 m_row_z[4];
	float4 m_row_w[4];

	// bad thing is that we need to convert the matrix...
	m_row_x[0] = float4::splat_x(viewProjection.getrow0());
	m_row_x[1] = float4::splat_y(viewProjection.getrow0());
	m_row_x[2] = float4::splat_z(viewProjection.getrow0());
	m_row_x[3] = float4::splat_w(viewProjection.getrow0());

	m_row_y[0] = float4::splat_x(viewProjection.getrow1());
	m_row_y[1] = float4::splat_y(viewProjection.getrow1());
	m_row_y[2] = float4::splat_z(viewProjection.getrow1());
	m_row_y[3] = float4::splat_w(viewProjection.getrow1());

	m_row_z[0] = float4::splat_x(viewProjection.getrow2());
	m_row_z[1] = float4::splat_y(viewProjection.getrow2());
	m_row_z[2] = float4::splat_z(viewProjection.getrow2());
	m_row_z[3] = float4::splat_w(viewProjection.getrow2());

	m_row_w[0] = float4::splat_x(viewProjection.getrow3());
	m_row_w[1] = float4::splat_y(viewProjection.getrow3());
	m_row_w[2] = float4::splat_z(viewProjection.getrow3());
	m_row_w[3] = float4::splat_w(viewProjection.getrow3());

	float4 p1;
	float4 res1, res2;
	const float4 xLeftFlags(ClipLeft);
	const float4 xRightFlags(ClipRight);
	const float4 yBottomFlags(ClipBottom);
	const float4 yTopFlags(ClipTop);
	const float4 zFarFlags(ClipFar);
	const float4 zNearFlags(ClipNear);

	// check two loops of particles arranged as xxxx yyyy zzzz
	IndexT i;
	for (i = 0; i < 2; ++i)
	{
		int clip = 0;
		xs[i] = float4::multiply(px[i], m_row_x[0]);
		ys[i] = float4::multiply(py[i], m_row_x[1]);
		zs[i] = float4::multiply(pz[i], m_row_x[2]);

		xs[i] = float4::multiplyadd(px[i], m_row_y[0], xs[i]);
		ys[i] = float4::multiplyadd(py[i], m_row_y[1], ys[i]);
		zs[i] = float4::multiplyadd(pz[i], m_row_y[2], zs[i]);

		xs[i] = float4::multiplyadd(px[i], m_row_z[0], xs[i]);
		ys[i] = float4::multiplyadd(py[i], m_row_z[1], ys[i]);
		zs[i] = float4::multiplyadd(pz[i], m_row_z[2], zs[i]);

		xs[i] = (xs[i] + m_row_w[0]);
		ys[i] = (ys[i] + m_row_w[1]);
		zs[i] = (zs[i] + m_row_w[2]);
		ws[i] = pz[i];

		{
			const float4 nws = float4::multiply(ws[i], float4(-1.0f));
			const float4 pws = ws[i];
			res1 = float4::multiply(float4::less(xs[i], nws), xLeftFlags);
			res2 = float4::multiply(float4::greater(xs[i], pws), xRightFlags);

			res1 = float4::multiplyadd(float4::less(ys[i], nws), yBottomFlags, res1);
			res2 = float4::multiplyadd(float4::greater(ys[i], pws), yTopFlags, res2);

			res1 = float4::multiplyadd(float4::less(zs[i], nws), zFarFlags, res1);
			res2 = float4::multiplyadd(float4::greater(zs[i], pws), zNearFlags, res2);
		}

		alignas(16) uint res1_u[4];
		res1.store((scalar*)res1_u);
		alignas(16) uint res2_u[4];
		res2.store((scalar*)res2_u);

		clip |= res1_u[0];
		clip |= res2_u[0];
		clip |= res1_u[1];
		clip |= res2_u[1];
		clip |= res1_u[2];
		clip |= res2_u[2];

		andFlags &= clip;
		orFlags |= clip;
	}
	if (0 == orFlags)       return ClipStatus::Inside;
	else if (0 != andFlags) return ClipStatus::Outside;
	else                    return ClipStatus::Clipped;
}

} // namespace Math
//------------------------------------------------------------------------------
