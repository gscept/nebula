#pragma once
//------------------------------------------------------------------------------
/**
    @class Physics::BaseVisualDebuggerServer
    
    Base server for class that communicates with the physics visual debugger 
	program. Holds utility methods to easily display stuff such as arrow and 
	text in the debugger.

	Note: Make sure to place continuous calls of draw-methods into 
	OnStepBefore() of physicsclasses, instead of RenderDebug for example.
    
    (C) 2013-2015 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------

#include "core/refcounted.h"
#include "core/ptr.h"
#include "core/singleton.h"
#include "math/bbox.h"

namespace Physics
{
class Scene;

class BaseVisualDebuggerServer : public Core::RefCounted
{
	__DeclareClass(BaseVisualDebuggerServer);
	__DeclareInterfaceSingleton(BaseVisualDebuggerServer);
public:
	/// constructor
	BaseVisualDebuggerServer();
	/// destructor
	virtual ~BaseVisualDebuggerServer();

	/// do one step
	virtual void OnStep();
	/// initialize debugger connection
	virtual void Initialize(Scene* s);

	/// set how much time that should pass each step
	void SetSimulationFrameTime(float time);
	/// get how much time that should pass each step
	float SetSimulationFrameTime() const;


	/// draw an arrow during a number of steps
	void DrawTimedArrow(const Math::point& p, const Math::vector& dir, uint numSteps, const Math::float4& color = Math::float4(1, 1, 1, 1));
	/// draw a line during a number of steps
	void DrawTimedLine(const Math::point& p, const Math::vector& dir, uint numSteps, const Math::float4& color = Math::float4(1, 1, 1, 1));
	/// draw a plane during a number of steps
	void DrawTimedPlane(const Math::point& p, const Math::vector& dir, float scale, uint numSteps, const Math::float4& color = Math::float4(1, 1, 1, 1));
	/// draw text during a number of steps
	void DrawTimedText(const Math::point& p, const Util::String& text, uint numSteps, const Math::float4& color = Math::float4(1, 1, 1, 1));
	/// draw an AABB during a number of steps
	void DrawTimedAABB(const Math::bbox& bbox, uint numSteps, const Math::float4& color = Math::float4(1, 1, 1, 1));

	/// set the camera view
	virtual void SetCameraView(const Math::matrix44& viewMatrix);

	/*		=== abstract draw-methods ===		*/

	/// draw an arrow
	virtual void DrawArrow(const Math::point& p, const Math::vector& dir, const Math::float4& color = Math::float4(1, 1, 1, 1));
	/// draw a line
	virtual void DrawLine(const Math::point& p, const Math::vector& dir, const Math::float4& color = Math::float4(1, 1, 1, 1));
	/// draw a plane
	virtual void DrawPlane(const Math::point& p, const Math::vector& dir, float scale, const Math::float4& color = Math::float4(1, 1, 1, 1));
	/// draw text
	virtual void DrawText(const Math::point& p, const Util::String& text, const Math::float4& color = Math::float4(1, 1, 1, 1));
	/// draw an AABB
	virtual void DrawAABB(const Math::bbox& bbox, const Math::float4& color = Math::float4(1, 1, 1, 1));

protected:

	class TimedDrawData;

	/// perform drawing
	void Draw(const TimedDrawData& data);

	/// base class for draw-calls that should be done over a number of steps
	class TimedDrawData
	{
	public:
		enum DrawType
		{
			Unitialized = -1,
			Arrow = 0,
			Text3D,
			AABB,
			Line,
			Plane
		};

		/// default constructor
		TimedDrawData():
		  type(Unitialized)
		{ }
		
		/// constructor
		TimedDrawData(uint numSteps, DrawType t, const Math::float4& c, const Math::vector& vec1, const Math::vector& vec2, const Util::String& txt, float _f):
		  stepsLeft(numSteps),
		  type(t),
		  color(c),
		  v1(vec1),
		  v2(vec2),
		  text(txt),
		  f(_f)
		{ }

		uint stepsLeft;
		DrawType type;
		Math::float4 color;
		/// these below are not always used
		Math::vector v1, v2;
		float f;
		Util::String text;
	};

	Scene* scene;
	float simulationFrameTime;
	Util::Array<TimedDrawData> timedDrawData;
}; 

//------------------------------------------------------------------------------
/**
*/
inline void 
BaseVisualDebuggerServer::SetSimulationFrameTime(float time)
{
	this->simulationFrameTime = time;
}

//------------------------------------------------------------------------------
/**
*/
inline float 
BaseVisualDebuggerServer::SetSimulationFrameTime() const
{
	return this->simulationFrameTime;
}

} 
// namespace Physics
//------------------------------------------------------------------------------