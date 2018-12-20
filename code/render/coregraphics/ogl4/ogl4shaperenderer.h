#pragma once
//------------------------------------------------------------------------------
/**
    @class OpenGL4::OGL4ShapeRenderer
  
    OGL4 implementation of ShapeRenderer.
    
    (C) 2007 Radon Labs GmbH
	(C) 2013-2018 Individual contributors, see AUTHORS file
*/    
#include "coregraphics/base/shaperendererbase.h"
#include "coregraphics/vertexlayout.h"
#include "coregraphics/vertexbuffer.h"
#include "coregraphics/indexbuffer.h"
#include "coregraphics/bufferlock.h"
#include "resources/managedmesh.h"
#include "util/fixedarray.h"

//------------------------------------------------------------------------------
namespace OpenGL4
{
class OGL4ShapeRenderer : public Base::ShapeRendererBase
{
    __DeclareClass(OGL4ShapeRenderer);
    __DeclareSingleton(OGL4ShapeRenderer);
public:
    /// constructor
    OGL4ShapeRenderer();
    /// destructor
    virtual ~OGL4ShapeRenderer();
    
    /// open the shape renderer
    void Open();
    /// close the shape renderer
    void Close();

    /// draw attached shapes and clear deferred stack, must be called inside render loop
    void DrawShapes();

	/// maximum amount of vertices to be rendered by drawprimitives and drawindexedprimitives
    static const int MaxNumVertices = 262140;
	/// maximum amount of indices to be rendered by drawprimitives and drawindexedprimitives
    static const int MaxNumIndices = 262140;

    /// maximum size for primitive size (4 floats for position, 4 floats for color)
    static const int MaxVertexWidth = 8 * sizeof(float);
    /// maximum size for an index
    static const int MaxIndexWidth = sizeof(int);
private:

    /// draw buffered primitives
    void DrawBufferedPrimitives();
    /// draw buffered indexed primtives
    void DrawBufferedIndexedPrimitives();

    /// draw a shape
    void DrawSimpleShape(const Math::matrix44& modelTransform, CoreGraphics::RenderShape::Type shapeType, const Math::float4& color);
	/// draw debug mesh
	void DrawMesh(const Math::matrix44& modelTransform, const Ptr<CoreGraphics::Mesh>& mesh, const IndexT& groupIndex, const Math::float4& color);
    /// draw primitives
    void DrawPrimitives(const Math::matrix44& modelTransform, CoreGraphics::PrimitiveTopology::Code topology, SizeT numPrimitives, const void* vertices, SizeT vertexWidth, const Math::float4& color);
    /// draw indexed primitives
    void DrawIndexedPrimitives(const Math::matrix44& modelTransform, CoreGraphics::PrimitiveTopology::Code topology, SizeT numPrimitives, const void* vertices, SizeT numVertices, SizeT vertexWidth, const void* indices, CoreGraphics::IndexType::Code indexType, const Math::float4& color);

	/// create a box shape
	void CreateBoxShape();
	/// create a sphere shape
	void CreateSphereShape();
	/// create a cylinder shape
	void CreateCylinderShape();
	/// create a torus shape
    void CreateTorusShape();
	/// create a cone shape
	void CreateConeShape();

	uint featureBits[CoreGraphics::RenderShape::NumDepthFlags * 2];

	Util::FixedArray<Ptr<Resources::ManagedMesh> > shapeMeshes;
    Ptr<CoreGraphics::Shader> shapeShader;
	CoreGraphics::PrimitiveGroup primGroup;
	
	Ptr<CoreGraphics::VertexBuffer> vbo;
	Ptr<CoreGraphics::IndexBuffer> ibo;
    Ptr<CoreGraphics::VertexLayout> vertexLayout;
	Ptr<CoreGraphics::BufferLock> vboLock;
	Ptr<CoreGraphics::BufferLock> iboLock;
	Ptr<CoreGraphics::ShaderVariable> model;
	Ptr<CoreGraphics::ShaderVariable> viewProjection;
    Ptr<CoreGraphics::ShaderVariable> diffuseColor;

    SizeT numPrimitives;
    SizeT numIndices;

    struct IndexedDraws
    {
        Util::Array<CoreGraphics::PrimitiveGroup> primitives;
        Util::Array<Math::float4> colors;
        Util::Array<Math::matrix44> transforms;
    } indexed;

    struct UnindexedDraws
    {
        Util::Array<CoreGraphics::PrimitiveGroup> primitives;
        Util::Array<Math::float4> colors;
        Util::Array<Math::matrix44> transforms;
    } unindexed;
    
    static const GLuint NumBuffers = 12;
    byte* vertexBufferPtr;
    byte* indexBufferPtr;
    GLuint vbBufferIndex;
	GLuint ibBufferIndex;
};

} // namespace OpenGL4
//------------------------------------------------------------------------------
