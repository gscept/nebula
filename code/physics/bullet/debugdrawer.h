#pragma once
//------------------------------------------------------------------------------
/**
    @class Bullet::DebugDrawer
	
	drawing functions used by bullet do visualize internal bullet objects
	like colliders joints, etc

    (C) 2006 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/

#include "LinearMath/btIDebugDraw.h"

namespace Bullet
{

class BulletScene;
class DebugDrawer : public btIDebugDraw
{
public:

	DebugDrawer();

	virtual ~DebugDrawer();

	///
	virtual void	drawLine(const btVector3& from,const btVector3& to,const btVector3& color);
	///
	virtual void	drawSphere(btScalar radius, const btTransform& transform, const btVector3& color);
	///
	virtual void	drawBox(const btVector3& bbMin, const btVector3& bbMax, const btVector3& color);
	///
	virtual void	drawBox(const btVector3& bbMin, const btVector3& bbMax, const btTransform& trans, const btVector3& color);
    ///
    virtual	void	drawTriangle(const btVector3& v0, const btVector3& v1, const btVector3& v2, const btVector3& color, btScalar /*alpha*/);
	///
	virtual void	drawContactPoint(const btVector3& PointOnB,const btVector3& normalOnB,btScalar distance,int lifeTime,const btVector3& color);
	///
	virtual void	reportErrorWarning(const char* warningString);
	///
	virtual void	draw3dText(const btVector3& location,const char* textString);	
	/// configure which type of debug information is to be rendered
	virtual void	setDebugMode(int debugMode);	
	///
	virtual int		getDebugMode() const;

	/// set the scene from which this debug drawer was created
	void SetScene(BulletScene* scene);

private:
	BulletScene* scene;
    int m_debugMode;
};

//------------------------------------------------------------------------------
/**
*/
inline void	
DebugDrawer::setDebugMode(int debugMode)
{
    this->m_debugMode = debugMode;
}

//------------------------------------------------------------------------------
/**
*/
inline int		
DebugDrawer::getDebugMode() const
{
    return this->m_debugMode;
}

//------------------------------------------------------------------------------
/**
*/
inline void
DebugDrawer::SetScene(BulletScene* scene)
{
	this->scene = scene;
}

} // namespace Bullet