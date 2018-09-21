//------------------------------------------------------------------------------
//  particleserver.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "particles/particleserver.h"
#include "coregraphics/vertexbuffer.h"
#include "coregraphics/indexbuffer.h"

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

	// create job port
	this->jobPort = Jobs::CreateJobPort({ "ParticleJobs", 4, UINT_MAX });

	// setup mesh
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

	VertexBufferCreateInfo vboInfo =
	{
		"Default_Emitter_Mesh_VBO",
		"rendersystem",
		GpuBufferTypes::AccessRead, GpuBufferTypes::UsageImmutable, GpuBufferTypes::SyncingFlush,
		1, emitterComponents,
		vertex, sizeof(vertex)
	};
	VertexBufferId vbo = CreateVertexBuffer(vboInfo);
	if (vbo != VertexBufferId::Invalid())
	{
		n_error("ParticleServer::Open: Failed to setup default emitter mesh");
	}

	uint indices[] = { 0 };
	IndexBufferCreateInfo iboInfo = 
	{
		"Default_Emitter_Mesh_IBO",
		"rendersystem",
		GpuBufferTypes::AccessRead, GpuBufferTypes::UsageImmutable, GpuBufferTypes::SyncingFlush,
		IndexType::Index32,	1, indices, sizeof(indices)
	};
	IndexBufferId ibo = CreateIndexBuffer(iboInfo);
	if (ibo != IndexBufferId::Invalid())
	{
		n_error("ParticleServer::Open: Failed to setup default emitter mesh");
	}

	PrimitiveGroup group;
	group.SetBaseIndex(0);
	group.SetBaseVertex(0);
	group.SetNumIndices(1);
	group.SetNumVertices(1);
	Util::Array<PrimitiveGroup> groups;
	groups.Append(group);

	VertexLayoutId vlo = VertexBufferGetLayout(vbo);

	MeshCreateInfo info =
	{
		"Default_Emitter_Mesh",
		"rendersystem",
		vbo, ibo, vlo, CoreGraphics::PrimitiveTopology::PointList, groups
	};
	this->defaultEmitterMesh = CreateMesh(info);
}

//------------------------------------------------------------------------------
/**
*/
void
ParticleServer::Close()
{
    n_assert(this->IsOpen());

	// destroy job port
	Jobs::DestroyJobPort(this->jobPort);
	this->jobPort = Jobs::JobPortId::Invalid();

    // destroy the particle renderer singleton
    this->particleRenderer->Discard();
    this->particleRenderer = nullptr;

	DestroyMesh(this->defaultEmitterMesh);

    this->isOpen = false;
}

} // namespace Particles