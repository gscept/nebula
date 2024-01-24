
//------------------------------------------------------------------------------
//  vegetationcontext.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "graphics/graphicsserver.h"
#include "vegetationcontext.h"
#include "resources/resourceserver.h"
#include "math/plane.h"
#include "graphics/view.h"
#include "graphics/cameracontext.h"
#include "frame/framesubgraph.h"
#include "coregraphics/meshloader.h"
#include "models/model.h"
#include "models/nodes/modelnode.h"
#include "models/nodes/transformnode.h"
#include "models/nodes/primitivenode.h"

#include "system_shaders/vegetation.h"
namespace Vegetation
{

VegetationContext::VegetationAllocator VegetationContext::vegetationAllocator;
__ImplementContext(VegetationContext, VegetationContext::vegetationAllocator);

struct
{
    CoreGraphics::TextureId heightMap;
    float minHeight, maxHeight;
    Math::vec2 worldSize;
    uint numGrassBlades;

    CoreGraphics::BufferId grassVbo;
    CoreGraphics::BufferId grassIbo;
    CoreGraphics::VertexLayoutId grassLayout;
    CoreGraphics::VertexLayoutId combinedMeshLayoutWithColor;
    CoreGraphics::VertexLayoutId combinedMeshLayout;

    Vegetation::MeshInfo meshInfos[Vegetation::MAX_MESH_INFOS];
    Ids::IdPool meshInfoPool;
    CoreGraphics::BufferId meshInfoBuffer;

    Vegetation::GrassInfo grassInfos[Vegetation::MAX_GRASS_INFOS];
    Ids::IdPool grassInfoPool;
    CoreGraphics::BufferId grassInfoBuffer;

    CoreGraphics::ResourceTableId systemResourceTable;
    CoreGraphics::BufferId systemUniforms;

    CoreGraphics::ShaderId vegetationBaseShader;
    CoreGraphics::ShaderProgramId vegetationClearShader;
    CoreGraphics::ShaderProgramId vegetationGenerateDrawsShader;
    CoreGraphics::ShaderProgramId vegetationGrassZShader;
    CoreGraphics::ShaderProgramId vegetationGrassShader;
    CoreGraphics::ShaderProgramId vegetationMeshZShader;
    CoreGraphics::ShaderProgramId vegetationMeshShader;
    CoreGraphics::ShaderProgramId vegetationMeshVColorZShader;
    CoreGraphics::ShaderProgramId vegetationMeshVColorShader;

    CoreGraphics::BufferId grassDrawCallsBuffer;
    Util::FixedArray<CoreGraphics::BufferId> indirectGrassDrawCallsBuffer;
    CoreGraphics::BufferId grassArgumentsBuffer;
    Util::FixedArray<CoreGraphics::BufferId> indirectGrassArgumentBuffer;
    CoreGraphics::BufferId meshDrawCallsBuffer;
    Util::FixedArray<CoreGraphics::BufferId> indirectMeshDrawCallsBuffer;
    CoreGraphics::BufferId meshArgumentsBuffer;
    Util::FixedArray<CoreGraphics::BufferId> indirectMeshArgumentsBuffer;

    CoreGraphics::ResourceTableId argumentsTable;
    Util::FixedArray<CoreGraphics::ResourceTableId> indirectArgumentsTable;

    CoreGraphics::BufferId drawCountBuffer;
    Util::FixedArray<CoreGraphics::BufferId> indirectDrawCountBuffer;

    IndexT grassMaskCount = 0;
    IndexT meshMaskCount = 0;
    Ids::IdPool texturePool;

    SizeT grassDrawsThisFrame = 0;
    SizeT meshDrawsThisFrame[MAX_MESH_INFOS] = { 0 };
    SizeT meshVertexOffsets[MAX_MESH_INFOS];
    SizeT meshIndexOffsets[MAX_MESH_INFOS];
    CoreGraphics::VertexLayoutId layouts[MAX_MESH_INFOS];
    CoreGraphics::ShaderProgramId meshZPrograms[MAX_MESH_INFOS];
    CoreGraphics::ShaderProgramId meshPrograms[MAX_MESH_INFOS];

    Vegetation::VegetationMaterialUniforms materialUniforms;
    CoreGraphics::BufferId materialUniformsBuffer;

    Memory::ArenaAllocator<sizeof(Frame::FrameCode) * 3> frameOpAllocator;

} vegetationState;

static const uint VegetationDistributionRadius = 128;
static const uint MaxNumIndirectDraws = 8192;

struct GrassVertex
{
    Math::float3 position;
    Math::float3 normal;
    Math::float2 uv;
};

/*
* 	[slot=0] in vec3 position,
    [slot=1] in vec3 normal,
    [slot=2] in vec2 uv,
    [slot=3] in vec3 tangent,
    [slot=4] in vec3 binormal,
*/
struct CombinedMeshVertex
{
    Math::float3 position;
    Math::float3 normal;
    Math::float2 uv;
    Math::float3 tangent;
    Math::float3 binormal;
};

//------------------------------------------------------------------------------
/**
*/
VegetationContext::VegetationContext()
{
}

//------------------------------------------------------------------------------
/**
*/
VegetationContext::~VegetationContext()
{
}

//------------------------------------------------------------------------------
/**
*/
void
VegetationContext::Create(const VegetationSetupSettings& settings)
{
    __CreateContext();
    using namespace CoreGraphics;

    Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&__bundle, &__state);

    vegetationState.minHeight = settings.minHeight;
    vegetationState.maxHeight = settings.maxHeight;
    vegetationState.worldSize = settings.worldSize;

    CoreGraphics::BufferCreateInfo meshInfo;
    meshInfo.name = "Vegetation Mesh Info";
    meshInfo.usageFlags = CoreGraphics::ConstantBuffer;
    meshInfo.mode = CoreGraphics::HostCached;
    meshInfo.elementSize = sizeof(Vegetation::MeshInfo);
    meshInfo.size = Vegetation::MAX_MESH_INFOS;
    meshInfo.queueSupport = CoreGraphics::ComputeQueueSupport | CoreGraphics::GraphicsQueueSupport;
    vegetationState.meshInfoBuffer = CoreGraphics::CreateBuffer(meshInfo);

    CoreGraphics::BufferCreateInfo grassInfo;
    grassInfo.name = "Vegetation Grass Info";
    grassInfo.usageFlags = CoreGraphics::ConstantBuffer;
    grassInfo.mode = CoreGraphics::HostCached;
    grassInfo.elementSize = sizeof(Vegetation::GrassInfo);
    grassInfo.size = Vegetation::MAX_GRASS_INFOS;
    grassInfo.queueSupport = CoreGraphics::ComputeQueueSupport | CoreGraphics::GraphicsQueueSupport;
    vegetationState.grassInfoBuffer = CoreGraphics::CreateBuffer(grassInfo);

    CoreGraphics::BufferCreateInfo cboInfo;
    cboInfo.name = "Vegetation System Uniforms";
    cboInfo.usageFlags = CoreGraphics::ConstantBuffer;
    cboInfo.mode = CoreGraphics::DeviceAndHost;
    cboInfo.byteSize = sizeof(VegetationGenerateUniforms);
    cboInfo.queueSupport = CoreGraphics::ComputeQueueSupport | CoreGraphics::GraphicsQueueSupport;
    vegetationState.systemUniforms = CoreGraphics::CreateBuffer(cboInfo);

    CoreGraphics::BufferCreateInfo grassDrawCallsBufferInfo;
    grassDrawCallsBufferInfo.name = "Vegetation Grass Draw Calls Buffer";
    grassDrawCallsBufferInfo.usageFlags = CoreGraphics::ReadWriteBuffer | CoreGraphics::TransferBufferSource;
    grassDrawCallsBufferInfo.mode = CoreGraphics::DeviceLocal;
    grassDrawCallsBufferInfo.byteSize = sizeof(DrawIndexedCommand);
    grassDrawCallsBufferInfo.queueSupport = CoreGraphics::ComputeQueueSupport | CoreGraphics::GraphicsQueueSupport;
    vegetationState.grassDrawCallsBuffer = CoreGraphics::CreateBuffer(grassDrawCallsBufferInfo);

    vegetationState.indirectGrassDrawCallsBuffer.Resize(CoreGraphics::GetNumBufferedFrames());
    for (IndexT i = 0; i < vegetationState.indirectGrassDrawCallsBuffer.Size(); i++)
    {
        CoreGraphics::BufferCreateInfo indirectGrassDrawCallsBufferInfo;
        indirectGrassDrawCallsBufferInfo.name = "Vegetation Indirect Grass Draw Calls Buffer";
        indirectGrassDrawCallsBufferInfo.usageFlags = CoreGraphics::IndirectBuffer | CoreGraphics::TransferBufferDestination;
        indirectGrassDrawCallsBufferInfo.mode = CoreGraphics::DeviceLocal;
        indirectGrassDrawCallsBufferInfo.byteSize = sizeof(DrawIndexedCommand);
        indirectGrassDrawCallsBufferInfo.queueSupport = CoreGraphics::ComputeQueueSupport | CoreGraphics::GraphicsQueueSupport;
        vegetationState.indirectGrassDrawCallsBuffer[i] = CoreGraphics::CreateBuffer(indirectGrassDrawCallsBufferInfo);
    }

    // per-instance uniforms (fed through vertex)
    CoreGraphics::BufferCreateInfo grassArgumentsBufferInfo;
    grassArgumentsBufferInfo.name = "Vegetation Grass Arguments Buffer";
    grassArgumentsBufferInfo.usageFlags = CoreGraphics::ReadWriteBuffer | CoreGraphics::TransferBufferSource;
    grassArgumentsBufferInfo.mode = CoreGraphics::DeviceLocal;
    grassArgumentsBufferInfo.elementSize = sizeof(Vegetation::InstanceUniforms);
    grassArgumentsBufferInfo.size = MaxNumIndirectDraws;
    grassArgumentsBufferInfo.queueSupport = CoreGraphics::ComputeQueueSupport | CoreGraphics::GraphicsQueueSupport;
    vegetationState.grassArgumentsBuffer = CoreGraphics::CreateBuffer(grassArgumentsBufferInfo);

    vegetationState.indirectGrassArgumentBuffer.Resize(CoreGraphics::GetNumBufferedFrames());
    for (IndexT i = 0; i < vegetationState.indirectGrassArgumentBuffer.Size(); i++)
    {
        CoreGraphics::BufferCreateInfo indirectGrassArgumentsBufferInfo;
        indirectGrassArgumentsBufferInfo.name = "Vegetation Indirect Grass Arguments Buffer";
        indirectGrassArgumentsBufferInfo.usageFlags = CoreGraphics::ReadWriteBuffer | CoreGraphics::TransferBufferDestination;
        indirectGrassArgumentsBufferInfo.mode = CoreGraphics::DeviceLocal;
        indirectGrassArgumentsBufferInfo.elementSize = sizeof(Vegetation::InstanceUniforms);
        indirectGrassArgumentsBufferInfo.size = MaxNumIndirectDraws;
        indirectGrassArgumentsBufferInfo.queueSupport = CoreGraphics::ComputeQueueSupport | CoreGraphics::GraphicsQueueSupport;
        vegetationState.indirectGrassArgumentBuffer[i] = CoreGraphics::CreateBuffer(indirectGrassArgumentsBufferInfo);
    }

    CoreGraphics::BufferCreateInfo meshArgumentsBufferInfo;
    meshArgumentsBufferInfo.name = "Vegetation Mesh Draw Buffer";
    meshArgumentsBufferInfo.usageFlags = CoreGraphics::ReadWriteBuffer | CoreGraphics::TransferBufferSource;
    meshArgumentsBufferInfo.mode = CoreGraphics::DeviceLocal;
    meshArgumentsBufferInfo.elementSize = sizeof(DrawIndexedCommand);
    meshArgumentsBufferInfo.size = MaxNumIndirectDraws * MAX_MESH_INFOS;
    meshArgumentsBufferInfo.queueSupport = CoreGraphics::ComputeQueueSupport | CoreGraphics::GraphicsQueueSupport;
    vegetationState.meshDrawCallsBuffer = CoreGraphics::CreateBuffer(meshArgumentsBufferInfo);

    vegetationState.indirectMeshDrawCallsBuffer.Resize(CoreGraphics::GetNumBufferedFrames());
    for (IndexT i = 0; i < vegetationState.indirectMeshDrawCallsBuffer.Size(); i++)
    {
        CoreGraphics::BufferCreateInfo indirectMeshArgumentsBufferInfo;
        indirectMeshArgumentsBufferInfo.name = "Vegetation Indirect Mesh Draw Buffer";
        indirectMeshArgumentsBufferInfo.usageFlags = CoreGraphics::IndirectBuffer | CoreGraphics::TransferBufferDestination;
        indirectMeshArgumentsBufferInfo.mode = CoreGraphics::DeviceLocal;
        indirectMeshArgumentsBufferInfo.elementSize = sizeof(DrawIndexedCommand);
        indirectMeshArgumentsBufferInfo.size = MaxNumIndirectDraws * MAX_MESH_INFOS;
        indirectMeshArgumentsBufferInfo.queueSupport = CoreGraphics::ComputeQueueSupport | CoreGraphics::GraphicsQueueSupport;
        vegetationState.indirectMeshDrawCallsBuffer[i] = CoreGraphics::CreateBuffer(indirectMeshArgumentsBufferInfo);
    }

    // per-instance uniforms (fed through vertex)
    CoreGraphics::BufferCreateInfo meshDrawCallsBufferInfo;
    meshDrawCallsBufferInfo.name = "Vegetation Mesh Arguments Buffer";
    meshDrawCallsBufferInfo.usageFlags = CoreGraphics::ReadWriteBuffer | CoreGraphics::TransferBufferSource;
    meshDrawCallsBufferInfo.mode = CoreGraphics::DeviceLocal;
    meshDrawCallsBufferInfo.elementSize = sizeof(Vegetation::InstanceUniforms);
    meshDrawCallsBufferInfo.size = MaxNumIndirectDraws;
    meshDrawCallsBufferInfo.queueSupport = CoreGraphics::ComputeQueueSupport | CoreGraphics::GraphicsQueueSupport;
    vegetationState.meshArgumentsBuffer = CoreGraphics::CreateBuffer(meshDrawCallsBufferInfo);

    vegetationState.indirectMeshArgumentsBuffer.Resize(CoreGraphics::GetNumBufferedFrames());
    for (IndexT i = 0; i < vegetationState.indirectMeshArgumentsBuffer.Size(); i++)
    {
        CoreGraphics::BufferCreateInfo meshDrawCallsBufferInfo;
        meshDrawCallsBufferInfo.name = "Vegetation Indirect Mesh Arguments Buffer";
        meshDrawCallsBufferInfo.usageFlags = CoreGraphics::ReadWriteBuffer | CoreGraphics::TransferBufferDestination;
        meshDrawCallsBufferInfo.mode = CoreGraphics::DeviceLocal;
        meshDrawCallsBufferInfo.elementSize = sizeof(Vegetation::InstanceUniforms);
        meshDrawCallsBufferInfo.size = MaxNumIndirectDraws;
        meshDrawCallsBufferInfo.queueSupport = CoreGraphics::ComputeQueueSupport | CoreGraphics::GraphicsQueueSupport;
        vegetationState.indirectMeshArgumentsBuffer[i] = CoreGraphics::CreateBuffer(meshDrawCallsBufferInfo);
    }

    CoreGraphics::BufferCreateInfo indirectCountBufferInfo;
    indirectCountBufferInfo.name = "Vegetation Draw Count Buffer";
    indirectCountBufferInfo.usageFlags = CoreGraphics::ReadWriteBuffer | CoreGraphics::TransferBufferSource;
    indirectCountBufferInfo.elementSize = sizeof(Vegetation::DrawCount);
    indirectCountBufferInfo.mode = CoreGraphics::DeviceLocal;
    indirectCountBufferInfo.queueSupport = CoreGraphics::ComputeQueueSupport | CoreGraphics::GraphicsQueueSupport;
    vegetationState.drawCountBuffer = CoreGraphics::CreateBuffer(indirectCountBufferInfo);

    indirectCountBufferInfo.name = "Vegetation Indirect Count Buffer Readback";
    indirectCountBufferInfo.usageFlags = CoreGraphics::TransferBufferDestination;
    indirectCountBufferInfo.mode = CoreGraphics::HostLocal;
    vegetationState.indirectDrawCountBuffer.Resize(CoreGraphics::GetNumBufferedFrames());
    for (IndexT i = 0; i < vegetationState.indirectDrawCountBuffer.Size(); i++)
    {
        vegetationState.indirectDrawCountBuffer[i] = CoreGraphics::CreateBuffer(indirectCountBufferInfo);
    }

    CoreGraphics::BufferCreateInfo materialUniformInfo;
    materialUniformInfo.name = "Vegetation Materials Buffer";
    materialUniformInfo.elementSize = sizeof(Vegetation::VegetationMaterialUniforms);
    materialUniformInfo.usageFlags = CoreGraphics::ConstantBuffer;
    materialUniformInfo.mode = CoreGraphics::DeviceAndHost;
    vegetationState.materialUniformsBuffer = CoreGraphics::CreateBuffer(materialUniformInfo);
    memset(&vegetationState.materialUniforms, 0x0, sizeof(Vegetation::VegetationMaterialUniforms));

    vegetationState.vegetationBaseShader = CoreGraphics::ShaderGet("shd:vegetation.fxb");
    vegetationState.vegetationClearShader = CoreGraphics::ShaderGetProgram(vegetationState.vegetationBaseShader, ShaderFeatureFromString("VegetationClear"));
    vegetationState.vegetationGenerateDrawsShader = CoreGraphics::ShaderGetProgram(vegetationState.vegetationBaseShader, ShaderFeatureFromString("VegetationGenerateDraws"));
    vegetationState.vegetationGrassZShader = CoreGraphics::ShaderGetProgram(vegetationState.vegetationBaseShader, ShaderFeatureFromString("VegetationGrassDrawZ"));
    vegetationState.vegetationGrassShader = CoreGraphics::ShaderGetProgram(vegetationState.vegetationBaseShader, ShaderFeatureFromString("VegetationGrassDraw"));
    vegetationState.vegetationMeshZShader = CoreGraphics::ShaderGetProgram(vegetationState.vegetationBaseShader, ShaderFeatureFromString("VegetationMeshDrawZ"));
    vegetationState.vegetationMeshShader = CoreGraphics::ShaderGetProgram(vegetationState.vegetationBaseShader, ShaderFeatureFromString("VegetationMeshDraw"));
    vegetationState.vegetationMeshVColorZShader = CoreGraphics::ShaderGetProgram(vegetationState.vegetationBaseShader, ShaderFeatureFromString("VegetationMeshDrawVColorZ"));
    vegetationState.vegetationMeshVColorShader = CoreGraphics::ShaderGetProgram(vegetationState.vegetationBaseShader, ShaderFeatureFromString("VegetationMeshDrawVColor"));
    vegetationState.systemResourceTable = ShaderCreateResourceTable(vegetationState.vegetationBaseShader, NEBULA_SYSTEM_GROUP);

    ResourceTableSetConstantBuffer(vegetationState.systemResourceTable,
        {
            vegetationState.systemUniforms,
            Vegetation::Table_System::VegetationGenerateUniforms::SLOT,
            0,
            Vegetation::Table_System::VegetationGenerateUniforms::SIZE,
            0
        });

    ResourceTableSetConstantBuffer(vegetationState.systemResourceTable,
        {
            vegetationState.materialUniformsBuffer,
            Vegetation::Table_System::VegetationMaterialUniforms::SLOT,
            0,
            Vegetation::Table_System::VegetationMaterialUniforms::SIZE,
            0
        });


    ResourceTableSetConstantBuffer(vegetationState.systemResourceTable,
        {
            vegetationState.meshInfoBuffer,
            Vegetation::Table_System::MeshInfoUniforms::SLOT,
            0,
            sizeof(Vegetation::MeshInfo) * Vegetation::MAX_MESH_INFOS,
            0
        });

    ResourceTableSetConstantBuffer(vegetationState.systemResourceTable,
        {
            vegetationState.grassInfoBuffer,
            Vegetation::Table_System::GrassInfoUniforms::SLOT,
            0,
            sizeof(Vegetation::GrassInfo) * Vegetation::MAX_GRASS_INFOS,
            0
        });

    ResourceTableSetRWBuffer(vegetationState.systemResourceTable,
        {
            vegetationState.grassDrawCallsBuffer,
            Vegetation::Table_System::IndirectGrassDrawBuffer::SLOT,
            0,
            BufferGetByteSize(vegetationState.grassDrawCallsBuffer),
            0
        });

    ResourceTableSetRWBuffer(vegetationState.systemResourceTable,
        {
            vegetationState.meshDrawCallsBuffer,
            Vegetation::Table_System::IndirectMeshDrawBuffer::SLOT,
            0,
            BufferGetByteSize(vegetationState.meshDrawCallsBuffer),
            0
        });

    ResourceTableSetRWBuffer(vegetationState.systemResourceTable,
        {
            vegetationState.drawCountBuffer,
            Vegetation::Table_System::DrawCount::SLOT,
            0,
            BufferGetByteSize(vegetationState.drawCountBuffer),
            0
        });

    ResourceTableCommitChanges(vegetationState.systemResourceTable);

    vegetationState.argumentsTable = ShaderCreateResourceTable(vegetationState.vegetationBaseShader, NEBULA_BATCH_GROUP);
    ResourceTableSetRWBuffer(vegetationState.argumentsTable,
        {
            vegetationState.grassArgumentsBuffer,
            Vegetation::Table_Batch::InstanceGrassArguments::SLOT,
            0,
            BufferGetByteSize(vegetationState.grassArgumentsBuffer),
            0
        });

    ResourceTableSetRWBuffer(vegetationState.argumentsTable,
        {
            vegetationState.meshArgumentsBuffer,
            Vegetation::Table_Batch::InstanceMeshArguments::SLOT,
            0,
            BufferGetByteSize(vegetationState.meshArgumentsBuffer),
            0
        });

    ResourceTableCommitChanges(vegetationState.argumentsTable);

    vegetationState.indirectArgumentsTable.Resize(CoreGraphics::GetNumBufferedFrames());
    for (IndexT i = 0; i < vegetationState.indirectArgumentsTable.Size(); i++)
    {
        vegetationState.indirectArgumentsTable[i] = ShaderCreateResourceTable(vegetationState.vegetationBaseShader, NEBULA_BATCH_GROUP);

        ResourceTableSetRWBuffer(vegetationState.indirectArgumentsTable[i],
            {
                vegetationState.indirectGrassArgumentBuffer[i],
                Vegetation::Table_Batch::InstanceGrassArguments::SLOT,
                0,
                BufferGetByteSize(vegetationState.indirectGrassArgumentBuffer[i]),
                0
            });

        ResourceTableSetRWBuffer(vegetationState.indirectArgumentsTable[i],
            {
                vegetationState.indirectMeshArgumentsBuffer[i],
                Vegetation::Table_Batch::InstanceMeshArguments::SLOT,
                0,
                BufferGetByteSize(vegetationState.indirectMeshArgumentsBuffer[i]),
                0
            });

        ResourceTableCommitChanges(vegetationState.indirectArgumentsTable[i]);
    }

    Frame::FrameCode* clear = vegetationState.frameOpAllocator.Alloc<Frame::FrameCode>();
    clear->domain = CoreGraphics::BarrierDomain::Global;
    clear->queue = CoreGraphics::ComputeQueueType;
    clear->bufferDeps.Add(vegetationState.drawCountBuffer,
                        {
                            "Draw Count Buffer",
                            CoreGraphics::PipelineStage::ComputeShaderWrite,
                            CoreGraphics::BufferSubresourceInfo()
                        });
    clear->bufferDeps.Add(vegetationState.grassDrawCallsBuffer,
                        {
                            "Grass Draw Buffer",
                            CoreGraphics::PipelineStage::ComputeShaderWrite,
                            CoreGraphics::BufferSubresourceInfo()
                        });
    clear->func = [](const CoreGraphics::CmdBufferId cmdBuf, const IndexT frame, const IndexT bufferIndex)
    {
        CmdSetShaderProgram(cmdBuf, vegetationState.vegetationClearShader);
        CmdSetResourceTable(cmdBuf, vegetationState.systemResourceTable, NEBULA_SYSTEM_GROUP, ComputePipeline, nullptr);

        CmdDispatch(cmdBuf, 1, 1, 1);
    };

    Frame::FrameCode* drawGeneration = vegetationState.frameOpAllocator.Alloc<Frame::FrameCode>();
    drawGeneration->domain = CoreGraphics::BarrierDomain::Global;
    drawGeneration->queue = CoreGraphics::ComputeQueueType;
    drawGeneration->bufferDeps.Add(vegetationState.grassDrawCallsBuffer,
                            {
                                "Grass Draw Buffer",
                                CoreGraphics::PipelineStage::ComputeShaderWrite,
                                CoreGraphics::BufferSubresourceInfo()
                            });
    drawGeneration->bufferDeps.Add(vegetationState.meshDrawCallsBuffer,
                            {
                                "Mesh Draw Buffer",
                                CoreGraphics::PipelineStage::ComputeShaderWrite,
                                CoreGraphics::BufferSubresourceInfo()
                            });
    drawGeneration->bufferDeps.Add(vegetationState.grassArgumentsBuffer,
                            {
                                "Grass Uniform Buffer",
                                CoreGraphics::PipelineStage::ComputeShaderWrite,
                                CoreGraphics::BufferSubresourceInfo()
                            });
    drawGeneration->bufferDeps.Add(vegetationState.meshArgumentsBuffer,
                            {
                                "Mesh Uniform Buffer",
                                CoreGraphics::PipelineStage::ComputeShaderWrite,
                                CoreGraphics::BufferSubresourceInfo()
                            });
    drawGeneration->bufferDeps.Add(vegetationState.drawCountBuffer,
                            {
                                "Draw Count Buffer",
                                CoreGraphics::PipelineStage::ComputeShaderWrite,
                                CoreGraphics::BufferSubresourceInfo()
                            });

    drawGeneration->func = [](const CoreGraphics::CmdBufferId cmdBuf, const IndexT frame, const IndexT bufferIndex)
    {
        CmdSetShaderProgram(cmdBuf, vegetationState.vegetationGenerateDrawsShader);
        CmdSetResourceTable(cmdBuf, vegetationState.systemResourceTable, NEBULA_SYSTEM_GROUP, ComputePipeline, nullptr);
        CmdSetResourceTable(cmdBuf, vegetationState.argumentsTable, NEBULA_BATCH_GROUP, ComputePipeline, nullptr);

        CmdDispatch(cmdBuf, VegetationDistributionRadius / 64, VegetationDistributionRadius, 1);
    };

    Frame::FrameCode* readBack = vegetationState.frameOpAllocator.Alloc<Frame::FrameCode>();
    readBack->domain = CoreGraphics::BarrierDomain::Global;
    readBack->queue = CoreGraphics::ComputeQueueType;
    readBack->bufferDeps.Add(vegetationState.drawCountBuffer,
                        {
                            "Mesh Draw Buffer",
                            CoreGraphics::PipelineStage::TransferRead,
                            CoreGraphics::BufferSubresourceInfo()
                        });
    readBack->func = [](const CoreGraphics::CmdBufferId cmdBuf, const IndexT frame, const IndexT bufferIndex)
    {
         CmdBarrier(cmdBuf,
            PipelineStage::HostRead,
            PipelineStage::TransferWrite,
            BarrierDomain::Global,
            {
                BufferBarrierInfo
                {
                    vegetationState.indirectDrawCountBuffer[bufferIndex],
                    CoreGraphics::BufferSubresourceInfo()
                },
            });

        BufferCopy from, to;
        from.offset = 0;
        to.offset = 0;
        CmdCopy(cmdBuf, vegetationState.drawCountBuffer, { from }, vegetationState.indirectDrawCountBuffer[bufferIndex], { to }, BufferGetByteSize(vegetationState.drawCountBuffer));

        CmdBarrier(cmdBuf,
            PipelineStage::TransferWrite,
            PipelineStage::HostRead,
            BarrierDomain::Global,
            {
                BufferBarrierInfo
                {
                    vegetationState.indirectDrawCountBuffer[bufferIndex],
                    CoreGraphics::BufferSubresourceInfo()
                },
            });
    };
    Frame::AddSubgraph("Vegetation Generate Draws", { clear, drawGeneration, readBack });

    Frame::FrameCode* prepass = vegetationState.frameOpAllocator.Alloc<Frame::FrameCode>();
    prepass->domain = CoreGraphics::BarrierDomain::Pass;
    prepass->bufferDeps.Add(vegetationState.grassDrawCallsBuffer,
                            {
                                "Grass Draw Buffer",
                                CoreGraphics::PipelineStage::Indirect,
                                CoreGraphics::BufferSubresourceInfo()
                            });
    prepass->bufferDeps.Add(vegetationState.meshDrawCallsBuffer,
                            {
                                "Mesh Draw Buffer",
                                CoreGraphics::PipelineStage::Indirect,
                                CoreGraphics::BufferSubresourceInfo()
                            });
    prepass->bufferDeps.Add(vegetationState.grassArgumentsBuffer,
                            {
                                "Grass Uniform Buffer",
                                CoreGraphics::PipelineStage::UniformGraphics,
                                CoreGraphics::BufferSubresourceInfo()
                            });
    prepass->bufferDeps.Add(vegetationState.meshArgumentsBuffer,
                            {
                                "Mesh Uniform Buffer",
                                CoreGraphics::PipelineStage::UniformGraphics,
                                CoreGraphics::BufferSubresourceInfo()
                            });
    prepass->func = [](const CoreGraphics::CmdBufferId cmdBuf, const IndexT frame, const IndexT bufferIndex)
    {
        if (vegetationState.grassDrawsThisFrame > 0)
        {
            CmdSetShaderProgram(cmdBuf, vegetationState.vegetationGrassZShader);

            // setup mesh
            CmdSetPrimitiveTopology(cmdBuf, CoreGraphics::PrimitiveTopology::TriangleList);
            CmdSetVertexLayout(cmdBuf, vegetationState.grassLayout);
            CmdSetVertexBuffer(cmdBuf, 0, vegetationState.grassVbo, 0);
            CmdSetIndexBuffer(cmdBuf, IndexType::Index16, vegetationState.grassIbo, 0);

            // set graphics pipeline
            CmdSetGraphicsPipeline(cmdBuf);

            // bind resource table
            CmdSetResourceTable(cmdBuf, vegetationState.systemResourceTable, NEBULA_SYSTEM_GROUP, GraphicsPipeline, nullptr);
            CmdSetResourceTable(cmdBuf, vegetationState.indirectArgumentsTable[bufferIndex], NEBULA_BATCH_GROUP, GraphicsPipeline, nullptr);

            // draw
            CmdDrawIndirectIndexed(cmdBuf, vegetationState.indirectGrassDrawCallsBuffer[bufferIndex], 0, 1, sizeof(Vegetation::DrawIndexedCommand));
        }

        for (IndexT i = 0; i < MAX_MESH_INFOS; i++)
        {
            if (vegetationState.meshDrawsThisFrame[i] > 0)
            {
                CmdSetShaderProgram(cmdBuf, vegetationState.meshZPrograms[i]);

                // setup mesh
                CmdSetPrimitiveTopology(cmdBuf, CoreGraphics::PrimitiveTopology::TriangleList);
                CmdSetVertexLayout(cmdBuf, vegetationState.layouts[i]);
                CmdSetVertexBuffer(cmdBuf, 0, CoreGraphics::GetVertexBuffer(), vegetationState.meshVertexOffsets[i]);
                CmdSetIndexBuffer(cmdBuf, IndexType::Index16, CoreGraphics::GetIndexBuffer(), vegetationState.meshIndexOffsets[i]);

                // set graphics pipeline
                CmdSetGraphicsPipeline(cmdBuf);

                // bind resources
                CmdSetResourceTable(cmdBuf, vegetationState.systemResourceTable, NEBULA_SYSTEM_GROUP, GraphicsPipeline, nullptr);
                CmdSetResourceTable(cmdBuf, vegetationState.indirectArgumentsTable[bufferIndex], NEBULA_BATCH_GROUP, GraphicsPipeline, nullptr);

                // draw
                CmdDrawIndirectIndexed(cmdBuf, vegetationState.indirectMeshDrawCallsBuffer[bufferIndex], i * MAX_MESH_INFOS, vegetationState.meshDrawsThisFrame[i], sizeof(Vegetation::DrawIndexedCommand));
            }
        }
    };
    Frame::AddSubgraph("Vegetation Prepass", { prepass });

    Frame::FrameCode* render = vegetationState.frameOpAllocator.Alloc<Frame::FrameCode>();
    render->domain = CoreGraphics::BarrierDomain::Pass;
    render->bufferDeps.Add(vegetationState.grassDrawCallsBuffer,
                            {
                                "Grass Draw Buffer",
                                CoreGraphics::PipelineStage::Indirect,
                                CoreGraphics::BufferSubresourceInfo()
                            });
    render->bufferDeps.Add(vegetationState.meshDrawCallsBuffer,
                            {
                                "Mesh Draw Buffer",
                                CoreGraphics::PipelineStage::Indirect,
                                CoreGraphics::BufferSubresourceInfo()
                            });
    render->bufferDeps.Add(vegetationState.grassArgumentsBuffer,
                            {
                                "Grass Uniform Buffer",
                                CoreGraphics::PipelineStage::UniformGraphics,
                                CoreGraphics::BufferSubresourceInfo()
                            });
    render->bufferDeps.Add(vegetationState.meshArgumentsBuffer,
                            {
                                "Mesh Uniform Buffer",
                                CoreGraphics::PipelineStage::UniformGraphics,
                                CoreGraphics::BufferSubresourceInfo()
                            });
    render->func = [](const CoreGraphics::CmdBufferId cmdBuf, const IndexT frame, const IndexT bufferIndex)
    {
        if (vegetationState.grassDrawsThisFrame > 0)
        {
            CmdSetShaderProgram(cmdBuf, vegetationState.vegetationGrassShader);

            // setup mesh
            CmdSetPrimitiveTopology(cmdBuf, CoreGraphics::PrimitiveTopology::TriangleList);
            CmdSetVertexLayout(cmdBuf, vegetationState.grassLayout);
            CmdSetVertexBuffer(cmdBuf, 0, vegetationState.grassVbo, 0);
            CmdSetIndexBuffer(cmdBuf, IndexType::Index16, vegetationState.grassIbo, 0);

            // set graphics pipeline
            CmdSetGraphicsPipeline(cmdBuf);

            // bind resource table
            CmdSetResourceTable(cmdBuf, vegetationState.systemResourceTable, NEBULA_SYSTEM_GROUP, GraphicsPipeline, nullptr);
            CmdSetResourceTable(cmdBuf, vegetationState.indirectArgumentsTable[bufferIndex], NEBULA_BATCH_GROUP, GraphicsPipeline, nullptr);

            // draw
            CmdDrawIndirectIndexed(cmdBuf, vegetationState.indirectGrassDrawCallsBuffer[bufferIndex], 0, 1, sizeof(Vegetation::DrawIndexedCommand));
        }

        for (IndexT i = 0; i < MAX_MESH_INFOS; i++)
        {
            if (vegetationState.meshDrawsThisFrame[i] > 0)
            {
                CmdSetShaderProgram(cmdBuf, vegetationState.meshPrograms[i]);

                // setup mesh
                CmdSetPrimitiveTopology(cmdBuf, CoreGraphics::PrimitiveTopology::TriangleList);
                CmdSetVertexLayout(cmdBuf, vegetationState.layouts[i]);
                CmdSetVertexBuffer(cmdBuf, 0, CoreGraphics::GetVertexBuffer(), vegetationState.meshVertexOffsets[i]);
                CmdSetIndexBuffer(cmdBuf, IndexType::Index16, CoreGraphics::GetIndexBuffer(), vegetationState.meshIndexOffsets[i]);

                // set graphics pipeline
                CmdSetGraphicsPipeline(cmdBuf);

                // bind resources
                CmdSetResourceTable(cmdBuf, vegetationState.systemResourceTable, NEBULA_SYSTEM_GROUP, GraphicsPipeline, nullptr);
                CmdSetResourceTable(cmdBuf, vegetationState.indirectArgumentsTable[bufferIndex], NEBULA_BATCH_GROUP, GraphicsPipeline, nullptr);

                // draw
                CmdDrawIndirectIndexed(cmdBuf, vegetationState.indirectMeshDrawCallsBuffer[bufferIndex], i * MAX_MESH_INFOS, vegetationState.meshDrawsThisFrame[i], sizeof(Vegetation::DrawIndexedCommand));
            }
        }
    };
    Frame::AddSubgraph("Vegetation Render", { render });

    // Gah, we need to copy from the GPU to the indirect buffers after we render
    Frame::FrameCode* copy = vegetationState.frameOpAllocator.Alloc<Frame::FrameCode>();
    copy->domain = CoreGraphics::BarrierDomain::Global;
    copy->func = [](const CoreGraphics::CmdBufferId cmdBuf, const IndexT frame, const IndexT bufferIndex)
    {
        CoreGraphics::BufferCopy from, to;
        from.offset = 0;
        to.offset = 0;
        CmdCopy(cmdBuf, vegetationState.grassArgumentsBuffer, { from }, vegetationState.indirectGrassArgumentBuffer[bufferIndex], { to }, BufferGetByteSize(vegetationState.grassArgumentsBuffer));
        CmdCopy(cmdBuf, vegetationState.grassDrawCallsBuffer, { from }, vegetationState.indirectGrassDrawCallsBuffer[bufferIndex], { to }, BufferGetByteSize(vegetationState.grassDrawCallsBuffer));
        CmdCopy(cmdBuf, vegetationState.meshArgumentsBuffer, { from }, vegetationState.indirectMeshArgumentsBuffer[bufferIndex], { to }, BufferGetByteSize(vegetationState.meshArgumentsBuffer));
        CmdCopy(cmdBuf, vegetationState.meshDrawCallsBuffer, { from }, vegetationState.indirectMeshDrawCallsBuffer[bufferIndex], { to }, BufferGetByteSize(vegetationState.meshDrawCallsBuffer));

    };
    Frame::AddSubgraph("Vegetation Copy Indirect", { copy });
}

//------------------------------------------------------------------------------
/**
*/
void
VegetationContext::Discard()
{
}

//------------------------------------------------------------------------------
/**
*/
void
VegetationContext::Setup(Resources::ResourceName heightMap, SizeT numGrassPlanesPerTuft, float grassPatchRadius)
{
    using namespace CoreGraphics;
    vegetationState.heightMap = Resources::CreateResource(heightMap, "Vegetation", nullptr, nullptr, true);
    vegetationState.numGrassBlades = numGrassPlanesPerTuft;
    Util::FixedArray<GrassVertex> vertices(numGrassPlanesPerTuft * 4);
    Util::FixedArray<ushort> indices(numGrassPlanesPerTuft * 6);

    // calculate center points for grass patches by angle
    float angle = 0.0f;
    float angleIncrement = 360.0f / vegetationState.numGrassBlades;
    int index = 0;
    int vertex = 0;
    for (SizeT i = 0; i < vegetationState.numGrassBlades; i++, index += 6, vertex += 4)
    {
        float x = Math::cos(Math::deg2rad(angle));
        float z = Math::sin(Math::deg2rad(angle));

        // calculate a plane at the point facing the center
        Math::plane plane(Math::point(0, 0, 0), Math::point(0, 0, 0) - Math::point(x, 0, z));

        // get the plane components
        Math::point center = get_point(plane);
        Math::vector normal = get_normal(plane);

        // create perpendicular 2d vector
        Math::vector perp = Math::vector(normal.z, 0, -normal.x);

        // extrude vertex positions using perp
        (center + -perp * grassPatchRadius).storeu(vertices[vertex].position.v);
        vertices[vertex].position.y = 0.0f;
        (center + -perp * grassPatchRadius).storeu(vertices[vertex + 1].position.v);
        vertices[vertex + 1].position.y = grassPatchRadius;
        (center + perp * grassPatchRadius).storeu(vertices[vertex + 2].position.v);
        vertices[vertex + 2].position.y = 0.0f;
        (center + perp * grassPatchRadius).storeu(vertices[vertex + 3].position.v);
        vertices[vertex + 3].position.y = grassPatchRadius;

        // store normals
        normal.storeu(vertices[vertex].normal.v);
        normal.storeu(vertices[vertex + 1].normal.v);
        normal.storeu(vertices[vertex + 2].normal.v);
        normal.storeu(vertices[vertex + 3].normal.v);

        // store uvs
        vertices[vertex].uv = Math::float2{ 0.0f, 0.0f };
        vertices[vertex + 1].uv = Math::float2{ 0.0f, 1.0f };
        vertices[vertex + 2].uv = Math::float2{ 1.0f, 0.0f };
        vertices[vertex + 3].uv = Math::float2{ 1.0f, 1.0f };

        // tri 0
        indices[index] = vertex;
        indices[index + 1] = vertex + 1;
        indices[index + 2] = vertex + 2;

        // tri 1
        indices[index + 3] = vertex + 1;
        indices[index + 4] = vertex + 3;
        indices[index + 5] = vertex + 2;

        angle += angleIncrement;
    }

    CoreGraphics::VertexLayoutCreateInfo vloInfo;
    vloInfo.comps =
    {
        // per vertex geometry data
        VertexComponent(0, VertexComponent::Float3),
        VertexComponent(1, VertexComponent::Float3),
        VertexComponent(2, VertexComponent::Float2),
    };
    vegetationState.grassLayout = CoreGraphics::CreateVertexLayout(vloInfo);

    CoreGraphics::BufferCreateInfo vboInfo;
    vboInfo.name = "Grass VBO";
    vboInfo.usageFlags = CoreGraphics::VertexBuffer;
    vboInfo.elementSize = sizeof(GrassVertex);
    vboInfo.size = numGrassPlanesPerTuft * 4;
    vboInfo.mode = CoreGraphics::DeviceLocal;
    vboInfo.data = vertices.Begin();
    vboInfo.dataSize = vertices.ByteSize();
    vegetationState.grassVbo = CoreGraphics::CreateBuffer(vboInfo);

    CoreGraphics::BufferCreateInfo iboInfo;
    iboInfo.name = "Grass IBO";
    iboInfo.usageFlags = CoreGraphics::IndexBuffer;
    iboInfo.elementSize = CoreGraphics::IndexType::SizeOf(CoreGraphics::IndexType::Index16);
    iboInfo.size = numGrassPlanesPerTuft * 6;
    iboInfo.mode = CoreGraphics::DeviceLocal;
    iboInfo.data = indices.Begin();
    iboInfo.dataSize = indices.ByteSize();
    vegetationState.grassIbo = CoreGraphics::CreateBuffer(iboInfo);

    vloInfo.comps =
    {
        // per vertex geometry data
        VertexComponent(VertexComponent::IndexName::Position, VertexComponent::Float3),
        VertexComponent(VertexComponent::IndexName::Normal, VertexComponent::Byte4N),
        VertexComponent(VertexComponent::IndexName::TexCoord1, VertexComponent::Float2),
        VertexComponent(VertexComponent::IndexName::Tangent, VertexComponent::Byte4N),
        VertexComponent(VertexComponent::IndexName::Binormal, VertexComponent::Byte4N),
        VertexComponent(VertexComponent::IndexName::Color, VertexComponent::UByte4N),
    };
    vegetationState.combinedMeshLayoutWithColor = CoreGraphics::CreateVertexLayout(vloInfo);

    vloInfo.comps =
    {
        // per vertex geometry data
        VertexComponent(VertexComponent::IndexName::Position, VertexComponent::Float3),
        VertexComponent(VertexComponent::IndexName::Normal, VertexComponent::Byte4N),
        VertexComponent(VertexComponent::IndexName::TexCoord1, VertexComponent::Float2),
        VertexComponent(VertexComponent::IndexName::Tangent, VertexComponent::Byte4N),
        VertexComponent(VertexComponent::IndexName::Binormal, VertexComponent::Byte4N),
    };
    vegetationState.combinedMeshLayout = CoreGraphics::CreateVertexLayout(vloInfo);
}

//------------------------------------------------------------------------------
/**
*/
void
VegetationContext::SetupGrass(const Graphics::GraphicsEntityId id, const VegetationGrassSetup& setup)
{
    Graphics::ContextEntityId cid = GetContextId(id.id);
    CoreGraphics::TextureId mask = Resources::CreateResource(setup.mask, "Vegetation", nullptr, nullptr, true);
    vegetationAllocator.Set<Vegetation_Mask>(cid.id, mask);

    CoreGraphics::TextureId& albedo = vegetationAllocator.Get<Vegetation_Albedos>(cid.id);
    CoreGraphics::TextureId& normal = vegetationAllocator.Get<Vegetation_Normals>(cid.id);
    CoreGraphics::TextureId& material = vegetationAllocator.Get<Vegetation_Materials>(cid.id);
    CoreGraphics::MeshId& mesh = vegetationAllocator.Get<Vegetation_Meshes>(cid.id);
    uint& infoIndex = vegetationAllocator.Get<Vegetation_GPUInfo>(cid.id);
    float& slopeThreshold = vegetationAllocator.Get<Vegetation_SlopeThreshold>(cid.id);
    float& heightThreshold = vegetationAllocator.Get<Vegetation_HeightThreshold>(cid.id);
    IndexT& textureIndex = vegetationAllocator.Get<Vegetation_TextureIndex>(cid.id);
    VegetationType& type = vegetationAllocator.Get<Vegetation_Type>(cid.id);

    infoIndex = vegetationState.grassInfoPool.Alloc();
    n_assert(infoIndex < Vegetation::MAX_MESH_INFOS);
    GrassInfo& info = vegetationState.grassInfos[infoIndex];

    info.used = true;
    albedo = Resources::CreateResource(setup.albedo, "Vegetation", nullptr, nullptr, true);
    normal = Resources::CreateResource(setup.normals, "Vegetation", nullptr, nullptr, true);
    material = Resources::CreateResource(setup.material, "Vegetation", nullptr, nullptr, true);
    mesh = CoreGraphics::InvalidMeshId;
    slopeThreshold = setup.slopeThreshold;
    heightThreshold = setup.heightThreshold;
    textureIndex = vegetationState.texturePool.Alloc();
    type = VegetationType::GrassType;

    info.distribution = 0.0f;
    info.heightThreshold = heightThreshold;
    info.slopeThreshold = slopeThreshold;
    info.textureIndex = textureIndex;

    vegetationState.materialUniforms.VegetationAlbedo[textureIndex / 4][textureIndex % 4]    = CoreGraphics::TextureGetBindlessHandle(albedo);
    vegetationState.materialUniforms.VegetationNormal[textureIndex / 4][textureIndex % 4]    = CoreGraphics::TextureGetBindlessHandle(normal);
    vegetationState.materialUniforms.VegetationMaterial[textureIndex / 4][textureIndex % 4]  = CoreGraphics::TextureGetBindlessHandle(material);
    vegetationState.materialUniforms.VegetationMasks[textureIndex / 4][textureIndex % 4]     = CoreGraphics::TextureGetBindlessHandle(mask);

    CoreGraphics::BufferUpdate(vegetationState.materialUniformsBuffer, vegetationState.materialUniforms);
}

//------------------------------------------------------------------------------
/**
*/
void
VegetationContext::SetupMesh(const Graphics::GraphicsEntityId id, const VegetationMeshSetup& setup)
{
    Graphics::ContextEntityId cid = GetContextId(id.id);
    CoreGraphics::TextureId mask = Resources::CreateResource(setup.mask, "Vegetation", nullptr, nullptr, true);
    vegetationAllocator.Set<Vegetation_Mask>(cid.id, mask);

    CoreGraphics::TextureId& albedo = vegetationAllocator.Get<Vegetation_Albedos>(cid.id);
    CoreGraphics::TextureId& normal = vegetationAllocator.Get<Vegetation_Normals>(cid.id);
    CoreGraphics::TextureId& material = vegetationAllocator.Get<Vegetation_Materials>(cid.id);
    CoreGraphics::MeshId& mesh = vegetationAllocator.Get<Vegetation_Meshes>(cid.id);
    uint& infoIndex = vegetationAllocator.Get<Vegetation_GPUInfo>(cid.id);
    float& slopeThreshold = vegetationAllocator.Get<Vegetation_SlopeThreshold>(cid.id);
    float& heightThreshold = vegetationAllocator.Get<Vegetation_HeightThreshold>(cid.id);
    IndexT& textureIndex = vegetationAllocator.Get<Vegetation_TextureIndex>(cid.id);
    VegetationType& type = vegetationAllocator.Get<Vegetation_Type>(cid.id);

    albedo = Resources::CreateResource(setup.albedo, "Vegetation", nullptr, nullptr, true);
    normal = Resources::CreateResource(setup.normals, "Vegetation", nullptr, nullptr, true);
    material = Resources::CreateResource(setup.material, "Vegetation", nullptr, nullptr, true);

    CoreGraphics::MeshLoader::StreamMeshLoadMetaData metaData;
    metaData.copySource = true;
    mesh = CoreGraphics::InvalidMeshId;

    Models::ModelId model = Resources::CreateResource(setup.mesh, metaData, "Vegetation", nullptr, nullptr, true);
    const Util::Array<Models::ModelNode*>& nodes = Models::ModelGetNodes(model);
    Util::Array<float> lodDistances;
    for (auto node : nodes)
    {
        if (node->GetType() == Models::NodeType::PrimitiveNodeType)
        {
            auto pnode = static_cast<Models::PrimitiveNode*>(node);
            if (mesh == CoreGraphics::InvalidMeshId)
                mesh = pnode->GetMesh();

            float minDist, maxDist;
            pnode->GetLODDistances(minDist, maxDist);
            lodDistances.Append(maxDist);
        }
    }

    infoIndex = vegetationState.meshInfoPool.Alloc();
    n_assert(infoIndex < Vegetation::MAX_MESH_INFOS);
    MeshInfo& info = vegetationState.meshInfos[infoIndex];
    info.used = true;

    // get primitive groups so we can update the mesh info
    const Util::Array<CoreGraphics::PrimitiveGroup>& groups = CoreGraphics::MeshGetPrimitiveGroups(mesh);
    uint baseVertexOffset = CoreGraphics::MeshGetVertexOffset(mesh, 0);
    uint baseIndexOffset = CoreGraphics::MeshGetIndexOffset(mesh);

    // make sure the mesh is valid for vegetation rendering
    const Util::Array<CoreGraphics::VertexComponent>& components = CoreGraphics::VertexLayoutGetComponents(MeshGetVertexLayout(mesh));
    n_assert(components.Size() >= 5);
    if (components.Size() == 6) // with vertex colors
    {
        n_assert(components[0].GetFormat() == CoreGraphics::VertexComponent::Float3);
        n_assert(components[1].GetFormat() == CoreGraphics::VertexComponent::Byte4N);
        n_assert(components[2].GetFormat() == CoreGraphics::VertexComponent::Float2);
        n_assert(components[3].GetFormat() == CoreGraphics::VertexComponent::Byte4N);
        n_assert(components[4].GetFormat() == CoreGraphics::VertexComponent::Byte4N);
        n_assert(components[5].GetFormat() == CoreGraphics::VertexComponent::UByte4N);
        vegetationState.layouts[infoIndex] = vegetationState.combinedMeshLayoutWithColor;
        vegetationState.meshZPrograms[infoIndex] = vegetationState.vegetationMeshVColorZShader;
        vegetationState.meshPrograms[infoIndex] = vegetationState.vegetationMeshVColorShader;
    }
    else // without
    {
        n_assert(components[0].GetFormat() == CoreGraphics::VertexComponent::Float3);
        n_assert(components[1].GetFormat() == CoreGraphics::VertexComponent::Byte4N);
        n_assert(components[2].GetFormat() == CoreGraphics::VertexComponent::Float2);
        n_assert(components[3].GetFormat() == CoreGraphics::VertexComponent::Byte4N);
        n_assert(components[4].GetFormat() == CoreGraphics::VertexComponent::Byte4N);
        vegetationState.layouts[infoIndex] = vegetationState.combinedMeshLayout;
        vegetationState.meshZPrograms[infoIndex] = vegetationState.vegetationMeshZShader;
        vegetationState.meshPrograms[infoIndex] = vegetationState.vegetationMeshShader;
    }

    info.indexCount = 0;
    info.vertexCount = 0;
    info.numLods = groups.Size();

    vegetationState.meshVertexOffsets[infoIndex] = baseVertexOffset;
    vegetationState.meshIndexOffsets[infoIndex] = baseIndexOffset;

    float distances[] =
    {
        10,
        25,
        50,
        100,
        500,
        1000
    };
    //SizeT size = CoreGraphics::VertexLayoutGetSize(MeshGetVertexLayout(mesh));
    for (IndexT i = 0; i < groups.Size(); i++)
    {
        if (lodDistances[i] == FLT_MAX)
            info.lodDistances[i] = distances[i] * distances[i];
        else
            info.lodDistances[i] = lodDistances[i] * lodDistances[i];
        info.lodIndexOffsets[i] = groups[i].GetBaseIndex();
        info.lodVertexOffsets[i] = groups[i].GetBaseVertex();
        info.lodIndexCount[i] = groups[i].GetNumIndices();
        info.lodVertexCount[i] = groups[i].GetNumVertices();
        info.indexCount += groups[i].GetNumIndices();
        info.vertexCount += groups[i].GetNumVertices();
    }
    slopeThreshold = setup.slopeThreshold;
    heightThreshold = setup.heightThreshold;
    textureIndex = vegetationState.texturePool.Alloc();
    type = VegetationType::MeshType;

    info.slopeThreshold = slopeThreshold;
    info.heightThreshold = heightThreshold;
    info.textureIndex = textureIndex;
    info.distribution = 0.0f;

    vegetationState.materialUniforms.VegetationAlbedo[textureIndex / 4][textureIndex % 4] = CoreGraphics::TextureGetBindlessHandle(albedo);
    vegetationState.materialUniforms.VegetationNormal[textureIndex / 4][textureIndex % 4] = CoreGraphics::TextureGetBindlessHandle(normal);
    vegetationState.materialUniforms.VegetationMaterial[textureIndex / 4][textureIndex % 4] = CoreGraphics::TextureGetBindlessHandle(material);
    vegetationState.materialUniforms.VegetationMasks[textureIndex / 4][textureIndex % 4] = CoreGraphics::TextureGetBindlessHandle(mask);

    CoreGraphics::BufferUpdate(vegetationState.materialUniformsBuffer, vegetationState.materialUniforms);
}

//------------------------------------------------------------------------------
/**
*/
void
VegetationContext::UpdateViewResources(const Ptr<Graphics::View>& view, const Graphics::FrameContext& ctx)
{
    Graphics::CameraSettings settings = Graphics::CameraContext::GetSettings(view->GetCamera());
    Math::mat4 cameraTransform = Math::inverse(Graphics::CameraContext::GetView(view->GetCamera()));
    VegetationGenerateUniforms uniforms;
    cameraTransform.position.store(uniforms.CameraPosition);
    cameraTransform.z_axis.store(uniforms.CameraForward);
    uniforms.HeightMap = CoreGraphics::TextureGetBindlessHandle(vegetationState.heightMap);
    uniforms.WorldSize[0] = vegetationState.worldSize.x;
    uniforms.WorldSize[1] = vegetationState.worldSize.y;
    uniforms.InvWorldSize[0] = 1.0f / vegetationState.worldSize.x;
    uniforms.InvWorldSize[1] = 1.0f / vegetationState.worldSize.y;
    uniforms.GenerateQuadSize[0] = VegetationDistributionRadius;
    uniforms.GenerateQuadSize[1] = VegetationDistributionRadius;
    Math::vec4 forwardxz = cameraTransform.z_axis;
    forwardxz.y = 0.0f;
    forwardxz = normalize(forwardxz);
    uniforms.GenerateDirection[0] = forwardxz.x;
    uniforms.GenerateDirection[1] = forwardxz.z;
    uniforms.DensityFactor = 1.0f;
    uniforms.MinHeight = vegetationState.minHeight;
    uniforms.MaxHeight = vegetationState.maxHeight;
    uniforms.NumGrassBlades = vegetationState.numGrassBlades;
    uniforms.HeightMapSize[0] = CoreGraphics::TextureGetDimensions(vegetationState.heightMap).width;
    uniforms.HeightMapSize[1] = CoreGraphics::TextureGetDimensions(vegetationState.heightMap).height;
    uniforms.MaxRange = 100.0f * 100.0f;
    uniforms.Fov = Math::cos(settings.GetFov() + 5.0_rad);

    uniforms.NumGrassTypes = 0;
    uniforms.NumMeshTypes = 0;
    const Util::Array<VegetationType>& types = vegetationAllocator.GetArray<Vegetation_Type>();
    for (IndexT i = 0; i < types.Size(); i++)
    {
        switch (types[i])
        {
        case VegetationType::GrassType:
        {
            uniforms.NumGrassTypes++;
            break;
        }
        case VegetationType::MeshType:
        {
            uniforms.NumMeshTypes++;
            break;
        }
        }
    }

    // update uniform buffer
    CoreGraphics::BufferUpdate(vegetationState.systemUniforms, uniforms);

    // update mesh and grass info buffer
    CoreGraphics::BufferUpdateArray(vegetationState.meshInfoBuffer, vegetationState.meshInfos, Vegetation::MAX_MESH_INFOS);
    CoreGraphics::BufferFlush(vegetationState.meshInfoBuffer);

    CoreGraphics::BufferUpdateArray(vegetationState.grassInfoBuffer, vegetationState.grassInfos, Vegetation::MAX_GRASS_INFOS);
    CoreGraphics::BufferFlush(vegetationState.grassInfoBuffer);

    CoreGraphics::BufferInvalidate(vegetationState.indirectDrawCountBuffer[ctx.bufferIndex]);
    auto counts = (Vegetation::DrawCount*)CoreGraphics::BufferMap(vegetationState.indirectDrawCountBuffer[ctx.bufferIndex]);

    // update draw counts
    vegetationState.grassDrawsThisFrame = counts->NumGrassDraws;
    for (IndexT i = 0; i < MAX_MESH_INFOS; i++)
        vegetationState.meshDrawsThisFrame[i] = counts->NumMeshDraws[i/4][i%4];

    CoreGraphics::BufferUnmap(vegetationState.indirectDrawCountBuffer[ctx.bufferIndex]);
}

//------------------------------------------------------------------------------
/**
*/
Graphics::ContextEntityId
VegetationContext::Alloc()
{
    return vegetationAllocator.Alloc();
}

//------------------------------------------------------------------------------
/**
*/
void
VegetationContext::Dealloc(Graphics::ContextEntityId id)
{
    const VegetationType& type = vegetationAllocator.Get<Vegetation_Type>(id.id);

    // if mesh type, dealloc mesh info struct
    uint& gpuInfo = vegetationAllocator.Get<Vegetation_GPUInfo>(id.id);

    if (type == VegetationType::MeshType)
        vegetationState.meshInfoPool.Dealloc(gpuInfo);
    else if (type == VegetationType::GrassType)
        vegetationState.grassInfoPool.Dealloc(gpuInfo);


    // dealloc texture
    IndexT& textureIndex = vegetationAllocator.Get<Vegetation_TextureIndex>(id.id);
    vegetationState.texturePool.Dealloc(textureIndex);

    vegetationAllocator.Dealloc(id.id);
}

} // namespace Vegetation
