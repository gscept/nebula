//------------------------------------------------------------------------------
// framesubpassbatch.cc
// (C) 2016-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "framesubpassbatch.h"
#include "coregraphics/shaderserver.h"
#include "coregraphics/graphicsdevice.h"
#include "materials/materialserver.h"
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
FrameSubpassBatch::DrawBatch(CoreGraphics::BatchGroup::Code batch, const Graphics::GraphicsEntityId id)
{
    // now do usual render stuff
    ShaderServer* shaderServer = ShaderServer::Instance();
    MaterialServer* matServer = MaterialServer::Instance();

    // get current view and visibility draw list
    const Visibility::ObserverContext::VisibilityDrawList* drawList = Visibility::ObserverContext::GetVisibilityDrawList(id);

    // start batch
    CoreGraphics::BeginBatch(FrameBatchType::Geometry);

    const Util::Array<MaterialType*>* types = matServer->GetMaterialTypesByBatch(batch);
    if ((types != nullptr) && (drawList != nullptr))
    {
        for (IndexT typeIdx = 0; typeIdx < types->Size(); typeIdx++)
        {
            MaterialType* materialType = (*types)[typeIdx];
            IndexT idx = drawList->visibilityTable.FindIndex(materialType);
            if (idx != InvalidIndex)
            {

#if NEBULA_GRAPHICS_DEBUG
                CommandBufferBeginMarker(GraphicsQueueType, NEBULA_MARKER_DARK_GREEN, materialType->GetName().AsCharPtr());
#endif

                // if BeginBatch returns true if this material type has a shader for this batch
                if (Materials::MaterialBeginBatch(materialType, batch))
                {
                    Visibility::ObserverContext::VisibilityBatchCommand visBatchCmd = drawList->visibilityTable.ValueAtIndex(materialType, idx);
                    uint const start = visBatchCmd.packetOffset;
                    uint const end = visBatchCmd.packetOffset + visBatchCmd.numDrawPackets;
                    Visibility::ObserverContext::VisibilityModelCommand* visModelCmd = visBatchCmd.models.Begin();
                    Visibility::ObserverContext::VisibilityDrawCommand* visDrawCmd = visBatchCmd.draws.Begin();
                    uint32 numInstances = 0;
                    uint32 baseInstance = 0;

                    Models::ShaderStateNode::DrawPacket* currentInstance = nullptr;
                    for (uint packetIndex = start; packetIndex < end; ++packetIndex)
                    {
                        Models::ShaderStateNode::DrawPacket* instance = drawList->drawPackets[packetIndex];

                        // If new model node, bind model resources (vertex buffer, index buffer, vertex layout, primitive group)
                        if (visModelCmd && visModelCmd->offset == packetIndex)
                        {
#if NEBULA_GRAPHICS_DEBUG
                            CommandBufferInsertMarker(GraphicsQueueType, NEBULA_MARKER_DARK_DARK_GREEN, visModelCmd->nodeName.Value());
#endif
                            // Run model setup (applies vertex/index buffer and vertex layout)
                            visModelCmd->modelCallback();

                            // Bind graphics pipeline
                            CoreGraphics::SetGraphicsPipeline();

                            // Apply surface
                            Materials::MaterialApplySurface(materialType, visModelCmd->surface);

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
                        instance->Apply(materialType);
                        CoreGraphics::Draw(numInstances, baseInstance);
                    }
                }
                Materials::MaterialEndBatch(materialType);

#if NEBULA_GRAPHICS_DEBUG
                CommandBufferEndMarker(GraphicsQueueType);
#endif
            }
        }
    }

    // end batch
    CoreGraphics::EndBatch();
}

//------------------------------------------------------------------------------
/**
*/
void 
FrameSubpassBatch::DrawBatch(CoreGraphics::BatchGroup::Code batch, const Graphics::GraphicsEntityId id, const SizeT numInstances, const IndexT baseInstance)
{
    // now do usual render stuff
    ShaderServer* shaderServer = ShaderServer::Instance();
    MaterialServer* matServer = MaterialServer::Instance();

    // get current view and visibility draw list
    const Visibility::ObserverContext::VisibilityDrawList* drawList = Visibility::ObserverContext::GetVisibilityDrawList(id);

    // start batch
    CoreGraphics::BeginBatch(FrameBatchType::Geometry);

    const Util::Array<MaterialType*>* types = matServer->GetMaterialTypesByBatch(batch);
    if ((types != nullptr) && (drawList != nullptr))
    {
        for (IndexT typeIdx = 0; typeIdx < types->Size(); typeIdx++)
        {
            MaterialType* materialType = (*types)[typeIdx];
            IndexT idx = drawList->visibilityTable.FindIndex(materialType);
            if (idx != InvalidIndex)
            {

#if NEBULA_GRAPHICS_DEBUG
                CommandBufferBeginMarker(GraphicsQueueType, NEBULA_MARKER_DARK_GREEN, materialType->GetName().AsCharPtr());
#endif

                // if BeginBatch returns true if this material type has a shader for this batch
                if (Materials::MaterialBeginBatch(materialType, batch))
                {
                    Visibility::ObserverContext::VisibilityBatchCommand visBatchCmd = drawList->visibilityTable.ValueAtIndex(materialType, idx);
                    uint const start = visBatchCmd.packetOffset;
                    uint const end = visBatchCmd.packetOffset + visBatchCmd.numDrawPackets;
                    Visibility::ObserverContext::VisibilityModelCommand* visModelCmd = visBatchCmd.models.Begin();
                    Visibility::ObserverContext::VisibilityDrawCommand* visDrawCmd = visBatchCmd.draws.Begin();
                    uint32 baseNumInstances = 0;
                    uint32 baseBaseInstance = 0;

                    Models::ShaderStateNode::DrawPacket* currentInstance = nullptr;
                    for (uint packetIndex = start; packetIndex < end; ++packetIndex)
                    {
                        Models::ShaderStateNode::DrawPacket* instance = drawList->drawPackets[packetIndex];

                        // If new model node, update model callback and 
                        if (visModelCmd->offset == packetIndex)
                        {
#if NEBULA_GRAPHICS_DEBUG
                            CommandBufferInsertMarker(GraphicsQueueType, NEBULA_MARKER_DARK_DARK_GREEN, visModelCmd->nodeName.Value());
#endif
                            // Run model setup (applies vertex/index buffer and vertex layout)
                            visModelCmd->modelCallback();

                            // bind graphics pipeline
                            CoreGraphics::SetGraphicsPipeline();

                            // apply surface
                            Materials::MaterialApplySurface(materialType, visModelCmd->surface);

                            visModelCmd++;
                        }

                        // If new draw setup, progress to the next one
                        if (visDrawCmd->offset == packetIndex)
                        {
                            baseNumInstances = visDrawCmd->numInstances;
                            baseBaseInstance = visDrawCmd->baseInstance;
                            visDrawCmd++;
                        }

                        // Apply draw packet constants and draw
                        instance->Apply(materialType);
                        CoreGraphics::Draw(baseNumInstances * numInstances, baseBaseInstance + baseInstance);
                    }
                }
                Materials::MaterialEndBatch(materialType);

#if NEBULA_GRAPHICS_DEBUG
                CommandBufferEndMarker(GraphicsQueueType);
#endif
            }
        }
    }

    // end batch
    CoreGraphics::EndBatch();
}

//------------------------------------------------------------------------------
/**
*/
void
FrameSubpassBatch::CompiledImpl::Run(const IndexT frameIndex, const IndexT bufferIndex)
{
    const Ptr<View>& view = Graphics::GraphicsServer::Instance()->GetCurrentView();
    FrameSubpassBatch::DrawBatch(this->batch, view->GetCamera());
}

} // namespace Frame2
