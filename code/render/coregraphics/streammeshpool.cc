//------------------------------------------------------------------------------
// vkstreammeshloader.cc
// (C) 2016-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "streammeshpool.h"
#include "coregraphics/mesh.h"
#include "coregraphics/legacy/nvx2streamreader.h"
#include "coregraphics/memoryindexbufferpool.h"
#include "coregraphics/memoryvertexbufferpool.h"
#include "coregraphics/memorymeshpool.h"
#include "coregraphics/graphicsdevice.h"
#include "streammeshpool.h"

namespace CoreGraphics
{
using namespace IO;
using namespace CoreGraphics;
using namespace Util;
__ImplementClass(CoreGraphics::StreamMeshPool, 'VKML', Resources::ResourceStreamPool);
//------------------------------------------------------------------------------
/**
*/
StreamMeshPool::StreamMeshPool() :
	activeMesh(Ids::InvalidId24)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
StreamMeshPool::~StreamMeshPool()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
Resources::ResourcePool::LoadStatus
StreamMeshPool::LoadFromStream(const Resources::ResourceId id, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream)
{
	n_assert(stream.isvalid());
	n_assert(id != Ids::InvalidId24);
	String resIdExt = this->GetName(id.poolId).AsString().GetFileExtension();

#if NEBULA_LEGACY_SUPPORT
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
			n_error("StreamMeshPool::SetupMeshFromStream(): unrecognized file extension in '%s'\n", resIdExt.AsCharPtr());
			return Resources::ResourcePool::Failed;
		}
}

//------------------------------------------------------------------------------
/**
*/
void
StreamMeshPool::Unload(const Resources::ResourceId id)
{
	n_assert(id != Ids::InvalidId24);
	const MeshCreateInfo& msh = meshPool->GetSafe<0>(id.allocId);

	if (msh.indexBuffer != IndexBufferId::Invalid()) CoreGraphics::iboPool->Unload(msh.indexBuffer.AllocId());
	if (msh.vertexBuffer != VertexBufferId::Invalid()) CoreGraphics::vboPool->Unload(msh.vertexBuffer.AllocId());
}

//------------------------------------------------------------------------------
/**
*/
Resources::ResourceUnknownId
StreamMeshPool::AllocObject()
{
	return meshPool->AllocObject();
}

//------------------------------------------------------------------------------
/**
*/
void
StreamMeshPool::DeallocObject(const Resources::ResourceUnknownId id)
{
	meshPool->DeallocObject(id);
}

//------------------------------------------------------------------------------
/**
	Setup the mesh resource from legacy nvx2 file (Nebula2 binary mesh format).
*/
#if NEBULA_LEGACY_SUPPORT
Resources::ResourcePool::LoadStatus
StreamMeshPool::SetupMeshFromNvx2(const Ptr<Stream>& stream, const Resources::ResourceId res)
{
	n_assert(stream.isvalid());
	Ptr<Legacy::Nvx2StreamReader> nvx2Reader = Legacy::Nvx2StreamReader::Create();
	nvx2Reader->SetStream(stream);
	nvx2Reader->SetUsage(this->usage);
	nvx2Reader->SetAccess(this->access);
	Resources::ResourceName name = this->GetName(res);

	// opening the reader also loads the file
	if (nvx2Reader->Open(name))
	{
		meshPool->EnterGet();
		MeshCreateInfo& msh = meshPool->Get<0>(res);
		n_assert(this->GetState(res) == Resources::Resource::Pending);
		msh.vertexLayout = CreateVertexLayout({ nvx2Reader->GetVertexComponents() });
		msh.vertexBuffer = nvx2Reader->GetVertexBuffer();
		msh.indexBuffer = nvx2Reader->GetIndexBuffer();
		msh.topology = PrimitiveTopology::TriangleList;
		msh.primitiveGroups = nvx2Reader->GetPrimitiveGroups();
		meshPool->LeaveGet();

		nvx2Reader->Close();
		return ResourcePool::Success;
	}
	return ResourcePool::Failed;
}
#endif

//------------------------------------------------------------------------------
/**
	Setup the mesh resource from a nvx3 file (Nebula's
	native binary mesh file format).
*/
Resources::ResourcePool::LoadStatus
StreamMeshPool::SetupMeshFromNvx3(const Ptr<Stream>& stream, const Resources::ResourceId res)
{
	// FIXME!
	n_error("StreamMeshPool::SetupMeshFromNvx3() not yet implemented");
	return Resources::ResourcePool::Failed;
}

//------------------------------------------------------------------------------
/**
	Setup the mesh resource from a n3d3 file (Nebula's
	native ascii mesh file format).
*/
Resources::ResourcePool::LoadStatus
StreamMeshPool::SetupMeshFromN3d3(const Ptr<Stream>& stream, const Resources::ResourceId res)
{
	// FIXME!
	n_error("StreamMeshPool::SetupMeshFromN3d3() not yet implemented");
	return Resources::ResourcePool::Failed;
}

//------------------------------------------------------------------------------
/**
*/
void
StreamMeshPool::MeshBind(const Resources::ResourceId id)
{
	meshPool->EnterGet();
	const MeshCreateInfo& msh = meshPool->Get<0>(id);

	// bind vbo, and optional ibo
	CoreGraphics::SetPrimitiveTopology(msh.topology);
	CoreGraphics::SetVertexLayout(msh.vertexLayout);
	CoreGraphics::SetStreamVertexBuffer(0, msh.vertexBuffer, 0);
	if (msh.indexBuffer != Ids::InvalidId64)
		CoreGraphics::SetIndexBuffer(msh.indexBuffer, 0);

	meshPool->LeaveGet();
	this->activeMesh = id;
}

//------------------------------------------------------------------------------
/**
*/
void
StreamMeshPool::BindPrimitiveGroup(const IndexT primgroup)
{
	n_assert(this->activeMesh != Ids::InvalidId24);
	meshPool->EnterGet();
	const MeshCreateInfo& msh = meshPool->Get<0>(this->activeMesh);
	CoreGraphics::SetPrimitiveGroup(msh.primitiveGroups[primgroup]);
	meshPool->LeaveGet();
}

} // namespace Vulkan