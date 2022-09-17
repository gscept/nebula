#pragma once
//------------------------------------------------------------------------------
/**
    Implements a mesh loader from stream into Vulkan. Doubtful this is Vulkan specific...
    
    @copyright
    (C) 2016-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "resources/resourceloader.h"
#include "coregraphics/gpubuffertypes.h"
#include "coregraphics/mesh.h"
namespace CoreGraphics
{
class MeshLoader : public Resources::ResourceLoader
{
    __DeclareClass(MeshLoader);
public:
    /// constructor
    MeshLoader();
    /// destructor
    virtual ~MeshLoader();

	struct StreamMeshLoadMetaData
	{
		bool copySource;
	};

private:
    
    /// perform load
    Resources::ResourceUnknownId LoadFromStream(const Ids::Id32 entry, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream, bool immediate = false) override;
    /// unload resource (overload to implement resource deallocation)
    void Unload(const Resources::ResourceId id) override;

#if NEBULA_LEGACY_SUPPORT
    /// setup mesh from nvx2 file in memory
    MeshId SetupMeshFromNvx2(const Ptr<IO::Stream>& stream, const Ids::Id32 entry);
#endif
    /// setup mesh from nvx3 file in memory
    MeshId SetupMeshFromNvx3(const Ptr<IO::Stream>& stream, const Ids::Id32 entry);
    /// setup mesh from n3d3 file in memory
    MeshId SetupMeshFromN3d3(const Ptr<IO::Stream>& stream, const Ids::Id32 entry);

protected:
    GpuBufferTypes::Usage usage;
    GpuBufferTypes::Access access;

    Resources::ResourceId activeMesh;
};

} // namespace CoreGraphics
