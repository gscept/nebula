#pragma once
//------------------------------------------------------------------------------
/**
    @class OpenGL4::OGL4ParticleRenderer

    Particle system renderer for OpenGL4. The renderer makes use of
    hardware instancing to prevent writing redundant data to
    dynamic vertex buffers.

    See here for details:
    http://zeuxcg.blogspot.com/2007/09/particle-rendering-revisited.html

    (C) 2008 Radon Labs GmbH
	(C) 2013-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "particles/base/particlerendererbase.h"
#include "coregraphics/vertexbuffer.h"
#include "coregraphics/indexbuffer.h"
#include "coregraphics/vertexlayout.h"


namespace OpenGL4
{
class OGL4ParticleRenderer : public Base::ParticleRendererBase
{
    __DeclareClass(OGL4ParticleRenderer);
    __DeclareSingleton(OGL4ParticleRenderer);
public:
    /// constructor
    OGL4ParticleRenderer();
    /// destructor
    virtual ~OGL4ParticleRenderer();

    /// setup the particle renderer
    virtual void Setup();
    /// discard the particle renderer
    virtual void Discard();

    /// begin attaching visible particle systems
    virtual void BeginAttach();
    /// finish attaching visible particle systems
    virtual void EndAttach();

    /// get the current vertex index
    IndexT GetCurParticleVertexIndex() const;
	/// get the current particle index
	IndexT GetCurParticleIndex() const;
    /// add particle vertex index
    void AddCurParticleVertexIndex(IndexT add);
	/// add particle index
	void AddCurParticleIndex(IndexT add);

    /// get the current vertex pointer
    void* GetCurVertexPtr();
    /// set the current vertex pointer
    void SetCurVertexPtr(void *ptr);
    /// get particle vertex buffer
    const Ptr<CoreGraphics::VertexBuffer>& GetParticleVertexBuffer() const;
    /// get the corner vertex buffer
    const Ptr<CoreGraphics::VertexBuffer>& GetCornerVertexBuffer() const;
    /// get the corner index buffer
    const Ptr<CoreGraphics::IndexBuffer>& GetCornerIndexBuffer() const;
    /// get the primitive group
    CoreGraphics::PrimitiveGroup& GetPrimitiveGroup();
    /// get the vertex layout
    const Ptr<CoreGraphics::VertexLayout>& GetVertexLayout() const;
	/// get buffer lock
	const Ptr<CoreGraphics::BufferLock>& GetParticleBufferLock() const;
private:

	Ptr<CoreGraphics::BufferLock> particleBufferLock;
    Ptr<CoreGraphics::VertexBuffer> particleVertexBuffer;
    Ptr<CoreGraphics::VertexBuffer> cornerVertexBuffer;
    Ptr<CoreGraphics::IndexBuffer> cornerIndexBuffer;
    CoreGraphics::PrimitiveGroup primGroup;
    Ptr<CoreGraphics::VertexLayout> vertexLayout;
    IndexT curParticleVertexIndex;
	IndexT curParticleIndex;
    void* mappedVertices;
    void* curVertexPtr;
	IndexT bufferIndex;
};

//------------------------------------------------------------------------------
/**
*/
inline IndexT 
OGL4ParticleRenderer::GetCurParticleVertexIndex() const
{
    return this->curParticleVertexIndex;
}

//------------------------------------------------------------------------------
/**
*/
inline IndexT
OGL4ParticleRenderer::GetCurParticleIndex() const
{
	return this->curParticleIndex;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
OGL4ParticleRenderer::AddCurParticleVertexIndex(IndexT add)
{
    n_assert(add > 0);
    this->curParticleVertexIndex += add;
}

//------------------------------------------------------------------------------
/**
*/
inline void
OGL4ParticleRenderer::AddCurParticleIndex(IndexT add)
{
	n_assert(add > 0);
	this->curParticleIndex += add;
}

//------------------------------------------------------------------------------
/**
*/
inline void* 
OGL4ParticleRenderer::GetCurVertexPtr()
{
    return this->curVertexPtr;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
OGL4ParticleRenderer::SetCurVertexPtr(void *ptr)
{
    this->curVertexPtr = ptr;
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<CoreGraphics::VertexBuffer>&
OGL4ParticleRenderer::GetParticleVertexBuffer() const
{
    return this->particleVertexBuffer;
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<CoreGraphics::VertexBuffer>&
OGL4ParticleRenderer::GetCornerVertexBuffer() const
{
    return this->cornerVertexBuffer;
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<CoreGraphics::IndexBuffer>& 
OGL4ParticleRenderer::GetCornerIndexBuffer() const
{
    return this->cornerIndexBuffer;
}

//------------------------------------------------------------------------------
/**
*/
inline CoreGraphics::PrimitiveGroup& 
OGL4ParticleRenderer::GetPrimitiveGroup()
{
    return this->primGroup;
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<CoreGraphics::VertexLayout>& 
OGL4ParticleRenderer::GetVertexLayout() const
{
    return this->vertexLayout;
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<CoreGraphics::BufferLock>&
OGL4ParticleRenderer::GetParticleBufferLock() const
{
	return this->particleBufferLock;
}

} // namespace OpenGL4
//------------------------------------------------------------------------------

