//------------------------------------------------------------------------------
// framesubpassbatch.cc
// (C) 2016-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "framesubpassbatch.h"
#include "coregraphics/shaderserver.h"
#include "coregraphics/graphicsdevice.h"
#include "materials/shaderconfigserver.h"
#include "models/model.h"
#include "models/nodes/shaderstatenode.h"
#include "graphics/graphicsserver.h"
#include "graphics/view.h"
#include "visibility/visibilitycontext.h"

#include "models/nodes/particlesystemnode.h"
#include "models/nodes/primitivenode.h"

using namespace Graphics;
using namespace CoreGraphics;
using namespace Materials;
using namespace Models;
using namespace Util;

#define NEBULA_FRAME_LOG_ENABLED   (0)
#if NEBULA_FRAME_LOG_ENABLED
#define FRAME_LOG(...) n_printf(__VA_ARGS__); n_printf("\n")
#else
#define FRAME_LOG(...)
#endif

namespace Frame
{

//------------------------------------------------------------------------------
/**
*/
FrameSubpassBatch::FrameSubpassBatch()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
FrameSubpassBatch::~FrameSubpassBatch()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
FrameOp::Compiled* 
FrameSubpassBatch::AllocCompiled(Memory::ArenaAllocator<BIG_CHUNK>& allocator)
{
    CompiledImpl* ret = allocator.Alloc<CompiledImpl>();
    ret->batch = this->batch;
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
FrameSubpassBatch::DrawBatch(const CoreGraphics::CmdBufferId cmdBuf, CoreGraphics::BatchGroup::Code batch, const Graphics::GraphicsEntityId id)
{
        // now do usual render stuff
    ShaderServer* shaderServer = ShaderServer::Instance();
    ShaderConfigServer* matServer = ShaderConfigServer::Instance();

    // get current view and visibility draw list
    const Visibility::ObserverContext::VisibilityDrawList* drawList = Visibility::ObserverContext::GetVisibilityDrawList(id);

    const Util::Array<ShaderConfig*>& types = matServer->GetShaderConfigsByBatch(batch);
    if (types.Size() != 0 && (drawList != nullptr))
    {
        for (IndexT typeIdx = 0; typeIdx < types.Size(); typeIdx++)
        {
            ShaderConfig* shaderConfig = types[typeIdx];
            IndexT idx = drawList->visibilityTable.FindIndex(shaderConfig);
            if (idx != InvalidIndex)
            {
                N_CMD_SCOPE(cmdBuf, NEBULA_MARKER_DARK_GREEN, shaderConfig->GetName().AsCharPtr());

                // if BeginBatch returns true if this material type has a shader for this batch
                IndexT batchIndex = shaderConfig->BindShader(cmdBuf, batch);
                if (batchIndex != InvalidIndex)
                {
                    Visibility::ObserverContext::VisibilityBatchCommand visBatchCmd = drawList->visibilityTable.ValueAtIndex(shaderConfig, idx);
                    uint const start = visBatchCmd.packetOffset;
                    uint const end = visBatchCmd.packetOffset + visBatchCmd.numDrawPackets;
                    Visibility::ObserverContext::VisibilityModelCommand* visModelCmd = visBatchCmd.models.Begin();
                    Visibility::ObserverContext::VisibilityDrawCommand* visDrawCmd = visBatchCmd.draws.Begin();
                    uint32 numInstances = 0;
                    uint32 baseInstance = 0;
                    CoreGraphics::PrimitiveGroup primGroup;

                    Models::ShaderStateNode::DrawPacket* currentInstance = nullptr;
                    for (uint packetIndex = start; packetIndex < end; ++packetIndex)
                    {
                        Models::ShaderStateNode::DrawPacket* instance = drawList->drawPackets[packetIndex];

                        // If new model node, bind model resources (vertex buffer, index buffer, vertex layout, primitive group)
                        if (visModelCmd && visModelCmd->offset == packetIndex)
                        {
#if NEBULA_GRAPHICS_DEBUG
                            CoreGraphics::CmdInsertMarker(cmdBuf, NEBULA_MARKER_DARK_DARK_GREEN, visModelCmd->nodeName.Value());
#endif
                            // Run model setup (applies vertex/index buffer and vertex layout)
                            primGroup = visModelCmd->primitiveNodeApplyCallback();

                            if (primGroup.GetNumIndices() > 0 || primGroup.GetNumVertices() > 0)
                            {
                                visModelCmd->modelApplyCallback(cmdBuf);

                                // Bind graphics pipeline
                                CoreGraphics::CmdSetGraphicsPipeline(cmdBuf);

                                // Apply material
                                MaterialApply(visModelCmd->material, cmdBuf, batchIndex);
                            }

                            // Progress to next model command
                            visModelCmd++;

                            if (visModelCmd == visBatchCmd.models.End())
                                visModelCmd = nullptr;
                        }

                        // If new draw setup, progress to the next one
                        if (visDrawCmd && visDrawCmd->offset == packetIndex)
                        {
                            numInstances = visDrawCmd->numInstances;
                            baseInstance = visDrawCmd->baseInstance;
                            visDrawCmd++;

                            if (visDrawCmd == visBatchCmd.draws.End())
                                visDrawCmd = nullptr;
                        }

                        // Apply draw packet constants and draw
                        if (primGroup.GetNumIndices() > 0 || primGroup.GetNumVertices() > 0)
                        {
                            instance->Apply(cmdBuf, batchIndex, shaderConfig);
                            CoreGraphics::CmdDraw(cmdBuf, numInstances, baseInstance, primGroup);
                        }
                    }
                }
            }
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
FrameSubpassBatch::DrawBatch(const CoreGraphics::CmdBufferId cmdBuf, CoreGraphics::BatchGroup::Code batch, const Graphics::GraphicsEntityId id, const SizeT numInstances, const IndexT baseInstance)
{
    // now do usual render stuff
    ShaderServer* shaderServer = ShaderServer::Instance();
    ShaderConfigServer* matServer = ShaderConfigServer::Instance();

    // get current view and visibility draw list
    const Visibility::ObserverContext::VisibilityDrawList* drawList = Visibility::ObserverContext::GetVisibilityDrawList(id);

    const Util::Array<ShaderConfig*>& types = matServer->GetShaderConfigsByBatch(batch);
    if (types.Size() != 0 && (drawList != nullptr))
    {
        for (IndexT typeIdx = 0; typeIdx < types.Size(); typeIdx++)
        {
            ShaderConfig* shaderConfig = types[typeIdx];
            IndexT idx = drawList->visibilityTable.FindIndex(shaderConfig);
            if (idx != InvalidIndex)
            {
                N_CMD_SCOPE(cmdBuf, NEBULA_MARKER_DARK_GREEN, shaderConfig->GetName().AsCharPtr());

                // if BeginBatch returns true if this material type has a shader for this batch
                IndexT batchIndex = shaderConfig->BindShader(cmdBuf, batch);
                if (batchIndex != InvalidIndex)
                {
                    Visibility::ObserverContext::VisibilityBatchCommand visBatchCmd = drawList->visibilityTable.ValueAtIndex(shaderConfig, idx);
                    uint const start = visBatchCmd.packetOffset;
                    uint const end = visBatchCmd.packetOffset + visBatchCmd.numDrawPackets;
                    Visibility::ObserverContext::VisibilityModelCommand* visModelCmd = visBatchCmd.models.Begin();
                    Visibility::ObserverContext::VisibilityDrawCommand* visDrawCmd = visBatchCmd.draws.Begin();
                    uint32 baseNumInstances = 0;
                    uint32 baseBaseInstance = 0;
                    CoreGraphics::PrimitiveGroup primGroup;

                    Models::ShaderStateNode::DrawPacket* currentInstance = nullptr;
                    for (uint packetIndex = start; packetIndex < end; ++packetIndex)
                    {
                        Models::ShaderStateNode::DrawPacket* instance = drawList->drawPackets[packetIndex];

                        // If new model node, bind model resources (vertex buffer, index buffer, vertex layout, primitive group)
                        if (visModelCmd && visModelCmd->offset == packetIndex)
                        {
#if NEBULA_GRAPHICS_DEBUG
                            CoreGraphics::CmdInsertMarker(cmdBuf, NEBULA_MARKER_DARK_DARK_GREEN, visModelCmd->nodeName.Value());
#endif
                            // Run model setup (applies vertex/index buffer and vertex layout)
                            visModelCmd->modelApplyCallback(cmdBuf);
                            primGroup = visModelCmd->primitiveNodeApplyCallback();

                            // Bind graphics pipeline
                            CoreGraphics::CmdSetGraphicsPipeline(cmdBuf);

                            // Apply material
                            MaterialApply(visModelCmd->material, cmdBuf, batchIndex);

                            // Progress to next model command
                            visModelCmd++;

                            if (visModelCmd == visBatchCmd.models.End())
                                visModelCmd = nullptr;
                        }

                        // If new draw setup, progress to the next one
                        if (visDrawCmd && visDrawCmd->offset == packetIndex)
                        {
                            baseNumInstances = visDrawCmd->numInstances;
                            baseBaseInstance = visDrawCmd->baseInstance;
                            visDrawCmd++;

                            if (visDrawCmd == visBatchCmd.draws.End())
                                visDrawCmd = nullptr;
                        }

                        // Apply draw packet constants and draw
                        instance->Apply(cmdBuf, batchIndex, shaderConfig);
                        CoreGraphics::CmdDraw(cmdBuf, baseNumInstances * numInstances, baseBaseInstance + baseInstance, primGroup);
                    }
                }
            }
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
FrameSubpassBatch::CompiledImpl::Run(const CoreGraphics::CmdBufferId cmdBuf, const IndexT frameIndex, const IndexT bufferIndex)
{
    const Ptr<View>& view = Graphics::GraphicsServer::Instance()->GetCurrentView();
    FrameSubpassBatch::DrawBatch(cmdBuf, this->batch, view->GetCamera());
}

} // namespace Frame2
