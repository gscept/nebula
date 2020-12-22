//------------------------------------------------------------------------------
//  visualdebugger.cc
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "visualdebugger.h"

namespace Physics
{
__ImplementInterfaceSingleton(Physics::VisualDebugger);

//------------------------------------------------------------------------------
/**
*/
VisualDebugger::VisualDebugger():
    simulationFrameTime(-1)
{
    __ConstructInterfaceSingleton;
}

//------------------------------------------------------------------------------
/**
*/
VisualDebugger::~VisualDebugger()
{
    this->scene = 0;

    __DestructInterfaceSingleton;
}

//------------------------------------------------------------------------------
/**
*/
void 
VisualDebugger::OnStep()
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
VisualDebugger::Initialize(Scene* s)
{
    this->scene = s;
}

//------------------------------------------------------------------------------
/**
*/
void 
VisualDebugger::DrawTimedArrow(const Math::point& p, const Math::vector& dir, uint numSteps, const Math::vec4& color /*= Math::float4(1, 1, 1, 1)*/)
{
    this->timedDrawData.Append(TimedDrawData(numSteps, TimedDrawData::Arrow, color, p, dir, "", 0));
}

//------------------------------------------------------------------------------
/**
*/
void 
VisualDebugger::DrawTimedText(const Math::point& p, const Util::String& text, uint numSteps, const Math::vec4& color /*= Math::float4(1, 1, 1, 1)*/)
{
    this->timedDrawData.Append(TimedDrawData(numSteps, TimedDrawData::Text3D, color, p, Math::vector(), text, 0));
}

//------------------------------------------------------------------------------
/**
*/
void 
VisualDebugger::DrawTimedAABB(const Math::bbox& bbox, uint numSteps, const Math::vec4& color /*= Math::float4(1, 1, 1, 1)*/)
{
    this->timedDrawData.Append(TimedDrawData(numSteps, TimedDrawData::AABB, color, bbox.center(), bbox.extents(), "", 0));
}

//------------------------------------------------------------------------------
/**
*/
void 
VisualDebugger::DrawTimedLine(const Math::point& p, const Math::vector& dir, uint numSteps, const Math::vec4& color /*= Math::float4(1, 1, 1, 1)*/)
{
    this->timedDrawData.Append(TimedDrawData(numSteps, TimedDrawData::Line, color, p, dir, "", 0));
}

//------------------------------------------------------------------------------
/**
*/
void 
VisualDebugger::DrawTimedPlane(const Math::point& p, const Math::vector& dir, float scale, uint numSteps, const Math::vec4& color /*= Math::float4(1, 1, 1, 1)*/)
{
    this->timedDrawData.Append(TimedDrawData(numSteps, TimedDrawData::Plane, color, p, dir, "", scale));
}


//------------------------------------------------------------------------------
/**
*/
void 
VisualDebugger::DrawArrow(const Math::point& p, const Math::vector& dir, const Math::vec4& color /*= Math::float4(1, 1, 1, 1)*/)
{
    n_error("VisualDebugger::DrawArrow: Implement in subclass");
}

//------------------------------------------------------------------------------
/**
*/
void 
VisualDebugger::DrawText(const Math::point& p, const Util::String& text, const Math::vec4& color /*= Math::float4(1, 1, 1, 1)*/)
{
    n_error("VisualDebugger::DrawText: Implement in subclass");
}

//------------------------------------------------------------------------------
/**
*/
void 
VisualDebugger::DrawAABB(const Math::bbox& bbox, const Math::vec4& color /*= Math::float4(1, 1, 1, 1)*/)
{
    n_error("VisualDebugger::DrawAABB: Implement in subclass");
}

//------------------------------------------------------------------------------
/**
*/
void 
VisualDebugger::Draw(const TimedDrawData& data)
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
VisualDebugger::SetCameraView(const Math::mat4& viewMatrix)
{
    n_error("VisualDebugger::SetCameraView: Not implemented");
}

//------------------------------------------------------------------------------
/**
*/
void 
VisualDebugger::DrawLine(const Math::point& p, const Math::vector& dir, const Math::vec4& color /*= Math::float4(1, 1, 1, 1)*/)
{
    n_error("VisualDebugger::DrawLine: Not implemented");
}

//------------------------------------------------------------------------------
/**
*/
void 
VisualDebugger::DrawPlane(const Math::point& p, const Math::vector& dir, float scale, const Math::vec4& color /*= Math::float4(1, 1, 1, 1)*/)
{
    n_error("VisualDebugger::DrawPlane: Not implemented");
}

}
