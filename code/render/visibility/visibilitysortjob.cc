//------------------------------------------------------------------------------
//  visibilitysortjob.cc
//  (C) 2018-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "jobs/jobs.h"
#include "visibilitycontext.h"
#include "models/modelcontext.h"
#include "models/nodes/shaderstatenode.h"
#include "profiling/profiling.h"
#include "timing/timer.h"
namespace Visibility
{

//------------------------------------------------------------------------------
/**
*/
void
VisibilitySortJob(const Jobs::JobFuncContext& ctx)
{
    N_SCOPE(VisibilitySortJob, Visibility);
    auto packetAllocator = (Memory::ArenaAllocator<1024>* const)ctx.uniforms[0];
    auto nodes = (Models::ModelContext::ModelInstance::Renderable* const)ctx.uniforms[1];
    packetAllocator->Release();

    for (ptrdiff sliceIdx = 0; sliceIdx < ctx.numSlices; sliceIdx++)
    {
        auto visibilityResults = (Math::ClipStatus::Type*)N_JOB_INPUT(ctx, sliceIdx, 0);
        auto indices = (uint*)N_JOB_INPUT(ctx, sliceIdx, 1);
        auto drawList = (ObserverContext::VisibilityDrawList*)N_JOB_OUTPUT(ctx, sliceIdx, 0);

        // calculate amount of models
        uint32 numNodeInstances = ctx.inputSizes[0] / sizeof(Math::ClipStatus::Type);

        if (numNodeInstances == 0)
            break;

        Util::Array<uint64> indexBuffer;
        for (uint32 i = 0; i < numNodeInstances; i++)
        {
            // Make sure we're not exceeding the number of bits in the index buffer reserved for the actual node instance
            n_assert(indices[i] < 0xFFFFFFFF);
            indexBuffer.Append(indices[i]);
        }

        // loop over each node and give them the appropriate weight
        uint32 i = 0;
        while (i < indexBuffer.Size())
        {
            n_assert(indexBuffer[i] < 0x00000000FFFFFFFF);
            uint64 index = indexBuffer[i] & 0x00000000FFFFFFFF;

            // If not visible nor active, erase item from index list
            if (!AllBits(nodes->nodeFlags[index], Models::NodeInstanceFlags::NodeInstance_Active) || visibilityResults[index] == Math::ClipStatus::Outside)
            {
                indexBuffer.EraseIndexSwap(i);
                continue;
            }
            else
            {
                // Set the node visible flag (use this to figure out if a node is seen by __any__ observer)
                nodes->nodeFlags[index] = SetBits(nodes->nodeFlags[index], Models::NodeInstance_Visible);
            }

            // Get sort id and combine with index to get full sort id
            uint64 sortId = nodes->nodeSortId[index];
            indexBuffer[i] = sortId | index;
            i++;
        }

        if (indexBuffer.IsEmpty())
            return; // early out

        // sort the index buffer
        std::qsort(indexBuffer.Begin(), indexBuffer.Size(), sizeof(uint64), [](const void* a, const void* b)
        {
            uint64 arg1 = *static_cast<const uint64*>(a);
            uint64 arg2 = *static_cast<const uint64*>(b);
            return (arg1 > arg2) - (arg1 < arg2);
        });
       
        // Now resolve the indexbuffer into draw commands
        uint32 numDraws = 0;
        const uint32 numPackets = indexBuffer.Size();
        drawList->drawPackets.Reserve(numPackets);

        // Allocate single command which we can 
        ObserverContext::VisibilityBatchCommand* cmd = nullptr;
        Models::ModelNode* node = nullptr;
        static auto NullDrawModifiers = Util::MakeTuple(UINT32_MAX, UINT32_MAX);
        Util::Tuple<uint32, uint32> drawModifiers = NullDrawModifiers;
        Materials::ShaderConfig* currentMaterialType = nullptr;

        for (uint32 i = 0; i < numPackets; i++)
        {
            uint32 index = indexBuffer[i] & 0x00000000FFFFFFFF;

            // If new material, add a new entry into the lookup table
            auto otherMaterialType = nodes->nodeMaterialTypes[index];
            if (currentMaterialType != otherMaterialType)
            {
                // Add new draw command and get reference to it
                IndexT entry = drawList->visibilityTable.Add(otherMaterialType, ObserverContext::VisibilityBatchCommand());
                cmd = &drawList->visibilityTable.ValueAtIndex(otherMaterialType, entry);

                // Setup initial state for command
                cmd->packetOffset = numDraws;
                cmd->numDrawPackets = 0;

                node = nullptr;
                drawModifiers = NullDrawModifiers;
                currentMaterialType = otherMaterialType;
            }
            n_assert(cmd != nullptr);

            // If a new node (resource), add a model apply command
            auto otherNode = nodes->nodes[index];
            if (node != otherNode)
            {
                ObserverContext::VisibilityModelCommand& batchCmd = cmd->models.Emplace();

                // The offset of the command corresponds to where in the VisibilityBatchCommand batch the model should be applied
                batchCmd.offset = cmd->packetOffset + cmd->numDrawPackets;
                batchCmd.modelCallback = nodes->nodeModelCallbacks[index];
                batchCmd.surface = nodes->nodeSurfaces[index];

#if NEBULA_GRAPHICS_DEBUG
                batchCmd.nodeName = nodes->nodeNames[index];
#endif
                node = otherNode;
            }

            // If a new set of draw modifiers (instance count and base instance) are used, insert a new draw command
            auto otherDrawModifiers = nodes->nodeDrawModifiers[index];
            if (drawModifiers != otherDrawModifiers)
            {
                ObserverContext::VisibilityDrawCommand& drawCmd = cmd->draws.Emplace();
                drawCmd.offset = cmd->packetOffset + cmd->numDrawPackets;
                drawCmd.numInstances = Util::Get<0>(otherDrawModifiers);
                drawCmd.baseInstance = Util::Get<1>(otherDrawModifiers);

                drawModifiers = otherDrawModifiers;
            }

            // allocate memory for draw packet
            void* mem = packetAllocator->Alloc(sizeof(Models::ShaderStateNode::DrawPacket));

            // update packet and add to list
            Models::ShaderStateNode::DrawPacket* packet = reinterpret_cast<Models::ShaderStateNode::DrawPacket*>(mem);
            packet->numOffsets[0] = nodes->nodeStates[index].resourceTableOffsets.Size();
            packet->numTables = 1;
            packet->tables[0] = nodes->nodeStates[index].resourceTable;
            packet->surfaceInstance = nodes->nodeStates[index].surfaceInstance;
#ifndef PUBLIC_BUILD
            packet->boundingBox = nodes->nodeBoundingBoxes[index];
            packet->nodeInstanceHash = nodes->nodes[index]->HashCode();
#endif
            memcpy(packet->offsets[0], nodes->nodeStates[index].resourceTableOffsets.Begin(), nodes->nodeStates[index].resourceTableOffsets.ByteSize());
            packet->slots[0] = NEBULA_DYNAMIC_OFFSET_GROUP;
            drawList->drawPackets.Append(packet);
            cmd->numDrawPackets++;
            numDraws++;
        }
    }
}

} // namespace Visibility
