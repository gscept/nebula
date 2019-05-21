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
		1: (+x, -y, -z), (+x, -y, +z), (-x, -y, +z), (-x, -y, -z)
		1: (-x, +y, +z), (-x, +y, -z), (+x, +y, -z), (+x, +y, +z)
	*/
	px[0] = float4::select(xs[0], xs[1], 1, 1, 0, 0);
	py[0] = ys[0];
	pz[0] = float4::select(zs[0], zs[1], 0, 1, 1, 0);
	
	px[1] = float4::select(xs[0], xs[1], 0, 0, 1, 1);
	py[1] = ys[1];	
	pz[1] = float4::select(zs[0], zs[1], 1, 0, 0, 1);

	float4 m_row_x[4];
	float4 m_row_y[4];
	float4 m_row_z[4];
	float4 m_row_w[4];

	// bad thing is that we need to convert the matrix...
	m_row_x[0] = float4::splat_x(viewProjection.getrow0());
	m_row_x[1] = float4::splat_x(viewProjection.getrow1());
	m_row_x[2] = float4::splat_x(viewProjection.getrow2());
	m_row_x[3] = float4::splat_x(viewProjection.getrow3());

	m_row_y[0] = float4::splat_y(viewProjection.getrow0());
	m_row_y[1] = float4::splat_y(viewProjection.getrow1());
	m_row_y[2] = float4::splat_y(viewProjection.getrow2());
	m_row_y[3] = float4::splat_y(viewProjection.getrow3());

	m_row_z[0] = float4::splat_z(viewProjection.getrow0());
	m_row_z[1] = float4::splat_z(viewProjection.getrow1());
	m_row_z[2] = float4::splat_z(viewProjection.getrow2());
	m_row_z[3] = float4::splat_z(viewProjection.getrow3());

	m_row_w[0] = float4::splat_w(viewProjection.getrow0());
	m_row_w[1] = float4::splat_w(viewProjection.getrow1());
	m_row_w[2] = float4::splat_w(viewProjection.getrow2());
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
		xs[i] = float4::multiplyadd(m_row_x[2], pz[i],
				float4::multiplyadd(m_row_x[1], py[i], 
				float4::multiplyadd(m_row_x[0], px[i], m_row_x[3])));

		ys[i] = float4::multiplyadd(m_row_y[2], pz[i],
				float4::multiplyadd(m_row_y[1], py[i], 
				float4::multiplyadd(m_row_y[0], px[i], m_row_y[3])));

		zs[i] = float4::multiplyadd(m_row_z[2], pz[i],
				float4::multiplyadd(m_row_z[1], py[i], 
				float4::multiplyadd(m_row_z[0], px[i], m_row_z[3])));

		ws[i] = float4::multiplyadd(m_row_w[2], pz[i],
				float4::multiplyadd(m_row_w[1], py[i],
				float4::multiplyadd(m_row_w[0], px[i], m_row_w[3])));
		/*
		xs[i] = float4::multiply(px[i], m_row_x[0]);
		ys[i] = float4::multiply(py[i], m_row_x[1]);
		zs[i] = float4::multiply(pz[i], m_row_x[2]);

		xs[i] = float4::multiplyadd(px[i], m_row_y[0], xs[i]);
		ys[i] = float4::multiplyadd(py[i], m_row_y[1], ys[i]);
		zs[i] = float4::multiplyadd(pz[i], m_row_y[2], zs[i]);

		xs[i] = float4::multiplyadd(px[i], m_row_z[0], xs[i]);
		ys[i] = float4::multiplyadd(py[i], m_row_z[1], ys[i]);
		zs[i] = float4::multiplyadd(pz[i], m_row_z[2], zs[i]);

		
		
		*/
		xs[i] = (xs[i] + m_row_w[2]);
		ys[i] = (ys[i] + m_row_w[2]);
		zs[i] = (zs[i] + m_row_w[2]);
		ws[i] = zs[i];

		{
			const float4 nws = float4::multiply(ws[i], float4(-1.0f));
			const float4 pws = ws[i];
			res1 = float4::multiply(float4::less(xs[i], nws), xLeftFlags);
			res2 = float4::multiply(float4::greater(xs[i], pws), xRightFlags);

			res1 = float4::multiplyadd(float4::less(ys[i], nws), yBottomFlags, res1);
			res2 = float4::multiplyadd(float4::greater(ys[i], pws), yTopFlags, res2);

			res1 = float4::multiplyadd(float4::less(zs[i], nws), zNearFlags, res1);
			res2 = float4::multiplyadd(float4::greater(zs[i], pws), zFarFlags, res2);
		}

		alignas(16) uint res1_u[4];
		res1.storeui((uint*)res1_u);
		alignas(16) uint res2_u[4];
		res2.storeui((uint*)res2_u);

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