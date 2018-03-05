#pragma once
//------------------------------------------------------------------------------
/**
	Implements a particle renderer made to use Vulkan.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "particles/base/particlerendererbase.h"
namespace Vulkan
{
class VkParticleRenderer : public Base::ParticleRendererBase
{
	__DeclareClass(VkParticleRenderer);
	__DeclareSingleton(VkParticleRenderer);
public:
	/// constructor
	VkParticleRenderer();
	/// destructor
	virtual ~VkParticleRenderer();

	/// setup the particle renderer
	virtual void Setup();
	/// discard the particle renderer
	virtual void Discard();

	/// begin attaching visible particle systems
	virtual void BeginAttach();
	/// finish attaching visible particle systems
	virtual void EndAttach();

	/// get the current particle index
	IndexT GetCurParticleIndex() const;
	/// add particle index
	void AddCurParticleIndex(IndexT add);
	/// apply mesh
	void ApplyParticleMesh();

	/// get the current vertex pointer
	void* GetCurVertexPtr();
	/// set the current vertex pointer
	void SetCurVertexPtr(void *ptr);
	/// get particle vertex buffer
	const CoreGraphics::VertexBufferId& GetParticleVertexBuffer() const;
	/// get the corner vertex buffer
	const CoreGraphics::VertexBufferId& GetCornerVertexBuffer() const;
	/// get the corner index buffer
	const CoreGraphics::IndexBufferId& GetCornerIndexBuffer() const;
	/// get the primitive group
	CoreGraphics::PrimitiveGroup& GetPrimitiveGroup();
	/// get the vertex layout
	const CoreGraphics::VertexLayoutId& GetVertexLayout() const;
private:

	CoreGraphics::VertexBufferId particleVertexBuffer;
	CoreGraphics::VertexBufferId cornerVertexBuffer;
	CoreGraphics::IndexBufferId cornerIndexBuffer;
	CoreGraphics::PrimitiveGroup primGroup;
	CoreGraphics::VertexLayoutId vertexLayout;
	IndexT curParticleIndex;
	void* mappedVertices;
	void* curVertexPtr;
};


//------------------------------------------------------------------------------
/**
*/
inline IndexT
VkParticleRenderer::GetCurParticleIndex() const
{
	return this->curParticleIndex;
}

//------------------------------------------------------------------------------
/**
*/
inline void
VkParticleRenderer::AddCurParticleIndex(IndexT add)
{
	n_assert(add > 0);
	this->curParticleIndex += add;
}

//------------------------------------------------------------------------------
/**
*/
inline void*
VkParticleRenderer::GetCurVertexPtr()
{
	return this->curVertexPtr;
}

//------------------------------------------------------------------------------
/**
*/
inline void
VkParticleRenderer::SetCurVertexPtr(void *ptr)
{
	this->curVertexPtr = ptr;
}

//------------------------------------------------------------------------------
/**
*/
inline const CoreGraphics::VertexBufferId&
VkParticleRenderer::GetParticleVertexBuffer() const
{
	return this->particleVertexBuffer;
}

//------------------------------------------------------------------------------
/**
*/
inline const CoreGraphics::VertexBufferId&
VkParticleRenderer::GetCornerVertexBuffer() const
{
	return this->cornerVertexBuffer;
}

//------------------------------------------------------------------------------
/**
*/
inline const CoreGraphics::IndexBufferId&
VkParticleRenderer::GetCornerIndexBuffer() const
{
	return this->cornerIndexBuffer;
}

//------------------------------------------------------------------------------
/**
*/
inline CoreGraphics::PrimitiveGroup&
VkParticleRenderer::GetPrimitiveGroup()
{
	return this->primGroup;
}

//------------------------------------------------------------------------------
/**
*/
inline const CoreGraphics::VertexLayoutId&
VkParticleRenderer::GetVertexLayout() const
{
	return this->vertexLayout;
}

} // namespace Vulkan