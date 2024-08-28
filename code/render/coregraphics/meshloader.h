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
    };
    
    /// Initialize mesh
    ResourceInitOutput InitializeResource(const ResourceLoadJob& job, const Ptr<IO::Stream>& stream) override;
    /// Stream texture
    ResourceStreamOutput StreamResource(const ResourceLoadJob& job) override;
    /// unload resource (overload to implement resource deallocation)
    void Unload(const Resources::ResourceId id) override;
    /// Create load mask based on LOD
    uint LodMask(const _StreamData& stream, float lod, bool async) const override;

    /// Update intermediate loaded state
    void UpdateLoaderSyncState() override;

    /// Get vertex layout
    static const CoreGraphics::VertexLayoutId GetLayout(const CoreGraphics::VertexLayoutType type);
    /// setup mesh from nvx3 file in memory
    ResourceLoader::_StreamData SetupMeshFromNvx(const Ptr<IO::Stream>& stream, const ResourceLoadJob& job, const MeshResourceId meshResource);

    struct MeshesToSubmit
    {
        Resources::ResourceId id;
        uint bits;
        CoreGraphics::CmdBufferId cmdBuf;
    };
    Threading::SafeQueue<MeshesToSubmit> meshesToSubmit;

    struct FinishedMesh
    {
        uint64 submissionId;
        uint bits;
        CoreGraphics::CmdBufferId cmdBuf;
    };
    Util::HashTable<Resources::ResourceId, Util::Array<FinishedMesh>> meshesToFinish;
    Threading::CriticalSection meshLock;


    CoreGraphics::CmdBufferPoolId asyncTransferPool, immediateTransferPool;

};

} // namespace CoreGraphics
