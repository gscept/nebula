//------------------------------------------------------------------------------
// vkstreammeshloader.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "meshpool.h"
#include "coregraphics/mesh.h"
#include "coregraphics/legacy/nvx2streamreader.h"

namespace CoreGraphics
{
using namespace IO;
using namespace CoreGraphics;
using namespace Util;
__ImplementClass(CoreGraphics::MeshPool, 'VKML', Resources::ResourceStreamPool);
//------------------------------------------------------------------------------
/**
*/
MeshPool::MeshPool()
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
MeshPool::Load(const Ptr<Resources::Resource>& res, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream)
{
	n_assert(stream.isvalid());
	n_assert(res.isvalid());
	String resIdExt = res->GetResourceName().AsString().GetFileExtension();

#if NEBULA3_LEGACY_SUPPORT
	if (resIdExt == "nvx2")
	{
		return this->SetupMeshFromNvx2(stream, res);
	}
	else
#endif

		if (resIdExt == "nvx3")
		{
			return this->SetupMeshFromNvx3(stream, res);
		}
		else if (resIdExt == "n3d3")
		{
			return this->SetupMeshFromN3d3(stream, res);
		}
		else
		{
			n_error("MeshPool::SetupMeshFromStream(): unrecognized file extension in '%s'\n", res->GetResourceName().Value());
			return Resources::ResourcePool::Failed;
		}
}

//------------------------------------------------------------------------------
/**
	Setup the mesh resource from legacy nvx2 file (Nebula2 binary mesh format).
*/
#if NEBULA3_LEGACY_SUPPORT
Resources::ResourcePool::LoadStatus
MeshPool::SetupMeshFromNvx2(const Ptr<Stream>& stream, const Ptr<Resources::Resource>& res)
{
	n_assert(stream.isvalid());
	Ptr<Legacy::Nvx2StreamReader> nvx2Reader = Legacy::Nvx2StreamReader::Create();
	nvx2Reader->SetStream(stream);
	nvx2Reader->SetUsage(this->usage);
	nvx2Reader->SetAccess(this->access);

	// opening the reader also loads the file
	if (nvx2Reader->Open())
	{
		const Ptr<Mesh>& res = res.downcast<Mesh>();
		n_assert(res->GetState() == ResourcePool::Success);
		res->SetVertexBuffer(nvx2Reader->GetVertexBuffer());
		res->SetIndexBuffer(nvx2Reader->GetIndexBuffer());
		res->SetPrimitiveGroups(nvx2Reader->GetPrimitiveGroups());
		res->SetTopology(PrimitiveTopology::TriangleList);
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
MeshPool::SetupMeshFromNvx3(const Ptr<Stream>& stream, const Ptr<Resources::Resource>& res)
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
MeshPool::SetupMeshFromN3d3(const Ptr<Stream>& stream, const Ptr<Resources::Resource>& res)
{
	// FIXME!
	n_error("MeshPool::SetupMeshFromN3d3() not yet implemented");
	return Resources::ResourcePool::Failed;
}

} // namespace Vulkan