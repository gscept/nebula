
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

#include "gpulang/render/system_shaders/vegetation.h"

#include "frame/default.h"
namespace Vegetation
{

VegetationContext::VegetationAllocator VegetationContext::vegetationAllocator;
__ImplementContext(VegetationContext, VegetationContext::vegetationAllocator);

struct
{
    CoreGraphics::TextureId heightMap;
    float minHeight, maxHeight;
    Math::vec2 worldSize;

    Vegetation::MeshInfo meshInfos[Vegetation::MAX_MESH_INFOS];
    Ids::IdPool meshInfoPool;
    CoreGraphics::BufferId meshInfoBuffer;

    CoreGraphics::ResourceTableId systemResourceTable;
    CoreGraphics::BufferId systemUniforms;

    CoreGraphics::ShaderId vegetationBaseShader;
    CoreGraphics::ShaderProgramId vegetationClearShader;
    CoreGraphics::ShaderProgramId vegetationGenerateDrawsShader;
    CoreGraphics::ShaderProgramId vegetationMeshZShader;
    CoreGraphics::ShaderProgramId vegetationMeshShader;
    CoreGraphics::ShaderProgramId vegetationMeshVColorZShader;
    CoreGraphics::ShaderProgramId vegetationMeshVColorShader;

    CoreGraphics::BufferId meshDrawCallsBuffer;
    Util::FixedArray<CoreGraphics::BufferId> indirectMeshDrawCallsBuffer;
    CoreGraphics::BufferId meshArgumentsBuffer;
    Util::FixedArray<CoreGraphics::BufferId> indirectMeshArgumentsBuffer;

    CoreGraphics::ResourceTableId argumentsTable;
    Util::FixedArray<CoreGraphics::ResourceTableId> indirectArgumentsTable;

    CoreGraphics::BufferId drawCountBuffer;
    Util::FixedArray<CoreGraphics::BufferId> indirectDrawCountBuffer;

    IndexT meshMaskCount = 0;

    SizeT meshDrawsThisFrame[MAX_MESH_INFOS] = { 0 };
    CoreGraphics::VertexLayoutId meshVertexLayouts[MAX_MESH_INFOS];
    SizeT meshVertexOffsets[MAX_MESH_INFOS];
    SizeT meshIndexOffsets[MAX_MESH_INFOS];
    CoreGraphics::ShaderProgramId meshZPrograms[MAX_MESH_INFOS];
    CoreGraphics::ShaderProgramId meshPrograms[MAX_MESH_INFOS];

    Vegetation::VegetationMaterialUniforms::STRUCT materialUniforms;
    CoreGraphics::BufferId materialUniformsBuffer;

} vegetationState;

static const uint VegetationDistributionRadius = 128;
static const uint MaxNumIndirectDraws = 8192;

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
    meshInfo.usageFlags = CoreGraphics::BufferUsage::ConstantBuffer;
    meshInfo.mode = CoreGraphics::HostCached;
    meshInfo.elementSize = sizeof(Vegetation::MeshInfo);
    meshInfo.size = Vegetation::MAX_MESH_INFOS;
    meshInfo.queueSupport = CoreGraphics::ComputeQueueSupport | CoreGraphics::GraphicsQueueSupport;
    vegetationState.meshInfoBuffer = CoreGraphics::CreateBuffer(meshInfo);

    CoreGraphics::BufferCreateInfo cboInfo;
    cboInfo.name = "Vegetation System Uniforms";
    cboInfo.usageFlags = CoreGraphics::BufferUsage::ConstantBuffer;
    cboInfo.mode = CoreGraphics::DeviceAndHost;
    cboInfo.byteSize = sizeof(VegetationGenerateUniforms::STRUCT);
    cboInfo.queueSupport = CoreGraphics::ComputeQueueSupport | CoreGraphics::GraphicsQueueSupport;
    vegetationState.systemUniforms = CoreGraphics::CreateBuffer(cboInfo);

    CoreGraphics::BufferCreateInfo meshArgumentsBufferInfo;
    meshArgumentsBufferInfo.name = "Vegetation Mesh Draw Buffer";
    meshArgumentsBufferInfo.usageFlags = CoreGraphics::BufferUsage::ReadWrite | CoreGraphics::BufferUsage::TransferSource;
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
        indirectMeshArgumentsBufferInfo.usageFlags = CoreGraphics::BufferUsage::IndirectDraw | CoreGraphics::BufferUsage::TransferDestination;
        indirectMeshArgumentsBufferInfo.mode = CoreGraphics::DeviceLocal;
        indirectMeshArgumentsBufferInfo.elementSize = sizeof(DrawIndexedCommand);
        indirectMeshArgumentsBufferInfo.size = MaxNumIndirectDraws * MAX_MESH_INFOS;
        indirectMeshArgumentsBufferInfo.queueSupport = CoreGraphics::ComputeQueueSupport | CoreGraphics::GraphicsQueueSupport;
        vegetationState.indirectMeshDrawCallsBuffer[i] = CoreGraphics::CreateBuffer(indirectMeshArgumentsBufferInfo);
    }

    // per-instance uniforms (fed through vertex)
    CoreGraphics::BufferCreateInfo meshDrawCallsBufferInfo;
    meshDrawCallsBufferInfo.name = "Vegetation Mesh Arguments Buffer";
    meshDrawCallsBufferInfo.usageFlags = CoreGraphics::BufferUsage::ReadWrite | CoreGraphics::BufferUsage::TransferSource;
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
        meshDrawCallsBufferInfo.usageFlags = CoreGraphics::BufferUsage::ReadWrite | CoreGraphics::BufferUsage::TransferDestination;
        meshDrawCallsBufferInfo.mode = CoreGraphics::DeviceLocal;
        meshDrawCallsBufferInfo.elementSize = sizeof(Vegetation::InstanceUniforms);
        meshDrawCallsBufferInfo.size = MaxNumIndirectDraws;
        meshDrawCallsBufferInfo.queueSupport = CoreGraphics::ComputeQueueSupport | CoreGraphics::GraphicsQueueSupport;
        vegetationState.indirectMeshArgumentsBuffer[i] = CoreGraphics::CreateBuffer(meshDrawCallsBufferInfo);
    }

    CoreGraphics::BufferCreateInfo indirectCountBufferInfo;
    indirectCountBufferInfo.name = "Vegetation Draw Count Buffer";
    indirectCountBufferInfo.usageFlags = CoreGraphics::BufferUsage::ReadWrite | CoreGraphics::BufferUsage::TransferSource;
    indirectCountBufferInfo.elementSize = sizeof(Vegetation::DrawCount::STRUCT);
    indirectCountBufferInfo.mode = CoreGraphics::DeviceLocal;
    indirectCountBufferInfo.queueSupport = CoreGraphics::ComputeQueueSupport | CoreGraphics::GraphicsQueueSupport;
    vegetationState.drawCountBuffer = CoreGraphics::CreateBuffer(indirectCountBufferInfo);

    indirectCountBufferInfo.name = "Vegetation Indirect Count Buffer Readback";
    indirectCountBufferInfo.usageFlags = CoreGraphics::BufferUsage::TransferDestination;
    indirectCountBufferInfo.mode = CoreGraphics::HostLocal;
    vegetationState.indirectDrawCountBuffer.Resize(CoreGraphics::GetNumBufferedFrames());
    for (IndexT i = 0; i < vegetationState.indirectDrawCountBuffer.Size(); i++)
    {
        vegetationState.indirectDrawCountBuffer[i] = CoreGraphics::CreateBuffer(indirectCountBufferInfo);
    }

    CoreGraphics::BufferCreateInfo materialUniformInfo;
    materialUniformInfo.name = "Vegetation Materials Buffer";
    materialUniformInfo.elementSize = sizeof(Vegetation::VegetationMaterialUniforms::STRUCT);
    materialUniformInfo.usageFlags = CoreGraphics::BufferUsage::ConstantBuffer;
    materialUniformInfo.mode = CoreGraphics::DeviceAndHost;
    vegetationState.materialUniformsBuffer = CoreGraphics::CreateBuffer(materialUniformInfo);
    memset(&vegetationState.materialUniforms, 0x0, sizeof(Vegetation::VegetationMaterialUniforms::STRUCT));

    vegetationState.vegetationBaseShader = CoreGraphics::ShaderGet("shd:vegetation.gplb");
    vegetationState.vegetationClearShader = CoreGraphics::ShaderGetProgram(vegetationState.vegetationBaseShader, ShaderFeatureMask("VegetationClear"));
    vegetationState.vegetationGenerateDrawsShader = CoreGraphics::ShaderGetProgram(vegetationState.vegetationBaseShader, ShaderFeatureMask("VegetationGenerateDraws"));
    vegetationState.vegetationMeshZShader = CoreGraphics::ShaderGetProgram(vegetationState.vegetationBaseShader, ShaderFeatureMask("VegetationMeshDrawZ"));
    vegetationState.vegetationMeshShader = CoreGraphics::ShaderGetProgram(vegetationState.vegetationBaseShader, ShaderFeatureMask("VegetationMeshDraw"));
    vegetationState.vegetationMeshVColorZShader = CoreGraphics::ShaderGetProgram(vegetationState.vegetationBaseShader, ShaderFeatureMask("VegetationMeshDrawVColorZ"));
    vegetationState.vegetationMeshVColorShader = CoreGraphics::ShaderGetProgram(vegetationState.vegetationBaseShader, ShaderFeatureMask("VegetationMeshDrawVColor"));
    vegetationState.systemResourceTable = ShaderCreateResourceTable(vegetationState.vegetationBaseShader, NEBULA_SYSTEM_GROUP);

    ResourceTableSetConstantBuffer(vegetationState.systemResourceTable,
        {
            vegetationState.systemUniforms,
            Vegetation::VegetationGenerateUniforms::BINDING,
            0,
            sizeof(Vegetation::VegetationGenerateUniforms),
            0
        });

    ResourceTableSetConstantBuffer(vegetationState.systemResourceTable,
        {
            vegetationState.materialUniformsBuffer,
            Vegetation::VegetationMaterialUniforms::BINDING,
            0,
            sizeof(Vegetation::VegetationMaterialUniforms),
            0
        });


    ResourceTableSetConstantBuffer(vegetationState.systemResourceTable,
        {
            vegetationState.meshInfoBuffer,
            Vegetation::MeshInfos::BINDING,
            0,
            sizeof(Vegetation::MeshInfo) * Vegetation::MAX_MESH_INFOS,
            0
        });

    ResourceTableSetRWBuffer(vegetationState.systemResourceTable,
        {
            vegetationState.meshDrawCallsBuffer,
            Vegetation::IndirectMeshDrawBuffer::BINDING,
            0,
            BufferGetByteSize(vegetationState.meshDrawCallsBuffer),
            0
        });

    ResourceTableSetRWBuffer(vegetationState.systemResourceTable,
        {
            vegetationState.drawCountBuffer,
            Vegetation::DrawCount::BINDING,
            0,
            BufferGetByteSize(vegetationState.drawCountBuffer),
            0
        });

    ResourceTableCommitChanges(vegetationState.systemResourceTable);

    vegetationState.argumentsTable = ShaderCreateResourceTable(vegetationState.vegetationBaseShader, NEBULA_BATCH_GROUP);
    ResourceTableSetRWBuffer(vegetationState.argumentsTable,
        {
            vegetationState.meshArgumentsBuffer,
            Vegetation::InstanceMeshArguments::BINDING,
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
                vegetationState.indirectMeshArgumentsBuffer[i],
                Vegetation::InstanceMeshArguments::BINDING,
                0,
                BufferGetByteSize(vegetationState.indirectMeshArgumentsBuffer[i]),
                0
            });

        ResourceTableCommitChanges(vegetationState.indirectArgumentsTable[i]);
    }

    FrameScript_default::Bind_VegetationDrawCountBuffer(vegetationState.drawCountBuffer);
    FrameScript_default::Bind_VegetationMeshArgumentsBuffer(vegetationState.meshArgumentsBuffer);
    FrameScript_default::Bind_VegetationMeshDrawsBuffer(vegetationState.meshDrawCallsBuffer);

    FrameScript_default::RegisterSubgraph_VegetationClearDraws_Compute([](const CoreGraphics::CmdBufferId cmdBuf, const CoreGraphics::QueueType queue, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {
        CmdSetShaderProgram(cmdBuf, vegetationState.vegetationClearShader, queue);
        CmdSetResourceTable(cmdBuf, vegetationState.systemResourceTable, NEBULA_SYSTEM_GROUP, ComputePipeline, nullptr);

        CmdDispatch(cmdBuf, 1, 1, 1);
    }, {
        { FrameScript_default::BufferIndex::VegetationDrawCountBuffer, CoreGraphics::PipelineStage::ComputeShaderWrite }
    });

    FrameScript_default::RegisterSubgraph_VegetationGenerateDraws_Compute([](const CoreGraphics::CmdBufferId cmdBuf, const CoreGraphics::QueueType queue, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {
        CmdSetShaderProgram(cmdBuf, vegetationState.vegetationGenerateDrawsShader, queue);
        CmdSetResourceTable(cmdBuf, vegetationState.systemResourceTable, NEBULA_SYSTEM_GROUP, ComputePipeline, nullptr);
        CmdSetResourceTable(cmdBuf, vegetationState.argumentsTable, NEBULA_BATCH_GROUP, ComputePipeline, nullptr);

        CmdDispatch(cmdBuf, VegetationDistributionRadius / 64, VegetationDistributionRadius, 1);
    }, {
        { FrameScript_default::BufferIndex::VegetationMeshDrawsBuffer, CoreGraphics::PipelineStage::ComputeShaderWrite }
        , { FrameScript_default::BufferIndex::VegetationMeshArgumentsBuffer, CoreGraphics::PipelineStage::ComputeShaderWrite }
        , { FrameScript_default::BufferIndex::VegetationDrawCountBuffer, CoreGraphics::PipelineStage::ComputeShaderWrite }
    });

    FrameScript_default::RegisterSubgraph_VegetationDrawCPUReadback_Compute([](const CoreGraphics::CmdBufferId cmdBuf, const CoreGraphics::QueueType queue, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
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
    }, {
        { FrameScript_default::BufferIndex::VegetationDrawCountBuffer, CoreGraphics::PipelineStage::TransferRead }
    });

    FrameScript_default::RegisterSubgraph_VegetationPrepass_Render([](const CoreGraphics::CmdBufferId cmdBuf, const CoreGraphics::QueueType queue, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {
        for (IndexT i = 0; i < MAX_MESH_INFOS; i++)
        {
            if (vegetationState.meshDrawsThisFrame[i] > 0)
            {
                CmdSetShaderProgram(cmdBuf, vegetationState.meshZPrograms[i], queue);

                // setup mesh
                CmdSetPrimitiveTopology(cmdBuf, CoreGraphics::PrimitiveTopology::TriangleList);
                CmdSetVertexLayout(cmdBuf, vegetationState.meshVertexLayouts[i]);
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
    }, {
        { FrameScript_default::BufferIndex::VegetationMeshDrawsBuffer, CoreGraphics::PipelineStage::Indirect }
        , { FrameScript_default::BufferIndex::VegetationMeshArgumentsBuffer, CoreGraphics::PipelineStage::Indirect }
    });

    FrameScript_default::RegisterSubgraph_VegetationRender_Render([](const CoreGraphics::CmdBufferId cmdBuf, const CoreGraphics::QueueType queue, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {
        for (IndexT i = 0; i < MAX_MESH_INFOS; i++)
        {
            if (vegetationState.meshDrawsThisFrame[i] > 0)
            {
                CmdSetShaderProgram(cmdBuf, vegetationState.meshPrograms[i], queue);

                // setup mesh
                CmdSetPrimitiveTopology(cmdBuf, CoreGraphics::PrimitiveTopology::TriangleList);
                CmdSetVertexLayout(cmdBuf, vegetationState.meshVertexLayouts[i]);
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
    }, {
        { FrameScript_default::BufferIndex::VegetationMeshDrawsBuffer, CoreGraphics::PipelineStage::Indirect }
        , { FrameScript_default::BufferIndex::VegetationMeshArgumentsBuffer, CoreGraphics::PipelineStage::Indirect }
    });

    FrameScript_default::RegisterSubgraph_VegetationCopyIndirect_Compute([](const CoreGraphics::CmdBufferId cmdBuf, const CoreGraphics::QueueType queue, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {
        CoreGraphics::BufferCopy from, to;
        from.offset = 0;
        to.offset = 0;
        CmdCopy(cmdBuf, vegetationState.meshArgumentsBuffer, { from }, vegetationState.indirectMeshArgumentsBuffer[bufferIndex], { to }, BufferGetByteSize(vegetationState.meshArgumentsBuffer));
        CmdCopy(cmdBuf, vegetationState.meshDrawCallsBuffer, { from }, vegetationState.indirectMeshDrawCallsBuffer[bufferIndex], { to }, BufferGetByteSize(vegetationState.meshDrawCallsBuffer));

    });
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
VegetationContext::Setup(Resources::ResourceName heightMap)
{
    using namespace CoreGraphics;
    vegetationState.heightMap = Resources::CreateResource(heightMap, "Vegetation", nullptr, nullptr, true);
}

//------------------------------------------------------------------------------
/**
*/
uint
VegetationContext::SetupMesh(const Graphics::GraphicsEntityId id, const VegetationMeshSetup& setup)
{
    Graphics::ContextEntityId cid = GetContextId(id.id);
    CoreGraphics::TextureId mask = Resources::CreateResource(setup.mask, "Vegetation", nullptr, nullptr, true);
    vegetationAllocator.Set<Vegetation_Mask>(cid.id, mask);

    Materials::MaterialId& material = vegetationAllocator.Get<Vegetation_Material>(cid.id);
    Models::ModelId& vmodel = vegetationAllocator.Get<Vegetation_Model>(cid.id);
    uint& infoIndex = vegetationAllocator.Get<Vegetation_GPUInfo>(cid.id);
    float& slopeThreshold = vegetationAllocator.Get<Vegetation_SlopeThreshold>(cid.id);
    float& heightThreshold = vegetationAllocator.Get<Vegetation_HeightThreshold>(cid.id);

    vmodel = Models::InvalidModelId;
    
    infoIndex = vegetationState.meshInfoPool.Alloc();
    n_assert(infoIndex < Vegetation::MAX_MESH_INFOS);
    MeshInfo& info = vegetationState.meshInfos[infoIndex];
    info.used = false;

    Models::ModelId model = Resources::CreateResource(setup.model, "Vegetation", [&vmodel, &info, infoIndex](Resources::ResourceId res)
    {
        const Util::Array<Models::ModelNode*>& nodes = Models::ModelGetNodes(res);
        CoreGraphics::MeshId mesh = CoreGraphics::InvalidMeshId;
        vmodel = res.resource;
        Util::Array<float> lodDistances;
        for (auto node : nodes)
        {
            if (node->GetType() == Models::NodeType::PrimitiveNodeType)
            {
                auto pnode = static_cast<Models::PrimitiveNode*>(node);
                if (mesh == CoreGraphics::InvalidMeshId)
                {
                    mesh = pnode->GetMesh();
                    info.materialIndex = Materials::MaterialGetBufferBinding(pnode->GetMaterial());
                }

                float minDist, maxDist;
                pnode->GetLODDistances(minDist, maxDist);
                lodDistances.Append(maxDist);
            }
        }

        // get primitive groups so we can update the mesh info
        const Util::Array<CoreGraphics::PrimitiveGroup>& groups = CoreGraphics::MeshGetPrimitiveGroups(mesh);
        uint baseVertexOffset = CoreGraphics::MeshGetVertexOffset(mesh, 0);
        uint baseIndexOffset = CoreGraphics::MeshGetIndexOffset(mesh);

        info.indexCount = 0;
        info.vertexCount = 0;
        info.numLods = groups.Size();

        vegetationState.meshVertexOffsets[infoIndex] = baseVertexOffset;
        vegetationState.meshIndexOffsets[infoIndex] = baseIndexOffset;

        float distances[] = {10, 25, 50, 100, 500, 1000};

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
            vegetationState.meshZPrograms[infoIndex] = vegetationState.vegetationMeshVColorZShader;
            vegetationState.meshPrograms[infoIndex] = vegetationState.vegetationMeshVColorShader;
            vegetationState.meshVertexLayouts[infoIndex] = CoreGraphics::MeshLoader::GetLayout(CoreGraphics::VertexLayoutType::Colors);
        }
        else // without
        {
            n_assert(components[0].GetFormat() == CoreGraphics::VertexComponent::Float3);
            n_assert(components[1].GetFormat() == CoreGraphics::VertexComponent::Byte4N);
            n_assert(components[2].GetFormat() == CoreGraphics::VertexComponent::Float2);
            n_assert(components[3].GetFormat() == CoreGraphics::VertexComponent::Byte4N);
            n_assert(components[4].GetFormat() == CoreGraphics::VertexComponent::Byte4N);
            vegetationState.meshZPrograms[infoIndex] = vegetationState.vegetationMeshZShader;
            vegetationState.meshPrograms[infoIndex] = vegetationState.vegetationMeshShader;
            vegetationState.meshVertexLayouts[infoIndex] = CoreGraphics::MeshLoader::GetLayout(CoreGraphics::VertexLayoutType::Normal);
        }

        info.used = true;

    }, nullptr);

    slopeThreshold = setup.slopeThreshold;
    heightThreshold = setup.heightThreshold;

    info.slopeThreshold = slopeThreshold;
    info.heightThreshold = heightThreshold;
    info.materialIndex = -1;

    info.distribution = 0.0f;

    vegetationState.materialUniforms.VegetationMasks[info.materialIndex / 4][info.materialIndex % 4] =
        CoreGraphics::TextureGetBindlessHandle(mask);

    CoreGraphics::BufferUpdate(vegetationState.materialUniformsBuffer, vegetationState.materialUniforms);
    return infoIndex;
}

//------------------------------------------------------------------------------
/**
*/
void
VegetationContext::SetModel(const Graphics::GraphicsEntityId id, const Resources::ResourceName& model)
{
    Graphics::ContextEntityId cid = GetContextId(id.id);

    Materials::MaterialId& material = vegetationAllocator.Get<Vegetation_Material>(cid.id);
    Models::ModelId& vmodel = vegetationAllocator.Get<Vegetation_Model>(cid.id);
    uint& infoIndex = vegetationAllocator.Get<Vegetation_GPUInfo>(cid.id);
    MeshInfo& info = vegetationState.meshInfos[infoIndex];
    info.used = false;
    Models::ModelId model = Resources::CreateResource(model, "Vegetation", [&vmodel, &info, infoIndex](Resources::ResourceId res)
    {
        const Util::Array<Models::ModelNode*>& nodes = Models::ModelGetNodes(res);
        CoreGraphics::MeshId mesh = CoreGraphics::InvalidMeshId;
        vmodel = res.resource;
        Util::Array<float> lodDistances;
        for (auto node : nodes)
        {
            if (node->GetType() == Models::NodeType::PrimitiveNodeType)
            {
                auto pnode = static_cast<Models::PrimitiveNode*>(node);
                if (mesh == CoreGraphics::InvalidMeshId)
                {
                    mesh = pnode->GetMesh();
                    info.materialIndex = Materials::MaterialGetBufferBinding(pnode->GetMaterial());
                }

                float minDist, maxDist;
                pnode->GetLODDistances(minDist, maxDist);
                lodDistances.Append(maxDist);
            }
        }

        // get primitive groups so we can update the mesh info
        const Util::Array<CoreGraphics::PrimitiveGroup>& groups = CoreGraphics::MeshGetPrimitiveGroups(mesh);
        uint baseVertexOffset = CoreGraphics::MeshGetVertexOffset(mesh, 0);
        uint baseIndexOffset = CoreGraphics::MeshGetIndexOffset(mesh);

        info.indexCount = 0;
        info.vertexCount = 0;
        info.numLods = groups.Size();

        vegetationState.meshVertexOffsets[infoIndex] = baseVertexOffset;
        vegetationState.meshIndexOffsets[infoIndex] = baseIndexOffset;

        float distances[] = { 10, 25, 50, 100, 500, 1000 };

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
            vegetationState.meshZPrograms[infoIndex] = vegetationState.vegetationMeshVColorZShader;
            vegetationState.meshPrograms[infoIndex] = vegetationState.vegetationMeshVColorShader;
            vegetationState.meshVertexLayouts[infoIndex] = CoreGraphics::MeshLoader::GetLayout(CoreGraphics::VertexLayoutType::Colors);
        }
        else // without
        {
            n_assert(components[0].GetFormat() == CoreGraphics::VertexComponent::Float3);
            n_assert(components[1].GetFormat() == CoreGraphics::VertexComponent::Byte4N);
            n_assert(components[2].GetFormat() == CoreGraphics::VertexComponent::Float2);
            n_assert(components[3].GetFormat() == CoreGraphics::VertexComponent::Byte4N);
            n_assert(components[4].GetFormat() == CoreGraphics::VertexComponent::Byte4N);
            vegetationState.meshZPrograms[infoIndex] = vegetationState.vegetationMeshZShader;
            vegetationState.meshPrograms[infoIndex] = vegetationState.vegetationMeshShader;
            vegetationState.meshVertexLayouts[infoIndex] = CoreGraphics::MeshLoader::GetLayout(CoreGraphics::VertexLayoutType::Normal);
        }

        info.used = true;

    }, nullptr);
}

//------------------------------------------------------------------------------
/**
*/
void
VegetationContext::UpdateViewResources(const Graphics::ViewId view, const Graphics::FrameContext& ctx)
{
    Graphics::CameraSettings settings = Graphics::CameraContext::GetSettings(ViewGetCamera(view));
    Math::mat4 cameraTransform = Math::inverse(Graphics::CameraContext::GetView(ViewGetCamera(view)));
    VegetationGenerateUniforms::STRUCT uniforms;
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
    uniforms.HeightMapSize[0] = CoreGraphics::TextureGetDimensions(vegetationState.heightMap).width;
    uniforms.HeightMapSize[1] = CoreGraphics::TextureGetDimensions(vegetationState.heightMap).height;
    uniforms.MaxRange = 100.0f * 100.0f;
    uniforms.Fov = Math::cos(settings.GetFov() + 5.0_rad);
    uniforms.NumMeshInfos = vegetationAllocator.Size();

    // update uniform buffer
    CoreGraphics::BufferUpdate(vegetationState.systemUniforms, uniforms);

    // update mesh and grass info buffer
    CoreGraphics::BufferUpdateArray(vegetationState.meshInfoBuffer, vegetationState.meshInfos, Vegetation::MAX_MESH_INFOS);
    CoreGraphics::BufferFlush(vegetationState.meshInfoBuffer);

    CoreGraphics::BufferInvalidate(vegetationState.indirectDrawCountBuffer[ctx.bufferIndex]);
    auto counts = (Vegetation::DrawCount::STRUCT*)CoreGraphics::BufferMap(vegetationState.indirectDrawCountBuffer[ctx.bufferIndex]);


    // update draw counts
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
    // if mesh type, dealloc mesh info struct
    uint& gpuInfo = vegetationAllocator.Get<Vegetation_GPUInfo>(id.id);

    vegetationState.meshInfoPool.Dealloc(gpuInfo);

    vegetationAllocator.Dealloc(id.id);
}

} // namespace Vegetation
