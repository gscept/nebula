//------------------------------------------------------------------------------
//  bbox.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "math/bbox.h"

namespace Math
{

//------------------------------------------------------------------------------
/**
    Return box/box clip status.
*/
ClipStatus::Type
bbox::clipstatus(const bbox& other) const
{
    if (this->contains(other))
    {
        return ClipStatus::Inside;
    }
    else if (this->intersects(other))
    {
        return ClipStatus::Clipped;
    }
    else 
    {
        return ClipStatus::Outside;
    }
}

//------------------------------------------------------------------------------
/**
    Returns one of the 8 corners of the bounding box.
*/
point
bbox::corner_point(int index) const
{
    n_assert((index >= 0) && (index < 8));
    switch (index)
    {
        case 0:     return this->pmin;
        case 1:     return point(this->pmin.x(), this->pmax.y(), this->pmin.z());
        case 2:     return point(this->pmax.x(), this->pmax.y(), this->pmin.z());
        case 3:     return point(this->pmax.x(), this->pmin.y(), this->pmin.z());
        case 4:     return this->pmax;
        case 5:     return point(this->pmin.x(), this->pmax.y(), this->pmax.z());
        case 6:     return point(this->pmin.x(), this->pmin.y(), this->pmax.z());
        default:    return point(this->pmax.x(), this->pmin.y(), this->pmax.z());
    }    
}

//------------------------------------------------------------------------------
/**
    Get the bounding box's side planes in clip space.
*/
void
bbox::get_clipplanes(const matrix44& viewProj, Util::Array<plane>& outPlanes) const
{
    matrix44 inverseTranspose = matrix44::transpose(matrix44::inverse(viewProj));
    plane planes[6];
    planes[0].set(-1, 0, 0, +this->pmax.x());
    planes[1].set(+1, 0, 0, -this->pmin.x());
    planes[2].set(0, -1, 0, +this->pmax.y());
    planes[3].set(0, +1, 0, -this->pmin.y());
    planes[4].set(0, 0, -1, +this->pmax.z());
    planes[5].set(0, 0, +1, -this->pmin.z());
    IndexT i;
    for (i = 0; i < 6; i++)
    {
        outPlanes.Append(matrix44::transform(planes[i], inverseTranspose));
    }
}

//------------------------------------------------------------------------------
/**
*/
ClipStatus::Type
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
	/*
		the points would be:

			P1	P2	P3	P4
		X0: x1, x1, x0, x0
		Y0: y0, y0, y0, y0
		Z0: z0, z1, z1, z0

			P5	P6	P7	P8
		X1: x0, x0, x1, x1
		Y1: y1, y1, y1, y1
		Z1: z1, z0, z0, z1

		Meaning P1, P4, P6 and P7 are near plane, P2, P3, P5, P8 are far plane
	*/
	px[0] = float4::select(xs[0], xs[1], 1, 1, 0, 0);
	py[0] = ys[0];
	pz[0] = float4::select(zs[0], zs[1], 1, 0, 0, 1);
	
	px[1] = float4::select(xs[0], xs[1], 0, 0, 1, 1);
	py[1] = ys[1];	
	pz[1] = float4::select(zs[0], zs[1], 0, 1, 1, 0);

	float4 m_col_x[4];
	float4 m_col_y[4];
	float4 m_col_z[4];
	float4 m_col_w[4];

	// splat the matrix such that all _x, _y, ... will contain the column values of x, y, ...
	m_col_x[0] = float4::splat_x(viewProjection.getrow0());
	m_col_x[1] = float4::splat_x(viewProjection.getrow1());
	m_col_x[2] = float4::splat_x(viewProjection.getrow2());
	m_col_x[3] = float4::splat_x(viewProjection.getrow3());

	m_col_y[0] = float4::splat_y(viewProjection.getrow0());
	m_col_y[1] = float4::splat_y(viewProjection.getrow1());
	m_col_y[2] = float4::splat_y(viewProjection.getrow2());
	m_col_y[3] = float4::splat_y(viewProjection.getrow3());

	m_col_z[0] = float4::splat_z(viewProjection.getrow0());
	m_col_z[1] = float4::splat_z(viewProjection.getrow1());
	m_col_z[2] = float4::splat_z(viewProjection.getrow2());
	m_col_z[3] = float4::splat_z(viewProjection.getrow3());

	m_col_w[0] = float4::splat_w(viewProjection.getrow0());
	m_col_w[1] = float4::splat_w(viewProjection.getrow1());
	m_col_w[2] = float4::splat_w(viewProjection.getrow2());
	m_col_w[3] = float4::splat_w(viewProjection.getrow3());

	float4 p1;
	float4 res1;
	const float4 xLeftFlags(ClipLeft);
	const float4 xRightFlags(ClipRight);
	const float4 yBottomFlags(ClipBottom);
	const float4 yTopFlags(ClipTop);
	const float4 zFarFlags(ClipFar);
	const float4 zNearFlags(ClipNear);

	// check two loops of points arranged as xxxx yyyy zzzz
	IndexT i;
	for (i = 0; i < 2; ++i)
	{
		int clip = 0;

		// transform the x component of 4 points simultaneously 
		xs[i] = float4::multiplyadd(m_col_x[2], pz[i],
				float4::multiplyadd(m_col_x[1], py[i], 
				float4::multiplyadd(m_col_x[0], px[i], m_col_x[3])));

		// transform the y component of 4 points simultaneously 
		ys[i] = float4::multiplyadd(m_col_y[2], pz[i],
				float4::multiplyadd(m_col_y[1], py[i], 
				float4::multiplyadd(m_col_y[0], px[i], m_col_y[3])));

		// transform the z component of 4 points simultaneously 
		zs[i] = float4::multiplyadd(m_col_z[2], pz[i],
				float4::multiplyadd(m_col_z[1], py[i], 
				float4::multiplyadd(m_col_z[0], px[i], m_col_z[3])));

		// transform the w component of 4 points simultaneously 
		ws[i] = float4::multiplyadd(m_col_w[2], pz[i],
				float4::multiplyadd(m_col_w[1], py[i],
				float4::multiplyadd(m_col_w[0], px[i], m_col_w[3])));

		{
			const float4 nws = float4::multiply(ws[i], float4(-1.0f));
			const float4 pws = ws[i];

			// add all flags together into one big vector of flags for all 4 points
			res1 = float4::multiplyadd(float4::less(xs[i], nws), xLeftFlags,
				   float4::multiplyadd(float4::greater(xs[i], pws), xRightFlags,
				   float4::multiplyadd(float4::less(ys[i], nws), yBottomFlags, 
				   float4::multiplyadd(float4::greater(ys[i], pws), yTopFlags,
				   float4::multiplyadd(float4::less(zs[i], nws), zFarFlags, 
				   float4::multiply(float4::greater(zs[i], pws), zNearFlags))))));
		}

		// read to stack and convert to uint in one swoop
		alignas(16) uint res1_u[4];
		res1.storeui((uint*)res1_u);

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