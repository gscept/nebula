//------------------------------------------------------------------------------
// vkparticlerenderer.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "vkparticlerenderer.h"
#include "coregraphics/vertexcomponent.h"
#include "coregraphics/graphicsdevice.h"
#include "coregraphics/vertexsignaturepool.h"
#include "particles/particle.h"
#include "coregraphics/vertexbuffer.h"
#include "coregraphics/indexbuffer.h"

using namespace Util;
using namespace Resources;
using namespace CoreGraphics;
using namespace Particles;
namespace Vulkan
{

__ImplementClass(Vulkan::VkParticleRenderer, 'VKPR', Base::ParticleRendererBase);
__ImplementSingleton(Vulkan::VkParticleRenderer);
//------------------------------------------------------------------------------
/**
*/
VkParticleRenderer::VkParticleRenderer() :
	curParticleIndex(0),
	mappedVertices(0),
	curVertexPtr(0)
{
	__ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
VkParticleRenderer::~VkParticleRenderer()
{
	if (this->IsValid())
	{
		this->Discard();
	}
	__DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
void
VkParticleRenderer::Setup()
{
	n_assert(!this->IsValid());
	n_assert(this->particleVertexBuffer == VertexBufferId::Invalid());
	n_assert(this->cornerVertexBuffer == VertexBufferId::Invalid());
	n_assert(this->cornerIndexBuffer == IndexBufferId::Invalid());

	ParticleRendererBase::Setup();
	this->mappedVertices = 0;
	this->curVertexPtr = 0;

	// we need to create 2 vertex buffers and 1 index buffer:
	//  - one static vertex buffer which contains 4 corner vertices
	//  - one dynamic vertex buffer with one vertex per particle
	//  - one index buffer which 6 indices, forming 2 triangles

	// setup the corner vertex buffer
	Array<VertexComponent> cornerComponents;
	cornerComponents.Append(VertexComponent((VertexComponent::SemanticName)0, 0, VertexComponent::Float2, 0));
	float cornerVertexData[] = { 0, 0, 1, 0, 1, 1, 0, 1 };
	VertexBufferCreateInfo vboInfo =
	{
		"corner_particle_vbo", "render_system", GpuBufferTypes::AccessNone, GpuBufferTypes::UsageImmutable, GpuBufferTypes::SyncingFlush,
		4, cornerComponents, cornerVertexData, sizeof(cornerVertexData)
	};
	this->cornerVertexBuffer = CreateVertexBuffer(vboInfo);

	// setup the corner index buffer
	ushort cornerIndexData[] = { 0, 1, 2, 2, 3, 0 };

	IndexBufferCreateInfo iboInfo =
	{
		"corner_particle_ibo", "render_system", GpuBufferTypes::AccessNone, GpuBufferTypes::UsageImmutable, GpuBufferTypes::SyncingFlush,
		IndexType::Index16, 6, cornerIndexData, sizeof(cornerIndexData)
	};
	this->cornerIndexBuffer = CreateIndexBuffer(iboInfo);

	// setup the cornerPrimitiveGroup which describes one particle instance
	this->primGroup.SetBaseVertex(0);
	this->primGroup.SetNumVertices(4);
	this->primGroup.SetBaseIndex(0);
	this->primGroup.SetNumIndices(6);

	// setup the dynamic particle vertex buffer (contains one vertex per particle)
	Array<VertexComponent> particleComponents;
	particleComponents.Append(VertexComponent((VertexComponent::SemanticName)1, 0, VertexComponent::Float4, 1, VertexComponent::PerInstance, 1));   // Particle::position
	particleComponents.Append(VertexComponent((VertexComponent::SemanticName)2, 0, VertexComponent::Float4, 1, VertexComponent::PerInstance, 1));   // Particle::stretchPosition
	particleComponents.Append(VertexComponent((VertexComponent::SemanticName)3, 0, VertexComponent::Float4, 1, VertexComponent::PerInstance, 1));   // Particle::color
	particleComponents.Append(VertexComponent((VertexComponent::SemanticName)4, 0, VertexComponent::Float4, 1, VertexComponent::PerInstance, 1));   // Particle::uvMinMax
	particleComponents.Append(VertexComponent((VertexComponent::SemanticName)5, 0, VertexComponent::Float4, 1, VertexComponent::PerInstance, 1));   // x: Particle::rotation, y: Particle::size

	VertexBufferCreateInfo vboPInfo =
	{
		"corner_particle_particle_vbo", "render_system", GpuBufferTypes::AccessWrite, GpuBufferTypes::UsageDynamic, GpuBufferTypes::SyncingCoherent,
		MaxNumRenderedParticles, particleComponents, nullptr, 0
	};
	this->particleVertexBuffer = CreateVertexBuffer(vboPInfo);

	// map buffer
	this->mappedVertices = VertexBufferMap(this->particleVertexBuffer, GpuBufferTypes::MapWrite);


	Array<VertexComponent> components = cornerComponents;
	components.AppendArray(particleComponents);
	VertexLayoutCreateInfo vloInfo = 
	{
		components
	};
	this->vertexLayout = CreateVertexLayout(vloInfo);
}

//------------------------------------------------------------------------------
/**
*/
void
VkParticleRenderer::Discard()
{
	n_assert(this->IsValid());

	DestroyVertexBuffer(this->cornerVertexBuffer);
	DestroyIndexBuffer(this->cornerIndexBuffer);
	VertexBufferUnmap(this->particleVertexBuffer);
	DestroyVertexBuffer(this->particleVertexBuffer);
	DestroyVertexLayout(this->vertexLayout);

	ParticleRendererBase::Discard();
}

//------------------------------------------------------------------------------
/**
*/
void
VkParticleRenderer::BeginAttach()
{
	n_assert(!this->inAttach);
	n_assert(0 != this->mappedVertices);
	this->inAttach = true;
	this->curVertexPtr = ((float*)this->mappedVertices);
	this->curParticleIndex = 0;
}

//------------------------------------------------------------------------------
/**
*/
void
VkParticleRenderer::EndAttach()
{
	n_assert(this->inAttach);
	n_assert(0 != this->mappedVertices);
	this->inAttach = false;
	this->curVertexPtr = 0;
}

//------------------------------------------------------------------------------
/**
*/
void
VkParticleRenderer::ApplyParticleMesh()
{
	n_assert(!this->IsInAttach());
	CoreGraphics::SetPrimitiveTopology(PrimitiveTopology::TriangleList);
	CoreGraphics::SetVertexLayout(this->vertexLayout);
	CoreGraphics::SetStreamVertexBuffer(0, this->cornerVertexBuffer, 0);
	CoreGraphics::SetStreamVertexBuffer(1, this->particleVertexBuffer, 0);
	CoreGraphics::SetIndexBuffer(this->cornerIndexBuffer, 0);
	CoreGraphics::SetPrimitiveGroup(this->GetPrimitiveGroup());
}

} // namespace Vulkan