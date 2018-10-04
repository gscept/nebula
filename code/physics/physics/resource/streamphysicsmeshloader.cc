//------------------------------------------------------------------------------
//  physicsstreammeshloader.cc
//  (C) 2012-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "physics/resource/streamphysicsmeshloader.h"
#include "physics/resource/physicsmesh.h"
#include "coregraphics/legacy/nvx2streamreader.h"

namespace Physics
{
__ImplementClass(Physics::StreamPhysicsMeshLoader, 'PHML', Resources::StreamResourceLoader);

using namespace IO;
using namespace CoreGraphics;
using namespace Util;

//------------------------------------------------------------------------------
/**
*/
StreamPhysicsMeshLoader::StreamPhysicsMeshLoader()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    Setup the mesh resource from legacy nvx2 file (Nebula2 binary mesh format).
*/
bool
StreamPhysicsMeshLoader::SetupResourceFromStream(const Ptr<Stream>& stream)
{
    n_assert(stream.isvalid());    
    Ptr<Legacy::Nvx2StreamReader> nvx2Reader = Legacy::Nvx2StreamReader::Create();
    nvx2Reader->SetStream(stream);
    nvx2Reader->SetUsage(Base::ResourceBase::UsageImmutable);	
    nvx2Reader->SetAccess(Base::ResourceBase::AccessNone);
	nvx2Reader->SetRawMode(true);

    if (nvx2Reader->Open())
    {
        const Ptr<PhysicsMesh>& res = this->resource.downcast<PhysicsMesh>();
        n_assert(!res->IsLoaded());
		const Util::Array<CoreGraphics::PrimitiveGroup>& groups = nvx2Reader->GetPrimitiveGroups();		

		float *vertexData = nvx2Reader->GetVertexData();
		uint *indexData = (uint*)nvx2Reader->GetIndexData(); 
		res->SetMeshData(vertexData, nvx2Reader->GetNumVertices(), nvx2Reader->GetVertexWidth(), indexData, nvx2Reader->GetNumIndices());
		for(int i=0;i < groups.Size();i++)
		{
			res->AddMeshComponent(i, groups[i]);
		}		
        nvx2Reader->Close();
        return true;
    }
	else
	{
		n_error("PhysicsStreamMeshLoader: Can't open file '%s'", stream->GetURI().AsString().AsCharPtr());
	}
    return false;
}

} // namespace Physics
