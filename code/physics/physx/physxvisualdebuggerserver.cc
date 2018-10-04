//------------------------------------------------------------------------------
//  basevisualdebuggerserver.cc
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "basevisualdebuggerserver.h"

namespace Physics
{
__ImplementAbstractClass(Physics::BaseVisualDebuggerServer, 'BVDS', Core::RefCounted);
__ImplementInterfaceSingleton(Physics::BaseVisualDebuggerServer);

//------------------------------------------------------------------------------
/**
*/
BaseVisualDebuggerServer::BaseVisualDebuggerServer():
	simulationFrameTime(-1)
{
    __ConstructInterfaceSingleton;
}

//------------------------------------------------------------------------------
/**
*/
BaseVisualDebuggerServer::~BaseVisualDebuggerServer()
{
	this->scene = 0;

	__DestructInterfaceSingleton;
}

//------------------------------------------------------------------------------
/**
*/
void 
BaseVisualDebuggerServer::OnStep()
{
	IndexT i;
	for (i = 0; i < this->timedDrawData.Size() ; i++)
	{
		TimedDrawData& data = this->timedDrawData[i];
		this->Draw(data);

		data.stepsLeft--;
		if (data.stepsLeft <= 0)
		{
			this->timedDrawData.EraseIndex(i);
			i--;
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
BaseVisualDebuggerServer::Initialize(Scene* s)
{
	this->scene = s;
}

//------------------------------------------------------------------------------
/**
*/
void 
BaseVisualDebuggerServer::DrawTimedArrow(const Math::point& p, const Math::vector& dir, uint numSteps, const Math::float4& color /*= Math::float4(1, 1, 1, 1)*/)
{
	this->timedDrawData.Append(TimedDrawData(numSteps, TimedDrawData::Arrow, color, p, dir, "", 0));
}

//------------------------------------------------------------------------------
/**
*/
void 
BaseVisualDebuggerServer::DrawTimedText(const Math::point& p, const Util::String& text, uint numSteps, const Math::float4& color /*= Math::float4(1, 1, 1, 1)*/)
{
	this->timedDrawData.Append(TimedDrawData(numSteps, TimedDrawData::Text3D, color, p, Math::vector(), text, 0));
}

//------------------------------------------------------------------------------
/**
*/
void 
BaseVisualDebuggerServer::DrawTimedAABB(const Math::bbox& bbox, uint numSteps, const Math::float4& color /*= Math::float4(1, 1, 1, 1)*/)
{
	this->timedDrawData.Append(TimedDrawData(numSteps, TimedDrawData::AABB, color, bbox.center(), bbox.extents(), "", 0));
}

//------------------------------------------------------------------------------
/**
*/
void 
BaseVisualDebuggerServer::DrawTimedLine(const Math::point& p, const Math::vector& dir, uint numSteps, const Math::float4& color /*= Math::float4(1, 1, 1, 1)*/)
{
	this->timedDrawData.Append(TimedDrawData(numSteps, TimedDrawData::Line, color, p, dir, "", 0));
}

//------------------------------------------------------------------------------
/**
*/
void 
BaseVisualDebuggerServer::DrawTimedPlane(const Math::point& p, const Math::vector& dir, float scale, uint numSteps, const Math::float4& color /*= Math::float4(1, 1, 1, 1)*/)
{
	this->timedDrawData.Append(TimedDrawData(numSteps, TimedDrawData::Plane, color, p, dir, "", scale));
}


//------------------------------------------------------------------------------
/**
*/
void 
BaseVisualDebuggerServer::DrawArrow(const Math::point& p, const Math::vector& dir, const Math::float4& color /*= Math::float4(1, 1, 1, 1)*/)
{
	n_error("BaseVisualDebuggerServer::DrawArrow: Implement in subclass");
}

//------------------------------------------------------------------------------
/**
*/
void 
BaseVisualDebuggerServer::DrawText(const Math::point& p, const Util::String& text, const Math::float4& color /*= Math::float4(1, 1, 1, 1)*/)
{
	n_error("BaseVisualDebuggerServer::DrawText: Implement in subclass");
}

//------------------------------------------------------------------------------
/**
*/
void 
BaseVisualDebuggerServer::DrawAABB(const Math::bbox& bbox, const Math::float4& color /*= Math::float4(1, 1, 1, 1)*/)
{
	n_error("BaseVisualDebuggerServer::DrawAABB: Implement in subclass");
}

//------------------------------------------------------------------------------
/**
*/
void 
BaseVisualDebuggerServer::Draw(const TimedDrawData& data)
{
	switch (data.type)
	{
	case TimedDrawData::Arrow:
		this->DrawArrow(data.v1, data.v2, data.color);
		break;
	case TimedDrawData::Text3D:
		this->DrawText(data.v1, data.text, data.color);
		break;
	case TimedDrawData::AABB:
		this->DrawAABB(Math::bbox(data.v1, data.v2), data.color);
		break;
	case TimedDrawData::Line:
		this->DrawLine(data.v1, data.v2, data.color);
		break;
	case TimedDrawData::Plane:
		this->DrawPlane(data.v1, data.v2, data.f, data.color);
		break;
	default:
		n_error("Not implemented to draw TimeDrawData::DrawType '%d'", data.type);
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
BaseVisualDebuggerServer::SetCameraView(const Math::matrix44& viewMatrix)
{
	n_error("BaseVisualDebuggerServer::SetCameraView: Not implemented");
}

//------------------------------------------------------------------------------
/**
*/
void 
BaseVisualDebuggerServer::DrawLine(const Math::point& p, const Math::vector& dir, const Math::float4& color /*= Math::float4(1, 1, 1, 1)*/)
{
	n_error("BaseVisualDebuggerServer::DrawLine: Not implemented");
}

//------------------------------------------------------------------------------
/**
*/
void 
BaseVisualDebuggerServer::DrawPlane(const Math::point& p, const Math::vector& dir, float scale, const Math::float4& color /*= Math::float4(1, 1, 1, 1)*/)
{
	n_error("BaseVisualDebuggerServer::DrawPlane: Not implemented");
}

}