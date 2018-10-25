#pragma once
//------------------------------------------------------------------------------
/**
    @class Direct3D11::D3D11ShapeRenderer
  
    D3D11/Xbox360 implementation of ShapeRenderer.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/    
//------------------------------------------------------------------------------
#include "coregraphics/base/shaperendererbase.h"
#include "coregraphics/vertexlayout.h"
#include "resources/managedmesh.h"
#include "util/fixedarray.h"
#include "../shaderfeature.h"

namespace Direct3D11
{
class D3D11ShapeRenderer : public Base::ShapeRendererBase
{
    __DeclareClass(D3D11ShapeRenderer);
    __DeclareSingleton(D3D11ShapeRenderer);
public:
    /// constructor
    D3D11ShapeRenderer();
    /// destructor
    virtual ~D3D11ShapeRenderer();
    
    /// open the shape renderer
    void Open();
    /// close the shape renderer
    void Close();

    /// draw attached shapes and clear deferred stack, must be called inside render loop
    void DrawShapes();

	/// maximum amount of vertices to be rendered by drawprimitives and drawindexedprimitives
	static const int MaxNumVertices = 4096;
	/// maximum amount of indices to be rendered by drawprimitives and drawindexedprimitives
	static const int MaxNumIndices = 4096;
private:

    /// draw a shape
    void DrawSimpleShape(const Math::matrix44& modelTransform, CoreGraphics::RenderShape::Type shapeType, const Math::float4& color);
    /// draw primitives
    void DrawPrimitives(const Math::matrix44& modelTransform, CoreGraphics::PrimitiveTopology::Code topology, SizeT numPrimitives, const void* vertices, SizeT vertexWidth, const Math::float4& color);
    /// draw indexed primitives
    void DrawIndexedPrimitives(const Math::matrix44& modelTransform, CoreGraphics::PrimitiveTopology::Code topology, SizeT numPrimitives, const void* vertices, SizeT numVertices, SizeT vertexWidth, const void* indices, CoreGraphics::IndexType::Code indexType, const Math::float4& color);
	/// draw mesh
	void DrawMesh(const Math::matrix44& modelTransform, const Ptr<CoreGraphics::Mesh>& mesh, const Math::float4& color);

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

	CoreGraphics::ShaderFeature::Mask depthFeatureBits[CoreGraphics::RenderShape::NumDepthFlags];

	Ptr<CoreGraphics::VertexLayout> vertexLayout;
    Ptr<CoreGraphics::ShaderInstance> shapeShader;
	Util::FixedArray<Ptr<Resources::ManagedMesh> > shapeMeshes;
    Ptr<CoreGraphics::ShaderVariable> model;
	Ptr<CoreGraphics::ShaderVariable> viewProjection;
    Ptr<CoreGraphics::ShaderVariable> diffuseColor;

	ID3D11Buffer* vertexBuffer;
	ID3D11Buffer* indexBuffer;
};

} // namespace Direct3D11
//------------------------------------------------------------------------------
