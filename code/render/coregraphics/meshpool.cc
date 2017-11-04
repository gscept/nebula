//------------------------------------------------------------------------------
// vkstreammeshloader.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "meshpool.h"
#include "coregraphics/mesh.h"
#include "coregraphics/legacy/nvx2streamreader.h"
#include "coregraphics.h"
#include "vk/vkrenderdevice.h"
#include "coregraphics/memoryindexbufferpool.h"
#include "coregraphics/memoryvertexbufferpool.h"

namespace CoreGraphics
{
using namespace IO;
using namespace CoreGraphics;
using namespace Util;
__ImplementClass(CoreGraphics::MeshPool, 'VKML', Resources::ResourceStreamPool);
//------------------------------------------------------------------------------
/**
*/
MeshPool::MeshPool() :
	activeMesh(Ids::InvalidId24)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
MeshPool::~MeshPool()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
Resources::ResourcePool::LoadStatus
MeshPool::Load(const Ids::Id24 id, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream)
{
	n_assert(stream.isvalid());
	n_assert(id != Ids::InvalidId24);
	Resources::ResourceName name = this->GetName(id);
	String resIdExt = this->GetName(id).AsString().GetFileExtension();

#if NEBULA3_LEGACY_SUPPORT
	if (resIdExt == "nvx2")
	{
		return this->SetupMeshFromNvx2(stream, id);
	}
	else
#endif
		if (resIdExt == "nvx3")
		{
			return this->SetupMeshFromNvx3(stream, id);
		}
		else if (resIdExt == "n3d3")
		{
			return this->SetupMeshFromN3d3(stream, id);
		}
		else
		{
			n_error("MeshPool::SetupMeshFromStream(): unrecognized file extension in '%s'\n", name.Value());
			return Resources::ResourcePool::Failed;
		}
}

//------------------------------------------------------------------------------
/**
*/
void
MeshPool::Unload(const Ids::Id24 id)
{
	n_assert(id != Ids::InvalidId24);
	const Mesh& msh = this->Get<0>(id);

	if (msh.indexBuffer != Ids::InvalidId64) CoreGraphics::iboPool->DiscardResource(msh.indexBuffer);
	if (msh.vertexBuffer != Ids::InvalidId64) CoreGraphics::vboPool->DiscardResource(msh.vertexBuffer);
}

//------------------------------------------------------------------------------
/**
	Setup the mesh resource from legacy nvx2 file (Nebula2 binary mesh format).
*/
#if NEBULA3_LEGACY_SUPPORT
Resources::ResourcePool::LoadStatus
MeshPool::SetupMeshFromNvx2(const Ptr<Stream>& stream, const Ids::Id24 res)
{
	n_assert(stream.isvalid());
	Ptr<Legacy::Nvx2StreamReader> nvx2Reader = Legacy::Nvx2StreamReader::Create();
	nvx2Reader->SetStream(stream);
	nvx2Reader->SetUsage(this->usage);
	nvx2Reader->SetAccess(this->access);

	// opening the reader also loads the file
	if (nvx2Reader->Open())
	{
		Mesh& msh = this->Get<0>(res);
		n_assert(this->GetState(res) == Resources::Resource::Loaded);
		msh.vertexBuffer = nvx2Reader->GetVertexBuffer();
		msh.indexBuffer = nvx2Reader->GetIndexBuffer();
		msh.topology = PrimitiveTopology::TriangleList;
		msh.primitiveGroups = nvx2Reader->GetPrimitiveGroups();
		
		nvx2Reader->Close();
		return ResourcePool::Success;
	}
	return ResourcePool::Failed;
}
#endif

//------------------------------------------------------------------------------
/**
	Setup the mesh resource from a nvx3 file (Nebula3's
	native binary mesh file format).
*/
Resources::ResourcePool::LoadStatus
MeshPool::SetupMeshFromNvx3(const Ptr<Stream>& stream, const Ids::Id24 res)
{
	// FIXME!
	n_error("MeshPool::SetupMeshFromNvx3() not yet implemented");
	return Resources::ResourcePool::Failed;
}

//------------------------------------------------------------------------------
/**
	Setup the mesh resource from a n3d3 file (Nebula3's
	native ascii mesh file format).
*/
Resources::ResourcePool::LoadStatus
MeshPool::SetupMeshFromN3d3(const Ptr<Stream>& stream, const Ids::Id24 res)
{
	// FIXME!
	n_error("MeshPool::SetupMeshFromN3d3() not yet implemented");
	return Resources::ResourcePool::Failed;
}

//------------------------------------------------------------------------------
/**
*/
void
MeshPool::BindMesh(const Resources::ResourceId id)
{
	const Ids::Id24 resId = Ids::Id::GetBig(Ids::Id::GetLow(id));
	const Mesh& msh = this->Get<0>(id);

	// bind vbo, and optional ibo
	CoreGraphics::RenderDevice::Instance()->SetPrimitiveTopology(msh.topology);
	CoreGraphics::BindVertexLayout(msh.vertexLayout);
	CoreGraphics::BindVertexBuffer(msh.vertexBuffer, 0, 0);
	if (msh.indexBuffer != Ids::InvalidId64)
		CoreGraphics::BindIndexBuffer(msh.indexBuffer);

	this->activeMesh = id;
}

//------------------------------------------------------------------------------
/**
*/
void
MeshPool::BindPrimitiveGroup(const IndexT primgroup)
{
	n_assert(this->activeMesh != Ids::InvalidId24);
	const Mesh& msh = this->Get<0>(this->activeMesh);
	RenderDevice::Instance()->SetPrimitiveGroup(msh.primitiveGroups[primgroup]);
}

} // namespace Vulkan