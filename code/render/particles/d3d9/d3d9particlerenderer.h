#pragma once
//------------------------------------------------------------------------------
/**
    @class Direct3D9::D3D9ParticleRenderer

    Particle system renderer for D3D9/Xbox360. The renderer makes use of
    hardware instancing to prevent writing redundant data to
    dynamic vertex buffers.

    See here for details:
    http://zeuxcg.blogspot.com/2007/09/particle-rendering-revisited.html

    (C) 2008 Radon Labs GmbH
*/
#include "particles/base/particlerendererbase.h"
#include "coregraphics/vertexbuffer.h"
#include "coregraphics/indexbuffer.h"
#include "coregraphics/vertexlayout.h"

//------------------------------------------------------------------------------
namespace Direct3D9
{
class D3D9ParticleRenderer : public Base::ParticleRendererBase
{
    __DeclareClass(D3D9ParticleRenderer);
    __DeclareSingleton(D3D9ParticleRenderer);
public:
    /// constructor
    D3D9ParticleRenderer();
    /// destructor
    virtual ~D3D9ParticleRenderer();

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
    /// add particle vertex index
    void AddCurParticleVertexIndex(IndexT add);

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

private:

    Ptr<CoreGraphics::VertexBuffer> particleVertexBuffer;
    Ptr<CoreGraphics::VertexBuffer> cornerVertexBuffer;
    Ptr<CoreGraphics::IndexBuffer> cornerIndexBuffer;
    CoreGraphics::PrimitiveGroup primGroup;
    Ptr<CoreGraphics::VertexLayout> vertexLayout;
    IndexT curParticleVertexIndex;
    void* mappedVertices;
    void* curVertexPtr;
};

//------------------------------------------------------------------------------
/**
*/
inline
IndexT 
D3D9ParticleRenderer::GetCurParticleVertexIndex() const
{
    return this->curParticleVertexIndex;
}

//------------------------------------------------------------------------------
/**
*/
inline
void 
D3D9ParticleRenderer::AddCurParticleVertexIndex(IndexT add)
{
    n_assert(add > 0);
    this->curParticleVertexIndex += add;
}

//------------------------------------------------------------------------------
/**
*/
inline
void* 
D3D9ParticleRenderer::GetCurVertexPtr()
{
    return this->curVertexPtr;
}

//------------------------------------------------------------------------------
/**
*/
inline
void 
D3D9ParticleRenderer::SetCurVertexPtr(void *ptr)
{
    this->curVertexPtr = ptr;
}

//------------------------------------------------------------------------------
/**
*/
inline
const Ptr<CoreGraphics::VertexBuffer>&
D3D9ParticleRenderer::GetParticleVertexBuffer() const
{
    return this->particleVertexBuffer;
}

//------------------------------------------------------------------------------
/**
*/
inline
const Ptr<CoreGraphics::VertexBuffer>&
D3D9ParticleRenderer::GetCornerVertexBuffer() const
{
    return this->cornerVertexBuffer;
}

//------------------------------------------------------------------------------
/**
*/
inline
const Ptr<CoreGraphics::IndexBuffer>& 
D3D9ParticleRenderer::GetCornerIndexBuffer() const
{
    return this->cornerIndexBuffer;
}

//------------------------------------------------------------------------------
/**
*/
inline
CoreGraphics::PrimitiveGroup& 
D3D9ParticleRenderer::GetPrimitiveGroup()
{
    return this->primGroup;
}

//------------------------------------------------------------------------------
/**
*/
inline
const Ptr<CoreGraphics::VertexLayout>& 
D3D9ParticleRenderer::GetVertexLayout() const
{
    return this->vertexLayout;
}

} // namespace Direct3D9
//------------------------------------------------------------------------------

