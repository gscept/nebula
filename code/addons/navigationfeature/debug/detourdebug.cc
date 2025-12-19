#include "foundation/stdneb.h"
#include "detourdebug.h"
#include "math/mat4.h"
#include "coregraphics/shaperenderer.h"
#include "dynui/im3d/im3dcontext.h"

using namespace Math;
namespace Navigation
{

//------------------------------------------------------------------------------
/**
*/

DebugDraw::DebugDraw() : current(duDebugDrawPrimitives::DU_DRAW_POINTS), flag(CoreGraphics::RenderShape::Wireframe)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
DebugDraw::depthMask(bool state)
{
	if (state)
	{
		this->flag = CoreGraphics::RenderShape::CheckDepth;
	}
	else
	{
		this->flag = CoreGraphics::RenderShape::AlwaysOnTop;
	}
}

void
DebugDraw::begin(duDebugDrawPrimitives prim, float size)
{
	points.Clear();
	current =  prim;
}

void 
DebugDraw::vertex(const float* pos, unsigned int color)
{
    unsigned char * c = (unsigned char*)(&color);
	CoreGraphics::RenderShape::RenderShapeVertex vert;
    vert.pos = vec4(pos[0], pos[1], pos[2], 1.0f);
    vert.color = vec4(c[0] / 255.0f, c[1] / 255.0f, c[2] / 255.0f, c[3] / 255.0f);
	points.Append(vert);
}

void 
DebugDraw::vertex(const float x, const float y, const float z, unsigned int color)
{
    unsigned char * c = (unsigned char*)(&color);
    CoreGraphics::RenderShape::RenderShapeVertex vert;
	vert.pos = vec4(x, y, z, 1.0f);
	vert.color = vec4(c[0] / 255.0f, c[1] / 255.0f, c[2] / 255.0f, c[3] / 255.0f);
    points.Append(vert);
}

void
DebugDraw::vertex(const float* pos, unsigned int color, const float* uv)
{
    unsigned char * c = (unsigned char*)(&color);	
    CoreGraphics::RenderShape::RenderShapeVertex vert;
	vert.pos = vec4(pos[0], pos[1], pos[2], 1.0f);
	vert.color = vec4(c[0] / 255.0f, c[1] / 255.0f, c[2] / 255.0f, c[3] / 255.0f);
    points.Append(vert);
}

void 
DebugDraw::vertex(const float x, const float y, const float z, unsigned int color, const float u, const float v)
{
    unsigned char * c = (unsigned char*)(&color);	
    CoreGraphics::RenderShape::RenderShapeVertex vert;
	vert.pos = vec4(x, y, z, 1.0f);
	vert.color = vec4(c[0] / 255.0f, c[1] / 255.0f, c[2] / 255.0f, c[3] / 255.0f);
	points.Append(vert);
}

#define USE_IM3D_FOR_DEBUGDRAW 1
void 
DebugDraw::end()
{
	if(points.Size() == 0)
	{
		return;
	}	
	switch(current)
	{
		case DU_DRAW_POINTS:
		{
#ifndef USE_IM3D_FOR_DEBUGDRAW
            CoreGraphics::RenderShape shape;
			shape.SetupPrimitives(points, CoreGraphics::PrimitiveTopology::PointList, this->flag);
            CoreGraphics::ShapeRenderer::Instance()->AddShape(shape);
#else
			for (int i = 0, k = points.Size(); i < k; i++)
			{
				auto col = points[i].color;
                Im3d::Im3dContext::DrawPoint(points[i].pos, 15.0f, col, this->flag);
			}
#endif
		}
		break;
		case DU_DRAW_LINES:
		{
#ifndef USE_IM3D_FOR_DEBUGDRAW
            CoreGraphics::RenderShape shape;
            shape.SetupPrimitives(points, CoreGraphics::PrimitiveTopology::LineList, this->flag);
            CoreGraphics::ShapeRenderer::Instance()->AddShape(shape);
#else
            for (int i = 0, k = points.Size(); i<k; i+=2)
			{
		        auto col = points[i].color;
		        Math::line dline(points[i].pos, points[i + 1].pos);
		        Im3d::Im3dContext::DrawLine(dline, 5.0f, col, Im3d::RenderFlag::AlwaysOnTop);
            }
#endif       
		}
		break;
		case DU_DRAW_TRIS:
        {
#ifdef USE_IM3D_FOR_DEBUGDRAW
			for (int i = 0, k = points.Size(); i < k; i+=3)
            {
                    auto col = points[i].color;
                    Math::line line1(points[i].pos, points[i + 1].pos);
                    Math::line line2(points[i+1].pos, points[i + 2].pos);
                    Math::line line3(points[i+2].pos, points[i].pos);
                    Im3d::Im3dContext::DrawLine(line1, 5.0f, col, Im3d::RenderFlag::AlwaysOnTop);
                    Im3d::Im3dContext::DrawLine(line2, 5.0f, col, Im3d::RenderFlag::AlwaysOnTop);
                    Im3d::Im3dContext::DrawLine(line3, 5.0f, col, Im3d::RenderFlag::AlwaysOnTop);
            }
#else
            CoreGraphics::RenderShape shape;
            shape.SetupPrimitives(points, CoreGraphics::PrimitiveTopology::TriangleList, CoreGraphics::RenderShape::Wireframe);// this->flag);
            CoreGraphics::ShapeRenderer::Instance()->AddShape(shape);
#endif
		}
		break;
		case DU_DRAW_QUADS:
		{
#ifdef USE_IM3D_FOR_DEBUGDRAW
			for (int i = 0, k = points.Size(); i < k; i += 4)
			{
				auto col = points[i].color;
				Im3d::Im3dContext::DrawQuad(points[i].pos, points[i + 1].pos, points[i + 2].pos, points[i + 3].pos, col, this->flag);
			}
#endif
		}
		break;			
	}

}


}
