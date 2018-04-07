//------------------------------------------------------------------------------
//  debugdrawer.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "stdneb.h"

#include "debugdrawer.h"
#include "core/debug.h"
#include "math/point.h"
#include "debug/debugfloat.h"
#include "debugrender/debugshaperenderer.h"
#include "coregraphics/textrenderer.h"
#include "debugrender/debugrender.h"
#include "conversion.h"
#include "bulletscene.h"

namespace Bullet
{
using namespace Math;
using namespace Debug;
using namespace CoreGraphics;

inline 
Math::matrix44 operator*(const Math::matrix44 &a, const Math::matrix44 &b)
{
    return Math::matrix44::multiply(a, b);
}

//------------------------------------------------------------------------------
/**
*/
DebugDrawer::DebugDrawer() :
    m_debugMode(0)
{
}

//------------------------------------------------------------------------------
/**
*/
DebugDrawer::~DebugDrawer()
{
}

//------------------------------------------------------------------------------
/**
*/
void	
DebugDrawer::drawLine(const btVector3& from,const btVector3& to,const btVector3& color)
{
	/// what was the point of using boxes?
	/// ok it is much faster
#define _NOT_USE_BOX_DRAWING
#ifndef _NOT_USE_BOX_DRAWING
    // we use a box as a line
    n_assert(color.x() >= 0.0f);
    n_assert(color.x() <= 1.0f);
    n_assert(color.y() >= 0.0f);
    n_assert(color.y() <= 1.0f);
    n_assert(color.z() >= 0.0f);
    n_assert(color.z() <= 1.0f);

    const point nFrom(from.x(), from.y(), from.z());
    const point nTo(to.x(), to.y(), to.z());
    const vector dir = nTo - nFrom;
    const point center = nFrom + (dir * 0.5f);
    const float halfWidth = dir.length() * 0.5f;

    // calc scale, the *line* lies in the z-axis
    const matrix44 scale = matrix44::scaling(0.01f, 0.01f, halfWidth);
    
    // calc rotation
    matrix44 rot;
    static const vector axisZ(0.0f, 0.0f, 1.0f);
    const vector dirNormalized = float4::normalize(dir);
    vector rotAxis = float4::cross3(axisZ, dirNormalized);    
    if(0.0f == rotAxis.lengthsq())
    {
        rot = matrix44::identity();
    }
    else
    {
        const float angle = n_acos(float4::dot3(axisZ, dirNormalized));
        rotAxis = float4::normalize(rotAxis);
        rot = matrix44::rotationaxis(rotAxis, angle);
    }

    matrix44 m = scale * rot * matrix44::translation(center);
    DebugShapeRenderer::Instance()->DrawBox(m, float4(color.x(), color.y(), color.z(), 1.0f));
#else
    CoreGraphics::RenderShape::RenderShapeVertex vert;
    vert.pos = Bt2NebPoint(from);
	vert.color = Bt2NebPoint(color);
	this->scene->debugPrimitives.Append(vert);
	vert.pos = Bt2NebPoint(to);
	vert.color = Bt2NebPoint(color);
	this->scene->debugPrimitives.Append(vert);
#endif
}

//------------------------------------------------------------------------------
/**
*/
void	
DebugDrawer::drawContactPoint(const btVector3& PointOnB,const btVector3& normalOnB,btScalar distance,int lifeTime,const btVector3& color)
{
    const matrix44 m = matrix44::multiply(matrix44::scaling(0.02f, 0.02f, 0.02f),
                                          matrix44::translation(PointOnB.x(), PointOnB.y(), PointOnB.z()));
    DebugShapeRenderer::Instance()->DrawBox(m, float4(1.0f, 0.0f, 0.0f, 1.0f));
}

//------------------------------------------------------------------------------
/**
*/
void	
DebugDrawer::reportErrorWarning(const char* warningString)
{
    n_printf("DebugDrawer::reportErrorWarning %s\n", warningString);
}

//------------------------------------------------------------------------------
/**
*/
void	
DebugDrawer::draw3dText(const btVector3& location,const char* textString)
{
    _debug_text3D(Util::String(textString), 
                  point(location.x(), location.y(), location.z()), 
                  float4(1.0f, 0.0f, 0.0f, 1.0f));     
}

//------------------------------------------------------------------------------
/**
*/
void
DebugDrawer::drawSphere(btScalar radius, const btTransform& transform, const btVector3& color)
{
	point ext(radius,radius,radius);
	bbox box(Bt2NebVector(transform.getOrigin()),ext);	

	DebugShapeRenderer::Instance()->DrawSphere(box.to_matrix44(),float4(color.x(),color.y(),color.z(),1),CoreGraphics::RenderShape::Wireframe);
}

//------------------------------------------------------------------------------
/**
*/
void	
DebugDrawer::drawBox(const btVector3& bbMin, const btVector3& bbMax, const btVector3& color)
{
	btVector3 halfExtents = (bbMax-bbMin)* 0.5f;
	btVector3 center = (bbMax+bbMin) *0.5f;
	point bcenter(center.x(),center.y(),center.z());
	bbox box(bcenter,Bt2NebVector(halfExtents));
	DebugShapeRenderer::Instance()->DrawBox(box.to_matrix44(), float4(color.x(),color.y(),color.z(),1),CoreGraphics::RenderShape::Wireframe);
}

//------------------------------------------------------------------------------
/**
*/
void	
DebugDrawer::drawBox(const btVector3& bbMin, const btVector3& bbMax, const btTransform& trans, const btVector3& color)
{	
	bbox box;
	box.pmin = Bt2NebVector(bbMin);
	box.pmax = Bt2NebVector(bbMax);
	matrix44 boxShape = box.to_matrix44();
	boxShape = matrix44::multiply(boxShape, Bt2NebTransform(trans));
	DebugShapeRenderer::Instance()->DrawBox(boxShape, float4(color.x(),color.y(),color.z(),1),CoreGraphics::RenderShape::Wireframe);
}

//------------------------------------------------------------------------------
/**
*/
void
DebugDrawer::drawTriangle(const btVector3& v0, const btVector3& v1, const btVector3& v2, const btVector3& color, btScalar /*alpha*/)
{
    CoreGraphics::RenderShape::RenderShapeVertex vert;
    vert.pos = Bt2NebPoint(v0);
    vert.color = Bt2NebPoint(color);
    this->scene->debugTriangles.Append(vert);
    vert.pos = Bt2NebPoint(v1);    
    this->scene->debugTriangles.Append(vert);
    vert.pos = Bt2NebPoint(v2);
    this->scene->debugTriangles.Append(vert);
}

} // namespace Bullet