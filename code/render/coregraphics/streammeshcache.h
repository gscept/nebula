#pragma once
//------------------------------------------------------------------------------
/**
    Implements a mesh loader from stream into Vulkan. Doubtful this is Vulkan specific...
    
    @copyright
    (C) 2016-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "resources/resourcestreamcache.h"
#include "coregraphics/gpubuffertypes.h"
#include "coregraphics/mesh.h"
namespace CoreGraphics
{
class StreamMeshCache : public Resources::ResourceStreamCache
{
    __DeclareClass(StreamMeshCache);
public:
    /// constructor
    StreamMeshCache();
    /// destructor
    virtual ~StreamMeshCache();

	struct StreamMeshLoadMetaData
	{
		bool copySource;
	};

private:
    
    /// perform load
    LoadStatus LoadFromStream(const Resources::ResourceId id, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream, bool immediate = false) override;
    /// unload resource (overload to implement resource deallocation)
    void Unload(const Resources::ResourceId id) override;

    /// allocate object
    Resources::ResourceUnknownId AllocObject() override;
    /// deallocate object
    void DeallocObject(const Resources::ResourceUnknownId id) override;

#if NEBULA_LEGACY_SUPPORT
    /// setup mesh from nvx2 file in memory
    LoadStatus SetupMeshFromNvx2(const Ptr<IO::Stream>& stream, const Resources::ResourceId res);
#endif
    /// setup mesh from nvx3 file in memory
    LoadStatus SetupMeshFromNvx3(const Ptr<IO::Stream>& stream, const Resources::ResourceId res);
    /// setup mesh from n3d3 file in memory
    LoadStatus SetupMeshFromN3d3(const Ptr<IO::Stream>& stream, const Resources::ResourceId res);

protected:
    GpuBufferTypes::Usage usage;
    GpuBufferTypes::Access access;

    Resources::ResourceId activeMesh;
};

} // namespace CoreGraphics
