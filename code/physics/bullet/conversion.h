#pragma once
//------------------------------------------------------------------------------
/**
	conversion utils for converting between bullet and nebula math types

	(C) 2012-2016 Individual contributors, see AUTHORS file
*/
#include "math/float2.h"
#include "math/line.h"
#include "math/vector.h"
#include "math/matrix44.h"

inline
Math::vector Bt2NebVector(const btVector3 &btV)
{
	return Math::vector(btV.x(), btV.y(), btV.z());
}

//------------------------------------------------------------------------------
/**
*/
inline
Math::point Bt2NebPoint(const btVector3 &btV)
{
	return Math::point(btV.x(), btV.y(), btV.z());
}

//------------------------------------------------------------------------------
/**
*/
inline
Math::matrix44 Bt2NebMatrix3x3(const btMatrix3x3 &m)
{
	return Math::matrix44(Math::float4(m.getRow(0).x(), m.getRow(1).x(), m.getRow(2).x(), 0.0f), 
		Math::float4(m.getRow(0).y(), m.getRow(1).y(), m.getRow(2).y(), 0.0f), 
		Math::float4(m.getRow(0).z(), m.getRow(1).z(), m.getRow(2).z(), 0.0f), 
		Math::float4(0.0f, 0.0f, 0.0f, 1.0f));
}

//------------------------------------------------------------------------------
/**
*/
inline
Math::matrix44 Bt2NebTransform(const btTransform &t)
{
	const btMatrix3x3 &rot = t.getBasis();
	Math::matrix44 m = Bt2NebMatrix3x3(rot);
	m.set_position(Bt2NebPoint(t.getOrigin()));
	return m;
}

//------------------------------------------------------------------------------
/**
*/
inline
btMatrix3x3 Neb2BtM44M33(const Math::matrix44 &m)
{
	return btMatrix3x3(m.getrow0().x(), m.getrow1().x(), m.getrow2().x(), 
		m.getrow0().y(), m.getrow1().y(), m.getrow2().y(), 
		m.getrow0().z(), m.getrow1().z(), m.getrow2().z());
}



//------------------------------------------------------------------------------
/**
*/
inline
btVector3 Neb2BtPoint(const Math::point &p)
{
	return btVector3(p.x(), p.y(), p.z());
}

//------------------------------------------------------------------------------
/**
*/
inline
btVector3 Neb2BtVector(const Math::vector &p)
{
	return btVector3(p.x(), p.y(), p.z());
}

//------------------------------------------------------------------------------
/**
*/
inline
btTransform Neb2BtM44Transform(const Math::matrix44 &m)
{
	btTransform transform;
	transform.setBasis(Neb2BtM44M33(m));
	transform.setOrigin(Neb2BtPoint(m.get_position()));
	return transform;
}