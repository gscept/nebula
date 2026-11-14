
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
#include "terrain/terraincontext.h"


#include "frame/default.h"
namespace Vegetation
{

VegetationContext::VegetationAllocator VegetationContext::vegetationAllocator;
__ImplementContext(VegetationContext, VegetationContext::vegetationAllocator);

struct
{
    CoreGraphics::ShaderId vegetationBaseShader;
    CoreGraphics::ShaderProgramId vegetationClearShader;
    CoreGraphics::ShaderProgramId vegetationGenerateDrawsShader;
    CoreGraphics::ShaderProgramId vegetationMeshZShader;
    CoreGraphics::ShaderProgramId vegetationMeshShader;
    CoreGraphics::ShaderProgramId vegetationMeshVColorZShader;
    CoreGraphics::ShaderProgramId vegetationMeshVColorShader;

} vegetationState;

static const uint VegetationGridSize = 32;
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
VegetationContext::Create()
{
    __CreateContext();
    using namespace CoreGraphics;

    Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&__bundle, &__state);

    vegetationState.vegetationBaseShader = CoreGraphics::ShaderGet("shd:vegetation/shaders/vegetation.gplb");
    vegetationState.vegetationClearShader = CoreGraphics::ShaderGetProgram(vegetationState.vegetationBaseShader, ShaderFeatureMask("VegetationClear"));
    vegetationState.vegetationGenerateDrawsShader = CoreGraphics::ShaderGetProgram(vegetationState.vegetationBaseShader, ShaderFeatureMask("VegetationGenerateDraws"));
    vegetationState.vegetationMeshZShader = CoreGraphics::ShaderGetProgram(vegetationState.vegetationBaseShader, ShaderFeatureMask("VegetationMeshDrawZ"));
    vegetationState.vegetationMeshShader = CoreGraphics::ShaderGetProgram(vegetationState.vegetationBaseShader, ShaderFeatureMask("VegetationMeshDraw"));
    vegetationState.vegetationMeshVColorZShader = CoreGraphics::ShaderGetProgram(vegetationState.vegetationBaseShader, ShaderFeatureMask("VegetationMeshDrawVColorZ"));
    vegetationState.vegetationMeshVColorShader = CoreGraphics::ShaderGetProgram(vegetationState.vegetationBaseShader, ShaderFeatureMask("VegetationMeshDrawVColor"));

    FrameScript_default::RegisterSubgraph_VegetationClearDraws_Compute([](const CoreGraphics::CmdBufferId cmdBuf, const CoreGraphics::QueueType queue, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {
        CmdSetShaderProgram(cmdBuf, vegetationState.vegetationClearShader, queue);

        const Util::Array<VegetationInstanceInfo>& instances = vegetationAllocator.GetArray<Vegetation_InstanceInfo>();
        for (const auto instance : instances)
        {
            BarrierPush(cmdBuf, PipelineStage::AllShadersRead, PipelineStage::ComputeShaderWrite, BarrierDomain::Global,
            {
                BufferBarrierInfo{ .buf = instance.drawCountBuffer[bufferIndex], .subres = BufferSubresourceInfo() }
            });
            CmdSetResourceTable(cmdBuf, instance.systemResourceTable, NEBULA_SYSTEM_GROUP, ComputePipeline, nullptr);
            CmdSetResourceTable(cmdBuf, instance.argumentsTable[bufferIndex], NEBULA_BATCH_GROUP, ComputePipeline, nullptr);

            CmdDispatch(cmdBuf, 1, 1, 1);
            BarrierPop(cmdBuf);
        }
    });

    FrameScript_default::RegisterSubgraph_VegetationGenerateDraws_Compute([](const CoreGraphics::CmdBufferId cmdBuf, const CoreGraphics::QueueType queue, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {
        CmdSetShaderProgram(cmdBuf, vegetationState.vegetationGenerateDrawsShader, queue);
        const Util::Array<VegetationInstanceInfo>& instances = vegetationAllocator.GetArray<Vegetation_InstanceInfo>();
        for (const auto instance : instances)
        {
            CmdBarrier(cmdBuf, PipelineStage::Indirect, PipelineStage::ComputeShaderWrite, BarrierDomain::Global,
            {
                BufferBarrierInfo{ .buf = instance.meshDrawCallsBuffer[bufferIndex], .subres = BufferSubresourceInfo() }
                , BufferBarrierInfo{ .buf = instance.drawCountBuffer[bufferIndex], .subres = BufferSubresourceInfo() }
            });
            CmdBarrier(cmdBuf, PipelineStage::AllShadersRead, PipelineStage::ComputeShaderWrite, BarrierDomain::Global,
            {
                BufferBarrierInfo{ .buf = instance.meshArgumentsBuffer[bufferIndex], .subres = BufferSubresourceInfo() }
            });
            CmdSetResourceTable(cmdBuf, instance.systemResourceTable, NEBULA_SYSTEM_GROUP, ComputePipeline, nullptr);
            CmdSetResourceTable(cmdBuf, instance.argumentsTable[bufferIndex], NEBULA_BATCH_GROUP, ComputePipeline, nullptr);

            uint groupsX = instance.worldSizeX / VegetationGridSize;
            uint groupsY = instance.worldSizeZ / VegetationGridSize;
            CmdDispatch(cmdBuf, groupsX, groupsY, 1);

            CmdBarrier(cmdBuf, PipelineStage::ComputeShaderWrite, PipelineStage::Indirect, BarrierDomain::Global,
            {
                BufferBarrierInfo{ .buf = instance.meshDrawCallsBuffer[bufferIndex], .subres = BufferSubresourceInfo() }
                , BufferBarrierInfo{ .buf = instance.drawCountBuffer[bufferIndex], .subres = BufferSubresourceInfo() }
            });
            CmdBarrier(cmdBuf, PipelineStage::ComputeShaderWrite, PipelineStage::AllShadersRead, BarrierDomain::Global,
            {
                BufferBarrierInfo{ .buf = instance.meshArgumentsBuffer[bufferIndex], .subres = BufferSubresourceInfo() }
            });
        }
    });

    FrameScript_default::RegisterSubgraph_VegetationPrepass_Render([](const CoreGraphics::CmdBufferId cmdBuf, const CoreGraphics::QueueType queue, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {
        const Util::Array<VegetationInstanceInfo>& instances = vegetationAllocator.GetArray<Vegetation_InstanceInfo>();
        for (const auto instance : instances)
        {
            for (IndexT i = 0; i < MAX_MESH_INFOS; i++)
            {
                if (!instance.meshInfos[i].used)
                    continue;

                CmdSetShaderProgram(cmdBuf, instance.meshZPrograms[i], queue);

                // setup mesh
                CmdSetPrimitiveTopology(cmdBuf, CoreGraphics::PrimitiveTopology::TriangleList);
                CmdSetVertexLayout(cmdBuf, instance.meshVertexLayouts[i]);
                CmdSetVertexBuffer(cmdBuf, 0, CoreGraphics::GetVertexBuffer(), instance.meshVertexPositionOffsets[i]);
                CmdSetVertexBuffer(cmdBuf, 1, CoreGraphics::GetVertexBuffer(), instance.meshVertexAttributeOffsets[i]);
                CmdSetIndexBuffer(cmdBuf, IndexType::Index16, CoreGraphics::GetIndexBuffer(), instance.meshIndexOffsets[i]);

                // set graphics pipeline
                CmdSetGraphicsPipeline(cmdBuf);

                // bind resources
                CmdSetResourceTable(cmdBuf, instance.systemResourceTable, NEBULA_SYSTEM_GROUP, GraphicsPipeline, nullptr);
                CmdSetResourceTable(cmdBuf, instance.argumentsTable[bufferIndex], NEBULA_BATCH_GROUP, GraphicsPipeline, nullptr);

                // draw
                CmdDrawIndirectIndexed(cmdBuf, instance.meshDrawCallsBuffer[bufferIndex], 0, instance.drawCountBuffer[bufferIndex], 0, MaxNumIndirectDraws, sizeof(Vegetation::DrawIndexedCommand));
            }
        }
    });

    FrameScript_default::RegisterSubgraph_VegetationRender_Render([](const CoreGraphics::CmdBufferId cmdBuf, const CoreGraphics::QueueType queue, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {
        const Util::Array<VegetationInstanceInfo>& instances = vegetationAllocator.GetArray<Vegetation_InstanceInfo>();

        for (const auto instance : instances)
        {
            for (IndexT i = 0; i < MAX_MESH_INFOS; i++)
            {
                if (!instance.meshInfos[i].used)
                    continue;

                CmdSetShaderProgram(cmdBuf, instance.meshPrograms[i], queue);

                // setup mesh
                CmdSetPrimitiveTopology(cmdBuf, CoreGraphics::PrimitiveTopology::TriangleList);
                CmdSetVertexLayout(cmdBuf, instance.meshVertexLayouts[i]);
                CmdSetVertexBuffer(cmdBuf, 0, CoreGraphics::GetVertexBuffer(), instance.meshVertexPositionOffsets[i]);
                CmdSetVertexBuffer(cmdBuf, 1, CoreGraphics::GetVertexBuffer(), instance.meshVertexAttributeOffsets[i]);
                CmdSetIndexBuffer(cmdBuf, IndexType::Index16, CoreGraphics::GetIndexBuffer(), instance.meshIndexOffsets[i]);

                // set graphics pipeline
                CmdSetGraphicsPipeline(cmdBuf);

                // bind resources
                CmdSetResourceTable(cmdBuf, instance.systemResourceTable, NEBULA_SYSTEM_GROUP, GraphicsPipeline, nullptr);
                CmdSetResourceTable(cmdBuf, instance.argumentsTable[bufferIndex], NEBULA_BATCH_GROUP, GraphicsPipeline, nullptr);

                // draw
                CmdDrawIndirectIndexed(cmdBuf, instance.meshDrawCallsBuffer[bufferIndex], i * MaxNumIndirectDraws, instance.drawCountBuffer[bufferIndex], 0, MaxNumIndirectDraws, sizeof(Vegetation::DrawIndexedCommand));
            }
        }
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
VegetationContext::Setup(const Graphics::GraphicsEntityId id)
{
    using namespace CoreGraphics;

    Graphics::ContextEntityId cid = GetContextId(id);
    VegetationInstanceInfo& instanceInfo = vegetationAllocator.Get<Vegetation_InstanceInfo>(cid.id);

    Graphics::ContextEntityId tcid = Terrain::TerrainContext::GetContextId(id);
    n_assert_fmt(tcid != Graphics::InvalidContextEntityId, "Entity %d needs to be setup as a terrain before vegetation!", id.HashCode());

    CoreGraphics::BufferCreateInfo meshInfo;
    meshInfo.name = "Vegetation Mesh Info";
    meshInfo.usageFlags = CoreGraphics::BufferUsage::ReadWrite;
    meshInfo.mode = CoreGraphics::DeviceAndHost;
    meshInfo.elementSize = sizeof(Vegetation::MeshInfo);
    meshInfo.size = Vegetation::MAX_MESH_INFOS;
    meshInfo.queueSupport = CoreGraphics::ComputeQueueSupport | CoreGraphics::GraphicsQueueSupport;
    instanceInfo.meshInfoBuffer = CoreGraphics::CreateBuffer(meshInfo);

    CoreGraphics::BufferCreateInfo cboInfo;
    cboInfo.name = "Vegetation System Uniforms";
    cboInfo.usageFlags = CoreGraphics::BufferUsage::ConstantBuffer;
    cboInfo.mode = CoreGraphics::DeviceAndHost;
    cboInfo.byteSize = sizeof(VegetationGenerateUniforms::STRUCT);
    cboInfo.queueSupport = CoreGraphics::ComputeQueueSupport | CoreGraphics::GraphicsQueueSupport;
    instanceInfo.systemUniforms = CoreGraphics::CreateBuffer(cboInfo);

    CoreGraphics::BufferCreateInfo indirectMeshArgumentsBufferInfo;
    indirectMeshArgumentsBufferInfo.name = "Vegetation Indirect Mesh Draw Buffer";
    indirectMeshArgumentsBufferInfo.usageFlags = CoreGraphics::BufferUsage::IndirectDraw | CoreGraphics::BufferUsage::ReadWrite;
    indirectMeshArgumentsBufferInfo.mode = CoreGraphics::DeviceLocal;
    indirectMeshArgumentsBufferInfo.elementSize = sizeof(DrawIndexedCommand);
    indirectMeshArgumentsBufferInfo.size = MaxNumIndirectDraws * MAX_MESH_INFOS;
    indirectMeshArgumentsBufferInfo.queueSupport = CoreGraphics::ComputeQueueSupport | CoreGraphics::GraphicsQueueSupport;
    instanceInfo.meshDrawCallsBuffer.Resize(CoreGraphics::GetNumBufferedFrames());
    for (IndexT i = 0; i < instanceInfo.meshDrawCallsBuffer.Size(); i++)
    {
        instanceInfo.meshDrawCallsBuffer[i] = CoreGraphics::CreateBuffer(indirectMeshArgumentsBufferInfo);
    }

    CoreGraphics::BufferCreateInfo meshDrawCallsBufferInfo;
    meshDrawCallsBufferInfo.name = "Vegetation Indirect Mesh Arguments Buffer";
    meshDrawCallsBufferInfo.usageFlags = CoreGraphics::BufferUsage::ReadWrite;
    meshDrawCallsBufferInfo.mode = CoreGraphics::DeviceLocal;
    meshDrawCallsBufferInfo.elementSize = sizeof(Vegetation::InstanceUniforms);
    meshDrawCallsBufferInfo.size = MaxNumIndirectDraws;
    meshDrawCallsBufferInfo.queueSupport = CoreGraphics::ComputeQueueSupport | CoreGraphics::GraphicsQueueSupport;
    instanceInfo.meshArgumentsBuffer.Resize(CoreGraphics::GetNumBufferedFrames());
    for (IndexT i = 0; i < instanceInfo.meshArgumentsBuffer.Size(); i++)
    {
        instanceInfo.meshArgumentsBuffer[i] = CoreGraphics::CreateBuffer(meshDrawCallsBufferInfo);
    }

    CoreGraphics::BufferCreateInfo indirectCountBufferInfo;
    indirectCountBufferInfo.name = "Vegetation Draw Count Buffer";
    indirectCountBufferInfo.usageFlags = CoreGraphics::BufferUsage::IndirectDraw | CoreGraphics::BufferUsage::ReadWrite;
    indirectCountBufferInfo.elementSize = sizeof(Vegetation::DrawCount::STRUCT);
    indirectCountBufferInfo.size = MAX_MESH_INFOS;
    indirectCountBufferInfo.mode = CoreGraphics::DeviceLocal;
    indirectCountBufferInfo.queueSupport = CoreGraphics::ComputeQueueSupport | CoreGraphics::GraphicsQueueSupport;
    instanceInfo.drawCountBuffer.Resize(CoreGraphics::GetNumBufferedFrames());
    for (IndexT i = 0; i < instanceInfo.drawCountBuffer.Size(); i++)
    {
        instanceInfo.drawCountBuffer[i] = CoreGraphics::CreateBuffer(indirectCountBufferInfo);
    }

    instanceInfo.heightMap = Terrain::TerrainContext::GetHeightMap(id);
    Terrain::TerrainContext::GetHeightBounds(id, instanceInfo.minHeight, instanceInfo.maxHeight);
    Terrain::TerrainContext::GetWorldSize(id, instanceInfo.worldSizeX, instanceInfo.worldSizeZ);

    CoreGraphics::TextureCreateInfo debugTextureInfo;
    debugTextureInfo.name = "Vegetation Debug Buffer";
    debugTextureInfo.usage = CoreGraphics::TextureUsage::ReadWrite;
    debugTextureInfo.format = CoreGraphics::PixelFormat::Code::R32F;
    debugTextureInfo.width = instanceInfo.worldSizeX;
    debugTextureInfo.height = instanceInfo.worldSizeZ;
    instanceInfo.debugTexture = CoreGraphics::CreateTexture(debugTextureInfo);

    instanceInfo.systemResourceTable = ShaderCreateResourceTable(vegetationState.vegetationBaseShader, NEBULA_SYSTEM_GROUP);

    ResourceTableSetConstantBuffer(instanceInfo.systemResourceTable,
        {
            instanceInfo.systemUniforms,
            Vegetation::VegetationGenerateUniforms::BINDING,
            0,
            sizeof(Vegetation::VegetationGenerateUniforms),
            0
        });

    ResourceTableSetRWBuffer(instanceInfo.systemResourceTable,
        {
            instanceInfo.meshInfoBuffer,
            Vegetation::MeshInfos::BINDING,
            0,
            sizeof(Vegetation::MeshInfo) * Vegetation::MAX_MESH_INFOS,
            0
        });

    ResourceTableSetRWTexture(instanceInfo.systemResourceTable,
        {
            instanceInfo.debugTexture,
            Vegetation::DebugTexture::BINDING
        });

    ResourceTableCommitChanges(instanceInfo.systemResourceTable);

    instanceInfo.argumentsTable.Resize(CoreGraphics::GetNumBufferedFrames());
    for (IndexT i = 0; i < instanceInfo.argumentsTable.Size(); i++)
    {
        instanceInfo.argumentsTable[i] = ShaderCreateResourceTable(vegetationState.vegetationBaseShader, NEBULA_BATCH_GROUP);

        ResourceTableSetRWBuffer(instanceInfo.argumentsTable[i],
            {
                instanceInfo.meshArgumentsBuffer[i],
                Vegetation::InstanceMeshArguments::BINDING,
                0,
                BufferGetByteSize(instanceInfo.meshArgumentsBuffer[i]),
                0
            });

        ResourceTableSetRWBuffer(instanceInfo.argumentsTable[i],
            {
                instanceInfo.meshDrawCallsBuffer[i],
                Vegetation::IndirectMeshDrawBuffer::BINDING,
                0,
                BufferGetByteSize(instanceInfo.meshDrawCallsBuffer[i]),
                0
            });

        ResourceTableSetRWBuffer(instanceInfo.argumentsTable[i],
            {
                instanceInfo.drawCountBuffer[i],
                Vegetation::DrawCount::BINDING,
                0,
                BufferGetByteSize(instanceInfo.drawCountBuffer[i]),
                0
            });

        ResourceTableCommitChanges(instanceInfo.argumentsTable[i]);
    }
}

//------------------------------------------------------------------------------
/**
*/
uint
VegetationContext::SetupMesh(const Graphics::GraphicsEntityId id, const VegetationMeshSetup& setup)
{
    Graphics::ContextEntityId cid = GetContextId(id.id);

    float& slopeThreshold = vegetationAllocator.Get<Vegetation_SlopeThreshold>(cid.id);
    float& heightThreshold = vegetationAllocator.Get<Vegetation_HeightThreshold>(cid.id);
    VegetationInstanceInfo& instanceInfo = vegetationAllocator.Get<Vegetation_InstanceInfo>(cid.id);

    uint infoIndex = instanceInfo.meshInfoPool.Alloc();
    n_assert(infoIndex < Vegetation::MAX_MESH_INFOS);
    MeshInfo& info = instanceInfo.meshInfos[infoIndex];
    info.used = false;

    info.maskIndex = CoreGraphics::TextureGetBindlessHandle(CoreGraphics::Black2D);
    info.maskSize[0] = 1;
    info.maskSize[1] = 1;
    // Load mask, update uniforms when ready
    Resources::CreateResource(setup.mask, "Vegetation", [&info, &instanceInfo, infoIndex](Resources::ResourceId res)
    {

        info.maskIndex = CoreGraphics::TextureGetBindlessHandle(res.resource);
        CoreGraphics::TextureDimensions dims = CoreGraphics::TextureGetDimensions(res.resource);
        info.maskSize[0] = dims.width;
        info.maskSize[1] = dims.height;
    }, nullptr);


    Resources::CreateResource(setup.model, "Vegetation", [&info, &instanceInfo, infoIndex](Resources::ResourceId res)
    {
        const Util::Array<Models::ModelNode*>& nodes = Models::ModelGetNodes(res);
        CoreGraphics::MeshId mesh = CoreGraphics::InvalidMeshId;
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

        info.indexCount = 0;
        info.vertexCount = 0;
        info.numLods = groups.Size();

        instanceInfo.meshVertexPositionOffsets[infoIndex] = CoreGraphics::MeshGetVertexOffset(mesh, 0);
        instanceInfo.meshVertexAttributeOffsets[infoIndex] = CoreGraphics::MeshGetVertexOffset(mesh, 1);
        instanceInfo.meshIndexOffsets[infoIndex] = CoreGraphics::MeshGetIndexOffset(mesh);

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
        n_assert(components.Size() >= 4);
        if (components.Size() == 5) // with vertex colors
        {
            n_assert(components[0].GetFormat() == CoreGraphics::VertexComponent::Float3);
            n_assert(components[1].GetFormat() == CoreGraphics::VertexComponent::Short2);
            n_assert(components[2].GetFormat() == CoreGraphics::VertexComponent::Byte4N);
            n_assert(components[3].GetFormat() == CoreGraphics::VertexComponent::Byte4N);
            n_assert(components[4].GetFormat() == CoreGraphics::VertexComponent::UByte4N);
            instanceInfo.meshZPrograms[infoIndex] = vegetationState.vegetationMeshVColorZShader;
            instanceInfo.meshPrograms[infoIndex] = vegetationState.vegetationMeshVColorShader;
            instanceInfo.meshVertexLayouts[infoIndex] = CoreGraphics::MeshLoader::GetLayout(CoreGraphics::VertexLayoutType::Colors);
        }
        else // without
        {
            n_assert(components[0].GetFormat() == CoreGraphics::VertexComponent::Float3);
            n_assert(components[1].GetFormat() == CoreGraphics::VertexComponent::Short2);
            n_assert(components[2].GetFormat() == CoreGraphics::VertexComponent::Byte4N);
            n_assert(components[3].GetFormat() == CoreGraphics::VertexComponent::Byte4N);
            instanceInfo.meshZPrograms[infoIndex] = vegetationState.vegetationMeshZShader;
            instanceInfo.meshPrograms[infoIndex] = vegetationState.vegetationMeshShader;
            instanceInfo.meshVertexLayouts[infoIndex] = CoreGraphics::MeshLoader::GetLayout(CoreGraphics::VertexLayoutType::Normal);
        }
        n_assert(instanceInfo.meshZPrograms[infoIndex] != CoreGraphics::InvalidShaderProgramId);
        n_assert(instanceInfo.meshPrograms[infoIndex] != CoreGraphics::InvalidShaderProgramId);
        n_assert(instanceInfo.meshVertexLayouts[infoIndex] != CoreGraphics::InvalidVertexLayoutId);

        info.used = true;

    }, nullptr);

    slopeThreshold = setup.slopeThreshold;
    heightThreshold = setup.heightThreshold;

    info.slopeThreshold = slopeThreshold;
    info.heightThreshold = heightThreshold;
    info.materialIndex = -1;

    info.distribution = 0.0f;

    return infoIndex;
}

//------------------------------------------------------------------------------
/**
*/
void
VegetationContext::SetModel(const Graphics::GraphicsEntityId id, uint index, const IO::Path& model)
{
    Graphics::ContextEntityId cid = GetContextId(id.id);

    VegetationInstanceInfo& instanceInfo = vegetationAllocator.Get<Vegetation_InstanceInfo>(cid.id);
    MeshInfo& info = instanceInfo.meshInfos[index];
    info.used = false;
    Resources::CreateResource(model, "Vegetation", [&instanceInfo, &info, index](Resources::ResourceId res)
    {
        const Util::Array<Models::ModelNode*>& nodes = Models::ModelGetNodes(res);
        CoreGraphics::MeshId mesh = CoreGraphics::InvalidMeshId;
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

        info.indexCount = 0;
        info.vertexCount = 0;
        info.numLods = groups.Size();

        instanceInfo.meshVertexPositionOffsets[index] = CoreGraphics::MeshGetVertexOffset(mesh, 0);
        instanceInfo.meshVertexAttributeOffsets[index] = CoreGraphics::MeshGetVertexOffset(mesh, 1);
        instanceInfo.meshIndexOffsets[index] = CoreGraphics::MeshGetIndexOffset(mesh);

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
            instanceInfo.meshZPrograms[index] = vegetationState.vegetationMeshVColorZShader;
            instanceInfo.meshPrograms[index] = vegetationState.vegetationMeshVColorShader;
            instanceInfo.meshVertexLayouts[index] = CoreGraphics::MeshLoader::GetLayout(CoreGraphics::VertexLayoutType::Colors);
        }
        else // without
        {
            n_assert(components[0].GetFormat() == CoreGraphics::VertexComponent::Float3);
            n_assert(components[1].GetFormat() == CoreGraphics::VertexComponent::Byte4N);
            n_assert(components[2].GetFormat() == CoreGraphics::VertexComponent::Float2);
            n_assert(components[3].GetFormat() == CoreGraphics::VertexComponent::Byte4N);
            n_assert(components[4].GetFormat() == CoreGraphics::VertexComponent::Byte4N);
            instanceInfo.meshZPrograms[index] = vegetationState.vegetationMeshZShader;
            instanceInfo.meshPrograms[index] = vegetationState.vegetationMeshShader;
            instanceInfo.meshVertexLayouts[index] = CoreGraphics::MeshLoader::GetLayout(CoreGraphics::VertexLayoutType::Normal);
        }

        info.used = true;

    }, nullptr);
}

//------------------------------------------------------------------------------
/**
*/
void
VegetationContext::SetMask(const Graphics::GraphicsEntityId id, uint index, CoreGraphics::TextureId mask)
{
    Graphics::ContextEntityId cid = GetContextId(id.id);
    VegetationInstanceInfo& instanceInfo = vegetationAllocator.Get<Vegetation_InstanceInfo>(cid.id);
    MeshInfo& info = instanceInfo.meshInfos[index];
    info.maskIndex = CoreGraphics::TextureGetBindlessHandle(mask);
    CoreGraphics::TextureDimensions dims = CoreGraphics::TextureGetDimensions(mask);
    info.maskSize[0] = dims.width;
    info.maskSize[1] = dims.height;
}

//------------------------------------------------------------------------------
/**
*/
void
VegetationContext::UpdateViewResources(const Graphics::ViewId view, const Graphics::FrameContext& ctx)
{
    if (view != Graphics::GraphicsServer::Instance()->GetView("mainview"))
        return;

    Graphics::CameraSettings settings = Graphics::CameraContext::GetSettings(ViewGetCamera(view));
    Math::mat4 cameraTransform = Math::inverse(Graphics::CameraContext::GetView(ViewGetCamera(view)));
    VegetationGenerateUniforms::STRUCT uniforms;
    cameraTransform.position.store(uniforms.CameraPosition);
    cameraTransform.z_axis.store(uniforms.CameraForward);

    Util::Array<VegetationInstanceInfo>& instances = vegetationAllocator.GetArray<Vegetation_InstanceInfo>();
    for (auto& instance : instances)
    {
        uniforms.NumMeshInfos = instance.meshInfoPool.GetNumUsed();

        uniforms.HeightMap = CoreGraphics::TextureGetBindlessHandle(instance.heightMap);
        uniforms.WorldSize[0] = instance.worldSizeX;
        uniforms.WorldSize[1] = instance.worldSizeZ;
        uniforms.WorldPosition[0] = 0;
        uniforms.WorldPosition[1] = 0;
        uniforms.InvWorldSize[0] = 1.0f / instance.worldSizeX;
        uniforms.InvWorldSize[1] = 1.0f / instance.worldSizeZ;
        uniforms.GenerateQuadSize[0] = VegetationGridSize;
        uniforms.GenerateQuadSize[1] = VegetationGridSize;
        uniforms.DensityFactor = 1.0f;
        uniforms.MinHeight = instance.minHeight;
        uniforms.MaxHeight = instance.maxHeight;
        uniforms.HeightMapSize[0] = CoreGraphics::TextureGetDimensions(instance.heightMap).width;
        uniforms.HeightMapSize[1] = CoreGraphics::TextureGetDimensions(instance.heightMap).height;
        uniforms.MaxRange = 100.0f * 100.0f;
        uniforms.Fov = Math::cos(settings.GetFov() + 5.0_rad);

        // update uniform buffer
        CoreGraphics::BufferUpdate(instance.systemUniforms, uniforms);
        CoreGraphics::BufferFlush(instance.systemUniforms);

        // update mesh and grass info buffer
        CoreGraphics::BufferUpdateArray(instance.meshInfoBuffer, instance.meshInfos, Vegetation::MAX_MESH_INFOS);
        CoreGraphics::BufferFlush(instance.meshInfoBuffer);
    }

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
    VegetationInstanceInfo& instance = vegetationAllocator.Get<Vegetation_InstanceInfo>(id.id);

    CoreGraphics::DestroyResourceTable(instance.systemResourceTable);
    for (auto tab : instance.argumentsTable)
        CoreGraphics::DestroyResourceTable(tab);

    CoreGraphics::DestroyBuffer(instance.meshInfoBuffer);
    CoreGraphics::DestroyBuffer(instance.systemUniforms);
    for (auto buf : instance.meshDrawCallsBuffer)
        CoreGraphics::DestroyBuffer(buf);
    for (auto buf : instance.meshArgumentsBuffer)
        CoreGraphics::DestroyBuffer(buf);

    for (auto buf : instance.drawCountBuffer)
        CoreGraphics::DestroyBuffer(buf);

    instance.meshInfoPool.Clear();

    vegetationAllocator.Dealloc(id.id);
}

} // namespace Vegetation
