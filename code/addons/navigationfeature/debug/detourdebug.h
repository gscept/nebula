#include "DebugDraw.h"
#include "util/array.h"
#include "math/vector.h"
#include "coregraphics/rendershape.h"


namespace Navigation
{

struct DebugDraw : public duDebugDraw
{
DebugDraw();
virtual ~DebugDraw(){}

virtual void depthMask(bool state);

virtual void texture(bool state){}

/// Begin drawing primitives.
///  @param prim [in] primitive type to draw, one of rcDebugDrawPrimitives.
///  @param size [in] size of a primitive, applies to point size and line width only.
virtual void begin(duDebugDrawPrimitives prim, float size = 1.0f) ;

/// Submit a vertex
///  @param pos [in] position of the verts.
///  @param color [in] color of the verts.
virtual void vertex(const float* pos, unsigned int color) ;

/// Submit a vertex
///  @param x,y,z [in] position of the verts.
///  @param color [in] color of the verts.
virtual void vertex(const float x, const float y, const float z, unsigned int color) ;

/// Submit a vertex
///  @param pos [in] position of the verts.
///  @param color [in] color of the verts.
virtual void vertex(const float* pos, unsigned int color, const float* uv) ;

/// Submit a vertex
///  @param x,y,z [in] position of the verts.
///  @param color [in] color of the verts.
virtual void vertex(const float x, const float y, const float z, unsigned int color, const float u, const float v) ;

/// End drawing primitives.
virtual void end() ;

private:
	duDebugDrawPrimitives current;
	Util::Array<CoreGraphics::RenderShape::RenderShapeVertex> points;
	CoreGraphics::RenderShape::RenderFlag flag;

};

}
