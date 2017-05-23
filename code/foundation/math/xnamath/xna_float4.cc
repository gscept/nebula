//------------------------------------------------------------------------------
//  float4.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2014 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "math/float4.h"
#include "math/matrix44.h"
#include "system/byteorder.h"

namespace Math
{

//------------------------------------------------------------------------------
/**
*/
float4
float4::transform(__Float4Arg v, const matrix44 &m)
{
    return XMVector4Transform(v.vec, m.mx);
}

//------------------------------------------------------------------------------
/**
*/
float4
float4::clamp(__Float4Arg vClamp, __Float4Arg vMin, __Float4Arg vMax)
{
    return XMVectorClamp(vClamp.vec, vMin.vec, vMax.vec);
}

//------------------------------------------------------------------------------
/**
*/
scalar
float4::angle(__Float4Arg v0, __Float4Arg v1)
{
    return float4::unpack_x(XMVector4AngleBetweenVectors(v0.vec, v1.vec));
}

//------------------------------------------------------------------------------
/**
*/
float4 
float4::select( const float4& v0, const float4& v1, const float4& control )
{
	return XMVectorSelect(v0.vec, v1.vec, control.vec);
}

//------------------------------------------------------------------------------
/**
*/
void
float4::load_ubyte4n_signed(const void* ptr, float w)
{
    // need to endian-convert the source...
    XMUBYTEN4 ub4nValue = System::ByteOrder::Convert<uint>(System::ByteOrder::Host, System::ByteOrder::LittleEndian, *(uint*)ptr);
    this->vec = XMVectorSubtract(XMVectorScale(XMLoadUByteN4(&ub4nValue), 2.0f), XMVectorSplatOne());
    this->set_w(w);
}

} // namespace Math
