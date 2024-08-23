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
#include "coregraphics/meshresource.h"
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

    struct MeshStreamData
    {
        void* mappedData;
        CoreGraphics::VertexAlloc indexAllocationOffset, vertexAllocationOffset;
        CoreGraphics::CmdBufferId cmdBuf;
    };
    
    /// Initialize mesh
    Resources::ResourceUnknownId InitializeResource(const Ids::Id32 entry, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream, bool immediate = false) override;
    /// Stream texture
    uint StreamResource(const Resources::ResourceId entry, IndexT frameIndex, uint requestedBits) override;
    /// unload resource (overload to implement resource deallocation)
    void Unload(const Resources::ResourceId id) override;
    /// Create load mask based on LOD
    uint LodMask(const Ids::Id32 entry, float lod, bool stream) const override;

    /// Update intermediate loaded state
    void UpdateLoaderSyncState() override;

    /// Get vertex layout
    static const CoreGraphics::VertexLayoutId GetLayout(const CoreGraphics::VertexLayoutType type);
    /// setup mesh from nvx3 file in memory
    void SetupMeshFromNvx(const Ptr<IO::Stream>& stream, const Ids::Id32 entry, const MeshResourceId meshResource, bool immediate);

    Util::FixedArray<Util::Array<CoreGraphics::CmdBufferId>> retiredCommandBuffers;

    Util::Array<Resources::ResourceId> partiallyCompleteResources;
    CoreGraphics::CmdBufferPoolId transferPool;

};

} // namespace CoreGraphics
