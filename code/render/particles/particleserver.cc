//------------------------------------------------------------------------------
//  particleserver.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "particles/particleserver.h"
#include "coregraphics/memoryvertexbufferloader.h"
#include "coregraphics/memoryindexbufferloader.h"
#include "resources/resourceloader.h"

namespace Particles
{
__ImplementClass(Particles::ParticleServer, 'PRSV', Core::RefCounted);
__ImplementSingleton(Particles::ParticleServer);

using namespace CoreGraphics;

//------------------------------------------------------------------------------
/**
*/
ParticleServer::ParticleServer() :
    isOpen(false)
{
    __ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
ParticleServer::~ParticleServer()
{
    __DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
void
ParticleServer::Open()
{
    n_assert(!this->IsOpen());
    this->isOpen = true;

    // setup the particle renderer singleton
    this->particleRenderer = ParticleRenderer::Create();
    this->particleRenderer->Setup();

	// setup mesh
	this->defaultEmitterMesh = Mesh::Create();

	// setup default emitter vertex buffer and index buffer
	Ptr<MemoryVertexBufferLoader> emitterLoader = MemoryVertexBufferLoader::Create();
	Util::Array<VertexComponent> emitterComponents;
	emitterComponents.Append(VertexComponent(VertexComponent::Position, 0, VertexComponent::Float3, 0));
	emitterComponents.Append(VertexComponent(VertexComponent::Normal, 0, VertexComponent::Byte4N, 0));
	emitterComponents.Append(VertexComponent(VertexComponent::Tangent, 0, VertexComponent::Byte4N, 0));

	float x = 0 * 0.5f * 255.0f;
	float y = 1 * 0.5f * 255.0f;
	float z = 0 * 0.5f * 255.0f;
	float w = 0 * 0.5f * 255.0f;
	int xBits = (int)x;
	int yBits = (int)y;
	int zBits = (int)z;
	int wBits = (int)w;
	int normPacked = ((wBits << 24) & 0xFF000000) | ((zBits << 16) & 0x00FF0000) | ((yBits << 8) & 0x0000FF00) | (xBits & 0x000000FF);

	x = 0 * 0.5f * 255.0f;
	y = 0 * 0.5f * 255.0f;
	z = 1 * 0.5f * 255.0f;
	w = 0 * 0.5f * 255.0f;
	xBits = (int)x;
	yBits = (int)y;
	zBits = (int)z;
	wBits = (int)w;
	int tangentPacked = ((wBits << 24) & 0xFF000000) | ((zBits << 16) & 0x00FF0000) | ((yBits << 8) & 0x0000FF00) | (xBits & 0x000000FF);

	float vertex[] = {0, 0, 0, 0, 0};
	*(int*)&vertex[3] = normPacked;
	*(int*)&vertex[4] = tangentPacked;
	emitterLoader->Setup(emitterComponents, 1, vertex, sizeof(vertex), VertexBuffer::UsageImmutable, VertexBuffer::AccessRead);
	Ptr<VertexBuffer> vb = VertexBuffer::Create();
	vb->SetLoader(emitterLoader.upcast<Resources::ResourceLoader>());
	vb->SetAsyncEnabled(false);
	vb->Load();
	if (!vb->IsLoaded())
	{
		n_error("ParticleServer::Open: Failed to setup default emitter mesh");
	}
	vb->SetLoader(0);

	Ptr<MemoryIndexBufferLoader> indexLoader = MemoryIndexBufferLoader::Create();
	uint indices[] = {0};
	indexLoader->Setup(IndexType::Index32, 1, indices, sizeof(indices), IndexBuffer::UsageImmutable, IndexBuffer::AccessRead);
	Ptr<IndexBuffer> ib = IndexBuffer::Create();
	ib->SetLoader(indexLoader.upcast<Resources::ResourceLoader>());
	ib->SetAsyncEnabled(false);
	ib->Load();
	if (!ib->IsLoaded())
	{
		n_error("ParticleServer::Open: Failed to setup default emitter mesh");
	}
	ib->SetLoader(0);

	PrimitiveGroup group;
	group.SetBaseIndex(0);
	group.SetBaseVertex(0);
	group.SetNumIndices(1);
	group.SetNumVertices(1);
	Util::Array<PrimitiveGroup> groups;
	groups.Append(group);

	// setup mesh
	this->defaultEmitterMesh->SetTopology(PrimitiveTopology::PointList);
	this->defaultEmitterMesh->SetVertexBuffer(vb);
	this->defaultEmitterMesh->SetIndexBuffer(ib);
	this->defaultEmitterMesh->SetPrimitiveGroups(groups);
}

//------------------------------------------------------------------------------
/**
*/
void
ParticleServer::Close()
{
    n_assert(this->IsOpen());

    // destroy the particle renderer singleton
    this->particleRenderer->Discard();
    this->particleRenderer = 0;

	// destroy mesh
	this->defaultEmitterMesh->Unload();
	this->defaultEmitterMesh = 0;

    this->isOpen = false;
}

} // namespace Particles