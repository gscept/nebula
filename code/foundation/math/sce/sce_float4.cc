//------------------------------------------------------------------------------
//  float4.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "math/sce/sce_float4.h"
#include "math/sce/sce_matrix44.h"

namespace Math
{

//------------------------------------------------------------------------------
/**
*/
float4
float4::transform(const float4& v, const matrix44& m)
{
    float4 res;
    // XXX: D3DXVec4Transform((D3DXVECTOR4*)&res, (CONST D3DXVECTOR4*)&v, (CONST D3DXMATRIX*)&m);
    return res;
}

//------------------------------------------------------------------------------
/**
*/
float4
float4::clamp(const float4& vClamp, const float4& vMin, const float4& vMax)
{
    return minimize(maximize(vClamp, vMin), vMax);
}

} // namespace Math