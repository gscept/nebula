#pragma once
//------------------------------------------------------------------------------
/**
    @class Direct3D11::D3D11ParticleRenderer

    Particle system renderer for D3D11/Xbox360. The renderer makes use of
    hardware instancing to prevent writing redundant data to
    dynamic vertex buffers.

    See here for details:
    http://zeuxcg.blogspot.com/2007/09/particle-rendering-revisited.html

    (C) 2008 Radon Labs GmbH
    (C) 2013 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "particles/base/particlerendererbase.h"
#include "coregraphics/vertexbuffer.h"
#include "coregraphics/indexbuffer.h"
#include "coregraphics/vertexlayout.h"

namespace Direct3D11
{
class D3D11ParticleRenderer : public Base::ParticleRendererBase
{
    __DeclareClass(D3D11ParticleRenderer);
    __DeclareSingleton(D3D11ParticleRenderer);
public:
    /// constructor
    D3D11ParticleRenderer();
    /// destructor
    virtual ~D3D11ParticleRenderer();

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
D3D11ParticleRenderer::GetCurParticleVertexIndex() const
{
    return this->curParticleVertexIndex;
}

//------------------------------------------------------------------------------
/**
*/
inline
void 
D3D11ParticleRenderer::AddCurParticleVertexIndex(IndexT add)
{
    n_assert(add > 0);
    this->curParticleVertexIndex += add;
}

//------------------------------------------------------------------------------
/**
*/
inline
void* 
D3D11ParticleRenderer::GetCurVertexPtr()
{
    return this->curVertexPtr;
}

//------------------------------------------------------------------------------
/**
*/
inline
void 
D3D11ParticleRenderer::SetCurVertexPtr(void *ptr)
{
    this->curVertexPtr = ptr;
}

//------------------------------------------------------------------------------
/**
*/
inline
const Ptr<CoreGraphics::VertexBuffer>&
D3D11ParticleRenderer::GetParticleVertexBuffer() const
{
    return this->particleVertexBuffer;
}

//------------------------------------------------------------------------------
/**
*/
inline
const Ptr<CoreGraphics::VertexBuffer>&
D3D11ParticleRenderer::GetCornerVertexBuffer() const
{
    return this->cornerVertexBuffer;
}

//------------------------------------------------------------------------------
/**
*/
inline
const Ptr<CoreGraphics::IndexBuffer>& 
D3D11ParticleRenderer::GetCornerIndexBuffer() const
{
    return this->cornerIndexBuffer;
}

//------------------------------------------------------------------------------
/**
*/
inline
CoreGraphics::PrimitiveGroup& 
D3D11ParticleRenderer::GetPrimitiveGroup()
{
    return this->primGroup;
}

//------------------------------------------------------------------------------
/**
*/
inline
const Ptr<CoreGraphics::VertexLayout>& 
D3D11ParticleRenderer::GetVertexLayout() const
{
    return this->vertexLayout;
}

} // namespace Direct3D11
//------------------------------------------------------------------------------

