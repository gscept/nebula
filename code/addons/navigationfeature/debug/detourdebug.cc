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

DebugDraw::DebugDraw() : flag(CoreGraphics::RenderShape::Wireframe)
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
            //CoreGraphics::RenderShape shape;
            //shape.SetupPrimitives(Math::mat4::identity, CoreGraphics::PrimitiveTopology::PointList, (SizeT)points.size(), points.begin(), vec4(1.0f), this->flag);
            //CoreGraphics::ShapeRenderer::Instance()->AddShape(shape);
            for (auto const& point : points)
            {
                Im3d::Im3dContext::DrawPoint(point.pos.vec, 10.0f, point.color);
            }
		}
		break;
		case DU_DRAW_LINES:
		{
            //CoreGraphics::RenderShape shape;
            //shape.SetupPrimitives(Math::mat4::identity, CoreGraphics::PrimitiveTopology::LineList, (SizeT)points.size() / 2, points.begin(), vec4(1.0f), this->flag);
            //CoreGraphics::ShapeRenderer::Instance()->AddShape(shape);
            for (int i = 0, k = points.Size()/2;i<k;++i)
            {
                Im3d::Im3dContext::DrawLine(Math::line(points[2 * i].pos, points[2 * i + 1].pos), 1.0f, points[2 * i].color);
            }
		}
		break;
		case DU_DRAW_TRIS:
        {
            static bool foo = true;
            if (foo)
            {
                CoreGraphics::RenderShape shape;
                shape.SetupPrimitives(Math::mat4::identity, CoreGraphics::PrimitiveTopology::TriangleList, (SizeT)points.Size() / 3, &points[0], vec4(1.0f), CoreGraphics::RenderShape::Wireframe);// this->flag);
                CoreGraphics::ShapeRenderer::Instance()->AddShape(shape);
            }
            else
            {
            for (int i = 0, k = points.Size() / 3; i < k; ++i)
            {
                Im3d::Im3dContext::DrawLine(Math::line(points[3 * i].pos, points[3 * i + 1].pos), 5.0f, points[3 * i].color);
                Im3d::Im3dContext::DrawLine(Math::line(points[3 * i + 1].pos, points[3 * i + 2].pos), 5.0f, points[3 * i + 1].color);
                Im3d::Im3dContext::DrawLine(Math::line(points[3 * i + 2].pos, points[3 * i].pos), 5.0f, points[3 * i + 2].color);
            }
            }
		}
		break;
		case DU_DRAW_QUADS:
		{
			n_assert(false);
		}
		break;			
	}

}


}
