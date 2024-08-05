//------------------------------------------------------------------------------
//  @file framescripts.cc
//  @copyright (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "framescripts.h"
#include "visibility/visibilitycontext.h"
#include "materials/materialtemplates.h"

namespace Frame
{

//------------------------------------------------------------------------------
/**
*/
void
DrawBatch(const CoreGraphics::CmdBufferId cmdBuf, MaterialTemplates::BatchGroup batch, const Graphics::GraphicsEntityId id, const IndexT bufferIndex)
{
    const Visibility::ObserverContext::VisibilityDrawList* drawList = Visibility::ObserverContext::GetVisibilityDrawList(id);
    const Util::Array<MaterialTemplates::Entry*>& types = MaterialTemplates::Configs[(uint)batch];
    if (types.Size() != 0 && (drawList != nullptr))
    {
        for (IndexT typeIdx = 0; typeIdx < types.Size(); typeIdx++)
        {
            MaterialTemplates::Entry* type = types[typeIdx];
            IndexT idx = drawList->visibilityTable.FindIndex(type);
            if (idx != InvalidIndex)
            {
                N_CMD_SCOPE(cmdBuf, NEBULA_MARKER_DARK_GREEN, type->name);

                // if BeginBatch returns true if this material type has a shader for this batch
                IndexT batchIndex = type->passes.FindIndex(batch);
                if (batchIndex != InvalidIndex)
                {
                    // Bind shader
                    const auto& pass = type->passes.ValueAtIndex(batchIndex);
                    CoreGraphics::CmdSetShaderProgram(cmdBuf, pass->program);
                    const Visibility::ObserverContext::VisibilityBatchCommand& visBatchCmd = drawList->visibilityTable.ValueAtIndex(type, idx);
                    uint const start = visBatchCmd.packetOffset;
                    uint const end = visBatchCmd.packetOffset + visBatchCmd.numDrawPackets;
                    Visibility::ObserverContext::VisibilityModelCommand* visModelCmd = visBatchCmd.models.Begin();
                    Visibility::ObserverContext::VisibilityDrawCommand* visDrawCmd = visBatchCmd.draws.Begin();
                    uint32 numInstances = 0;
                    uint32 baseInstance = 0;
                    CoreGraphics::PrimitiveGroup primGroup;
                    CoreGraphics::MeshId mesh = CoreGraphics::InvalidMeshId;

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
                            primGroup = visModelCmd->primitiveGroup;

                            if (primGroup.GetNumIndices() > 0 || primGroup.GetNumVertices() > 0)
                            {
                                if (visModelCmd != nullptr && mesh != visModelCmd->mesh)
                                {
                                    CoreGraphics::MeshBind(visModelCmd->mesh, cmdBuf);
                                    mesh = visModelCmd->mesh;

                                    // Bind graphics pipeline
                                    CoreGraphics::CmdSetGraphicsPipeline(cmdBuf);
                                }

                                // Apply material
                                MaterialApply(visModelCmd->material, cmdBuf, pass->index);
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
                            instance->Apply(cmdBuf, pass->index, bufferIndex);
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
DrawBatch(const CoreGraphics::CmdBufferId cmdBuf, MaterialTemplates::BatchGroup batch, const Graphics::GraphicsEntityId id, const SizeT numInstances, const IndexT baseInstance, const IndexT bufferIndex)
{
    const Visibility::ObserverContext::VisibilityDrawList* drawList = Visibility::ObserverContext::GetVisibilityDrawList(id);
    const Util::Array<MaterialTemplates::Entry*>& types = MaterialTemplates::Configs[(uint)batch];

    if (types.Size() != 0 && (drawList != nullptr))
    {
        for (IndexT typeIdx = 0; typeIdx < types.Size(); typeIdx++)
        {
            MaterialTemplates::Entry* type = types[typeIdx];
            IndexT idx = drawList->visibilityTable.FindIndex(type);
            if (idx != InvalidIndex)
            {
                N_CMD_SCOPE(cmdBuf, NEBULA_MARKER_DARK_GREEN, type->name);

                // if BeginBatch returns true if this material type has a shader for this batch
                IndexT batchIndex = type->passes.FindIndex(batch);
                if (batchIndex != InvalidIndex)
                {
                    const auto& pass = type->passes.ValueAtIndex(batchIndex);
                    CoreGraphics::CmdSetShaderProgram(cmdBuf, pass->program);
                    const Visibility::ObserverContext::VisibilityBatchCommand& visBatchCmd = drawList->visibilityTable.ValueAtIndex(type, idx);
                    uint const start = visBatchCmd.packetOffset;
                    uint const end = visBatchCmd.packetOffset + visBatchCmd.numDrawPackets;
                    Visibility::ObserverContext::VisibilityModelCommand* visModelCmd = visBatchCmd.models.Begin();
                    Visibility::ObserverContext::VisibilityDrawCommand* visDrawCmd = visBatchCmd.draws.Begin();
                    uint32 baseNumInstances = 0;
                    uint32 baseBaseInstance = 0;
                    CoreGraphics::PrimitiveGroup primGroup;
                    CoreGraphics::MeshId mesh = CoreGraphics::InvalidMeshId;

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
                            primGroup = visModelCmd->primitiveGroup;

                            if (primGroup.GetNumIndices() > 0 || primGroup.GetNumVertices() > 0)
                            {
                                if (mesh != visModelCmd->mesh)
                                {
                                    CoreGraphics::MeshBind(visModelCmd->mesh, cmdBuf);
                                    mesh = visModelCmd->mesh;

                                    // Bind graphics pipeline
                                    CoreGraphics::CmdSetGraphicsPipeline(cmdBuf);
                                }

                                // Apply material
                                MaterialApply(visModelCmd->material, cmdBuf, pass->index);
                            }

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
                        instance->Apply(cmdBuf, pass->index, bufferIndex);
                        CoreGraphics::CmdDraw(cmdBuf, baseNumInstances * numInstances, baseBaseInstance + baseInstance, primGroup);
                    }
                }
            }
        }
    }
}

} // namespace Frame
