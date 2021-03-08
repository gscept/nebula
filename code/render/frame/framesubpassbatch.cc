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
                    Visibility::ObserverContext::VisibilityDrawCommand visDrawCmd = drawList->visibilityTable.ValueAtIndex(materialType, idx);
                    uint const start = visDrawCmd.packetOffset;
                    uint const end = visDrawCmd.packetOffset + visDrawCmd.numDrawPackets;
                    
                    Models::ModelNode* currentNode = nullptr;
                    for (uint packetIndex = start; packetIndex < end; ++packetIndex)
                    {
                        Models::ShaderStateNode::DrawPacket* instance = drawList->drawPackets[packetIndex];
                        Models::ModelNode::Instance* nodeInst = instance->ToNode<Models::ModelNode::Instance>();
                        Models::ModelNode* node = nodeInst->node;
                        Models::ShaderStateNode* stateNode = reinterpret_cast<Models::ShaderStateNode*>(node);
                        if (currentNode != nodeInst->node)
                        {
#if NEBULA_GRAPHICS_DEBUG
                            CommandBufferInsertMarker(GraphicsQueueType, NEBULA_MARKER_DARK_DARK_GREEN, node->GetName().Value());
#endif
                            // apply node-wide state
                            node->ApplyNodeState();

                            // bind graphics pipeline
                            CoreGraphics::SetGraphicsPipeline();

                            // apply node-wide resources
                            node->ApplyNodeResources();

                            // apply surface
                            Materials::MaterialApplySurface(materialType, stateNode->GetSurface());
                        }
                        
                        Models::NodeType type = node->GetType();

                        instance->Apply(materialType);
                        if (type != ParticleSystemNodeType)
                        {
                            Models::PrimitiveNode::Instance* pinst = reinterpret_cast<Models::PrimitiveNode::Instance*>(nodeInst);
                            pinst->Draw(1, 0, instance);
                        }
                        else
                        {
                            Models::ParticleSystemNode::Instance* pinst = reinterpret_cast<Models::ParticleSystemNode::Instance*>(nodeInst);
                            pinst->Draw(1, 0, instance);
                        }
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
                    Visibility::ObserverContext::VisibilityDrawCommand visDrawCmd = drawList->visibilityTable.ValueAtIndex(materialType, idx);
                    uint const start = visDrawCmd.packetOffset;
                    uint const end = visDrawCmd.packetOffset + visDrawCmd.numDrawPackets;

                    Models::ModelNode* currentNode = nullptr;
                    for (uint packetIndex = start; packetIndex < end; ++packetIndex)
                    {
                        Models::ShaderStateNode::DrawPacket* instance = drawList->drawPackets[packetIndex];
                        Models::ModelNode::Instance* nodeInst = instance->ToNode<Models::ModelNode::Instance>();
                        Models::ModelNode* node = nodeInst->node;
                        Models::ShaderStateNode* stateNode = reinterpret_cast<Models::ShaderStateNode*>(node);
                        if (currentNode != nodeInst->node)
                        {
#if NEBULA_GRAPHICS_DEBUG
                            CommandBufferInsertMarker(GraphicsQueueType, NEBULA_MARKER_DARK_DARK_GREEN, node->GetName().Value());
#endif
                            // apply node-wide state
                            node->ApplyNodeState();

                            // bind graphics pipeline
                            CoreGraphics::SetGraphicsPipeline();

                            // apply node-wide resources
                            node->ApplyNodeResources();

                            // apply surface
                            Materials::MaterialApplySurface(materialType, stateNode->GetSurface());
                        }

                        Models::NodeType type = node->GetType();

                        instance->Apply(materialType);
                        if (type != ParticleSystemNodeType)
                        {
                            Models::PrimitiveNode::Instance* pinst = reinterpret_cast<Models::PrimitiveNode::Instance*>(nodeInst);
                            pinst->Draw(1, baseInstance, instance);
                        }
                        else
                        {
                            Models::ParticleSystemNode::Instance* pinst = reinterpret_cast<Models::ParticleSystemNode::Instance*>(nodeInst);
                            pinst->Draw(1, baseInstance, instance);
                        }
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
