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
    Memory::ArenaAllocator<1024>* packetAllocator = (Memory::ArenaAllocator<1024>*)ctx.uniforms[0];
    packetAllocator->Release();

    for (ptrdiff sliceIdx = 0; sliceIdx < ctx.numSlices; sliceIdx++)
    { 
        Math::ClipStatus::Type* results = (Math::ClipStatus::Type*)N_JOB_INPUT(ctx, sliceIdx, 0);
        Models::ModelContext::ModelInstance::Renderable* const nodes = (Models::ModelContext::ModelInstance::Renderable*)N_JOB_INPUT(ctx, sliceIdx, 1);
        ObserverContext::VisibilityDrawList* drawList = (ObserverContext::VisibilityDrawList*)N_JOB_OUTPUT(ctx, sliceIdx, 0);

        // calculate amount of models
        uint32 numNodeInstances = ctx.inputSizes[0] / sizeof(Math::ClipStatus::Type);

        if (numNodeInstances == 0)
            break;

        n_assert(numNodeInstances < 0xFFFFFFFF)
        
        Util::Array<uint64> indexBuffer;
        for (uint32 i = 0; i < numNodeInstances; i++)
            indexBuffer.Append(i);

        // loop over each node and give them the appropriate weight
        uint32 i = 0;
        while (i < indexBuffer.Size())
        {
            n_assert(indexBuffer[i] < 0x00000000FFFFFFFF);
            uint64 index = indexBuffer[i] & 0x00000000FFFFFFFF;

            // If not visible nor active, erase item from index list
            if (!AllBits(nodes->nodeFlags, Models::NodeInstanceFlags::NodeInstance_Active | Models::NodeInstanceFlags::NodeInstance_Visible))
            {
                indexBuffer.EraseIndexSwap(i);
                continue;
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
        
        Materials::MaterialType* currentMaterialType = nodes->nodeMaterialTypes[indexBuffer[0] & 0x00000000FFFFFFFF];

        for (uint32 i = 0; i < numPackets; i++)
        {
            uint32 index = indexBuffer[i] & 0x00000000FFFFFFFF;
            Materials::MaterialType* otherMaterialType = nodes->nodeMaterialTypes[index];
            if (currentMaterialType != otherMaterialType)
            {
                ObserverContext::VisibilityDrawCommand cmd;
                cmd.packetOffset = drawList->drawPackets.Size() - numDraws;
                cmd.numDrawPackets = numDraws;
                drawList->visibilityTable.Add(currentMaterialType, cmd);
                currentMaterialType = otherMaterialType;
                numDraws = 0;
            }

            // allocate memory for draw packet
            void* mem = packetAllocator->Alloc(sizeof(Models::ShaderStateNode::DrawPacket));

            // update packet and add to list
            Models::ShaderStateNode::DrawPacket* packet = reinterpret_cast<Models::ShaderStateNode::DrawPacket*>(mem);
            packet->node = nodes->nodes[index];
            packet->numOffsets[0] = nodes->nodeStates[index].resourceTableOffsets.Size();
            packet->numTables = 1;
            packet->tables[0] = nodes->nodeStates[index].resourceTable;
            packet->offsets[0] = nodes->nodeStates[index].resourceTableOffsets.Begin();
            memcpy(&packet->offsets[0][0], nodes->nodeStates[index].resourceTableOffsets.Begin(), nodes->nodeStates[index].resourceTableOffsets.ByteSize());
            packet->slots[0] = NEBULA_DYNAMIC_OFFSET_GROUP;
            drawList->drawPackets.Append(packet);
            numDraws++;
        }

        // make sure to not miss the last couple of draw packets material
        if (numDraws) 
        {
            ObserverContext::VisibilityDrawCommand cmd;
            cmd.packetOffset = drawList->drawPackets.Size() - numDraws;
            cmd.numDrawPackets = numDraws;
            drawList->visibilityTable.Add(currentMaterialType, cmd);
        }
    }
}

} // namespace Visibility
