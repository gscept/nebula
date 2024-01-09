//------------------------------------------------------------------------------
//  visobservercontext.cc
//  (C) 2018-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "visibilitycontext.h"
#include "graphics/graphicsserver.h"

#include "graphics/cameracontext.h"
#include "particles/particlecontext.h"
#include "lighting/lightcontext.h"
#include "lighting/lightprobecontext.h"
#include "models/modelcontext.h"
#include "particles/particlecontext.h"
#include "characters/charactercontext.h"

#include "systems/boxsystem.h"
#include "systems/octreesystem.h"
#include "systems/portalsystem.h"
#include "systems/quadtreesystem.h"
#include "systems/bruteforcesystem.h"

#include "profiling/profiling.h"

#include "util/randomnumbertable.h"

#include "jobs2/jobs2.h"

#ifndef PUBLIC_BUILD
#include "imgui.h"
#endif

namespace Visibility
{

ObserverContext::ObserverAllocator ObserverContext::observerAllocator;
ObservableContext::ObservableAllocator ObservableContext::observableAllocator;

Util::Array<VisibilitySystem*> ObserverContext::systems;

static Util::Queue<Threading::Event*> waitEvents;

__ImplementContext(ObserverContext, ObserverContext::observerAllocator);

//------------------------------------------------------------------------------
/**
*/
void
ObserverContext::Setup(const Graphics::GraphicsEntityId id, VisibilityEntityType entityType, bool isOrtho)
{
    const Graphics::ContextEntityId cid = GetContextId(id);
    observerAllocator.Set<Observer_EntityType>(cid.id, entityType);
    observerAllocator.Set<Observer_EntityId>(cid.id, id);
    observerAllocator.Set<Observer_IsOrtho>(cid.id, isOrtho);
}

//------------------------------------------------------------------------------
/**
*/
void 
ObserverContext::MakeDependency(const Graphics::GraphicsEntityId a, const Graphics::GraphicsEntityId b, const DependencyMode mode)
{
    const Graphics::ContextEntityId cid = GetContextId(b);
    observerAllocator.Set<Observer_Dependency>(cid.id, a);
    observerAllocator.Set<Observer_DependencyMode>(cid.id, mode);
}

//------------------------------------------------------------------------------
/**
*/
void 
ObserverContext::RunVisibilityTests(const Graphics::FrameContext& ctx)
{
    N_SCOPE(RunVisibilityTests, Visibility);

    const Models::ModelContext::ModelInstance::Renderable& NodeInstances = Models::ModelContext::GetModelRenderables();

    Util::Array<Math::mat4>& observerTransforms = observerAllocator.GetArray<Observer_Matrix>();
    const Util::Array<bool>& observerIsOrthogonal = observerAllocator.GetArray<Observer_IsOrtho>();
    const Util::Array<Graphics::GraphicsEntityId>& observerIds = observerAllocator.GetArray<Observer_EntityId>();
    const Util::Array<VisibilityEntityType>& observerTypes = observerAllocator.GetArray<Observer_EntityType>();
    Util::Array<VisibilityResultArray>& observerResults = observerAllocator.GetArray<Observer_ResultArray>();

    IndexT i;
    for (i = 0; i < observerIds.Size(); i++)
    {
        const Graphics::GraphicsEntityId id = observerIds[i];
        const VisibilityEntityType type = observerTypes[i];

        if (id == Graphics::GraphicsEntityId::Invalid())
            continue;

        switch (type)
        {
        case Camera:
            observerTransforms[i] = Graphics::CameraContext::GetViewProjection(id);
            break;
        case Light:
            observerTransforms[i] = Lighting::LightContext::GetObserverTransform(id);
            break;
        case LightProbe:
            observerTransforms[i] = Graphics::LightProbeContext::GetTransform(id);
            break;
        default: n_error("unhandled enum"); break;
        }
    }

    // reset all lists to that all entities are visible
    for (i = 0; i < observerResults.Size(); i++)
    {
        VisibilityDrawList& visibilities = observerAllocator.Get<Observer_DrawList>(i);
        observerResults[i].Fill(0, observerResults[i].Size(), Math::ClipStatus::Type::Outside);
        visibilities.visibilityTable.Clear();
        visibilities.drawPackets.Clear();
    }

    // prepare visibility systems
    if (observerTransforms.Size() > 0)
    {
        for (i = 0; i < ObserverContext::systems.Size(); i++)
        {
            VisibilitySystem* sys = ObserverContext::systems[i];
            sys->PrepareObservers(observerTransforms.Begin(), observerIsOrthogonal.Begin(), observerResults.Begin(), observerTransforms.Size());
        }
    }

    // setup observerable entities
    const Util::Array<Graphics::GraphicsEntityId>& ids = ObservableContext::observableAllocator.GetArray<Observable_EntityId>();
    static Util::Array<uint32> nodes;
    nodes.Clear();
    nodes.Resize(observerResults[0].Size());

    static Threading::AtomicCounter idCounter;
    idCounter = 1;
    if (nodes.Size() > 0)
    {
        Threading::AtomicCounter counter;
        counter = 0;

        // Run job to collect model node ids
        Jobs2::JobDispatch(
            [idData = ids.Begin(), nodeData = nodes.Begin(), counter](SizeT totalJobs, SizeT groupSize, IndexT groupIndex, SizeT invocationOffset) mutable
        {
            N_SCOPE(VisibilityIdCollectJob, Graphics);
            for (IndexT i = 0; i < groupSize; i++)
            {
                IndexT index = i + invocationOffset;
                if (index >= totalJobs)
                    return;

                // Get node range and update ids buffer
                const Models::NodeInstanceRange& NodeInstances = Models::ModelContext::GetModelRenderableRange(idData[index]);
                const uint numNodes = NodeInstances.end - NodeInstances.begin;
                uint offset = Threading::Interlocked::Add(&counter, numNodes);
                for (IndexT j = NodeInstances.begin; j < NodeInstances.end; j++)
                    nodeData[offset++] = j;
            }
        }, ids.Size(), 1024, {}, &idCounter, nullptr);
        
        for (i = 0; i < ObserverContext::systems.Size(); i++)
        {
            VisibilitySystem* sys = ObserverContext::systems[i];
            sys->PrepareEntities(NodeInstances.nodeBoundingBoxes.Begin(), nodes.Begin(), ids.Begin(), reinterpret_cast<uint32_t*>(NodeInstances.nodeFlags.Begin()), nodes.Size());
        }
    }

    // run all visibility systems
    const Threading::AtomicCounter* const* prevSystemCounters = nullptr;
    if ((observerTransforms.Size() > 0) && (NodeInstances.nodeBoundingBoxes.Size() > 0))
    {
        for (i = 0; i < ObserverContext::systems.Size(); i++)
        {
            VisibilitySystem* sys = ObserverContext::systems[i];

            // Wait for transforms and bounding box updates to finish before we do the visibility testing
            sys->Run(prevSystemCounters, { &idCounter, &Particles::ParticleContext::ConstantUpdateCounter, &Models::ModelContext::TransformsUpdateCounter });
            prevSystemCounters = sys->GetCompletionCounters();
        }
    }

    static Threading::AtomicCounter completionCounter;
    completionCounter = observerResults.Size();
    Threading::Event* finishedEvent = new Threading::Event;

    for (i = 0; i < observerResults.Size(); i++)
    {
        // early abort empty visibility queries
        if (NodeInstances.nodeStates.Size() == 0)
        {
            continue;
        }

        const VisibilityResultArray& results = observerResults[i];
        VisibilityDrawList& visibilities = observerAllocator.Get<Observer_DrawList>(i);
        Memory::ArenaAllocator<1024>& allocator = observerAllocator.Get<Observer_DrawListAllocator>(i);

        struct Context
        {
            Math::ClipStatus::Type* clipStatuses;
            uint32* ids;
            Visibility::ObserverContext::VisibilityDrawList* drawList;
            Memory::ArenaAllocator<1024>* allocator;
            const Models::ModelContext::ModelInstance::Renderable* renderables;
        } jobCtx;

        jobCtx.clipStatuses = results.Begin();
        jobCtx.ids = nodes.Begin();
        jobCtx.drawList = &visibilities;
        jobCtx.allocator = &allocator;
        jobCtx.renderables = &NodeInstances;

        // Before we create our draws, we have to wait for the constants to be allocated first
        // For particles, that's done before visibility so we can omit it here
        Util::Array<const Threading::AtomicCounter*> waitCounters =
        {
            prevSystemCounters[i],
            &Models::ModelContext::ConstantsUpdateCounter,
            &Characters::CharacterContext::ConstantUpdateCounter,
        };

        Jobs2::JobDispatch([](SizeT totalJobs, SizeT groupSize, IndexT groupIndex, SizeT invocationOffset, void* ctx)
        {
            N_SCOPE(VisibilitySortJob, Graphics);
            auto context = static_cast<Context*>(ctx);
            context->allocator->Release();

            // calculate amount of models
            uint32 numNodeInstances = totalJobs;

            if (numNodeInstances == 0)
                return;

            Util::Array<uint64> indexBuffer(numNodeInstances, 0);
            Util::Array<Math::ClipStatus::Type> clipStatuses(context->clipStatuses, numNodeInstances);
            for (uint32 i = 0; i < numNodeInstances; i++)
            {
                // Make sure we're not exceeding the number of bits in the index buffer reserved for the actual node instance
                n_assert(context->ids[i] < 0xFFFFFFFF);
                indexBuffer.Append(context->ids[i]);
            }

            // loop over each node and give them the appropriate weight
            uint32 i = 0;
            while (i < indexBuffer.Size())
            {
                n_assert(indexBuffer[i] < 0x00000000FFFFFFFF);
                uint64 index = indexBuffer[i] & 0x00000000FFFFFFFF;
                Math::ClipStatus::Type clipStatus = clipStatuses[i];

                // If not visible nor active, erase item from index list
                if (!AllBits(context->renderables->nodeFlags[index], Models::NodeInstanceFlags::NodeInstance_Active)
                    || clipStatus == Math::ClipStatus::Outside)
                {
                    indexBuffer.EraseIndexSwap(i);
                    clipStatuses.EraseIndexSwap(i);
                    continue;
                }
                else
                {
                    // Set the node visible flag (use this to figure out if a node is seen by __any__ observer)
                    context->renderables->nodeFlags[index] = SetBits(context->renderables->nodeFlags[index], Models::NodeInstanceFlags::NodeInstance_Visible);
                }

                // Get sort id and combine with index to get full sort id
                uint64 sortId = context->renderables->nodeSortId[index];
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
            context->drawList->drawPackets.Reserve(numPackets);

            // Allocate single command which we can 
            ObserverContext::VisibilityBatchCommand* cmd = nullptr;
            CoreGraphics::MeshId mesh = CoreGraphics::InvalidMeshId;
            Materials::MaterialId mat = Materials::InvalidMaterialId;
            static auto NullDrawModifiers = Util::MakeTuple(UINT32_MAX, UINT32_MAX);
            Util::Tuple<uint32, uint32> drawModifiers = NullDrawModifiers;
            Materials::ShaderConfig* currentMaterialType = nullptr;

            for (uint32 i = 0; i < numPackets; i++)
            {
                uint32 index = indexBuffer[i] & 0x00000000FFFFFFFF;

                // If new material, add a new entry into the lookup table
                auto otherMaterialType = context->renderables->nodeShaderConfigs[index];
                if (currentMaterialType != otherMaterialType)
                {
                    // Add new draw command and get reference to it
                    cmd = &context->drawList->visibilityTable.Emplace(otherMaterialType);

                    // Setup initial state for command
                    cmd->packetOffset = numDraws;
                    cmd->numDrawPackets = 0;

                    mesh = CoreGraphics::InvalidMeshId;
                    drawModifiers = NullDrawModifiers;
                    currentMaterialType = otherMaterialType;
                }
                n_assert(cmd != nullptr);

                // If a new node (resource), add a model apply command
                auto otherMesh = context->renderables->nodeMeshes[index];
                auto otherMat = context->renderables->nodeMaterials[index];
                if (mesh != otherMesh || mat != otherMat)
                {
                    ObserverContext::VisibilityModelCommand& batchCmd = cmd->models.Emplace();

                    // The offset of the command corresponds to where in the VisibilityBatchCommand batch the model should be applied
                    batchCmd.offset = cmd->packetOffset + cmd->numDrawPackets;
                    batchCmd.mesh = otherMesh;
                    batchCmd.primitiveGroup = context->renderables->nodePrimitiveGroup[index];
                    batchCmd.material = otherMat;

#if NEBULA_GRAPHICS_DEBUG
                    batchCmd.nodeName = context->renderables->nodeNames[index];
#endif
                    mesh = otherMesh;
                }

                // If a new set of draw modifiers (instance count and base instance) are used, insert a new draw command
                auto otherDrawModifiers = context->renderables->nodeDrawModifiers[index];
                if (drawModifiers != otherDrawModifiers)
                {
                    ObserverContext::VisibilityDrawCommand& drawCmd = cmd->draws.Emplace();
                    drawCmd.offset = cmd->packetOffset + cmd->numDrawPackets;
                    drawCmd.numInstances = Util::Get<0>(otherDrawModifiers);
                    drawCmd.baseInstance = Util::Get<1>(otherDrawModifiers);

                    drawModifiers = otherDrawModifiers;
                }

                // allocate memory for draw packet
                void* mem = context->allocator->Alloc(sizeof(Models::ShaderStateNode::DrawPacket));

                // update packet and add to list
                Models::ShaderStateNode::DrawPacket* packet = reinterpret_cast<Models::ShaderStateNode::DrawPacket*>(mem);
                packet->numOffsets[0] = context->renderables->nodeStates[index].resourceTableOffsets.Size();
                packet->numTables = 1;
                packet->tables[0] = context->renderables->nodeStates[index].resourceTables[CoreGraphics::GetBufferedFrameIndex()];
                packet->materialInstance = context->renderables->nodeStates[index].materialInstance;
#ifndef PUBLIC_BUILD
                packet->boundingBox = context->renderables->nodeBoundingBoxes[index];
                packet->nodeInstanceHash = index;
#endif
                memcpy(packet->offsets[0], context->renderables->nodeStates[index].resourceTableOffsets.Begin(), context->renderables->nodeStates[index].resourceTableOffsets.ByteSize());
                packet->slots[0] = NEBULA_DYNAMIC_OFFSET_GROUP;
                context->drawList->drawPackets.Append(packet);
                cmd->numDrawPackets++;
                numDraws++;
            }
        },
        nodes.Size(), jobCtx, waitCounters, &completionCounter, finishedEvent);
    }

    waitEvents.Enqueue(finishedEvent);
}

//------------------------------------------------------------------------------
/**
*/
void
ObserverContext::GenerateDrawLists(const Graphics::FrameContext& ctx)
{
    N_SCOPE(GenerateDrawLists, Visibility);
}

//------------------------------------------------------------------------------
/**
*/
void
ObserverContext::Create()
{
    __CreateContext();
#ifndef PUBLIC_BUILD
    __bundle.OnRenderDebug = ObserverContext::OnRenderDebug;
#endif 

    Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&__bundle, &__state);
    __CreateContext();
}

//------------------------------------------------------------------------------
/**
*/
void 
ObserverContext::Discard()
{
    for (int i = 0; i < ObserverContext::systems.Size(); i++)
    {
        delete ObserverContext::systems[i];
    }

    ObserverContext::systems.Clear();
    Graphics::GraphicsServer::Instance()->UnregisterGraphicsContext(&__bundle);
}

//------------------------------------------------------------------------------
/**
*/
VisibilitySystem*
ObserverContext::CreateBoxSystem(const BoxSystemLoadInfo& info)
{
    BoxSystem* system = new BoxSystem;
    system->Setup(info);
    ObserverContext::systems.Append(system);
    return system;
}

//------------------------------------------------------------------------------
/**
*/
VisibilitySystem*
ObserverContext::CreatePortalSystem(const PortalSystemLoadInfo& info)
{
    PortalSystem* system = new PortalSystem;
    system->Setup(info);
    ObserverContext::systems.Append(system);
    return system;
}

//------------------------------------------------------------------------------
/**
*/
VisibilitySystem*
ObserverContext::CreateOctreeSystem(const OctreeSystemLoadInfo& info)
{
    OctreeSystem* system = new OctreeSystem;
    system->Setup(info);
    ObserverContext::systems.Append(system);
    return system;
}

//------------------------------------------------------------------------------
/**
*/
VisibilitySystem*
ObserverContext::CreateQuadtreeSystem(const QuadtreeSystemLoadInfo & info)
{
    QuadtreeSystem* system = new QuadtreeSystem;
    system->Setup(info);
    ObserverContext::systems.Append(system);
    return system;
}

//------------------------------------------------------------------------------
/**
*/
VisibilitySystem* 
ObserverContext::CreateBruteforceSystem(const BruteforceSystemLoadInfo& info)
{
    BruteforceSystem* system = new BruteforceSystem;
    system->Setup(info);
    ObserverContext::systems.Append(system);
    return system;
}

//------------------------------------------------------------------------------
/**
*/
void
ObserverContext::WaitForVisibility(const Graphics::FrameContext& ctx)
{
    while (waitEvents.Size() > 0)
    {
        auto ev = waitEvents.Dequeue();
        ev->Wait();
        delete ev;
    }
}

#ifndef PUBLIC_BUILD
//------------------------------------------------------------------------------
/**
*/
void 
ObserverContext::OnRenderDebug(uint32_t flags)
{
    while (waitEvents.Size() > 0)
    {
        auto ev = waitEvents.Dequeue();
        ev->Wait();
        delete ev;
    }

    static int visIndex = 0;
    static int atomIndex = 0;
    static bool singleAtomMode = false;

    auto const& foo = observerAllocator.GetArray<Observer_DrawList>();
    if (singleAtomMode)
    {
        if (!foo[visIndex].drawPackets.IsEmpty())
        {
            atomIndex = Math::clamp(atomIndex, 0, foo[visIndex].drawPackets.Size() - 1);
            auto const& a = foo[visIndex].drawPackets[atomIndex];
            CoreGraphics::RenderShape shape;
            Math::vec4 color = {
#ifndef PUBLIC_BUILD
                    Util::RandomNumberTable::Rand(a->nodeInstanceHash),
                    Util::RandomNumberTable::Rand(a->nodeInstanceHash + 1),
                    Util::RandomNumberTable::Rand(a->nodeInstanceHash + 2),
#endif
                    1
            };
            shape.SetupSimpleShape(CoreGraphics::RenderShape::Box, CoreGraphics::RenderShape::RenderFlag(CoreGraphics::RenderShape::CheckDepth | CoreGraphics::RenderShape::Wireframe), color, a->boundingBox.to_mat4());
            CoreGraphics::ShapeRenderer::Instance()->AddShape(shape);
        }
    }
    else
    {
        for (auto const& a : foo[visIndex].drawPackets)
        {
            CoreGraphics::RenderShape shape;
            Math::vec4 color = {
#ifndef PUBLIC_BUILD
                Util::RandomNumberTable::Rand(a->nodeInstanceHash),
                Util::RandomNumberTable::Rand(a->nodeInstanceHash + 1),
                Util::RandomNumberTable::Rand(a->nodeInstanceHash + 2),
#endif
                1 
            };
            shape.SetupSimpleShape(CoreGraphics::RenderShape::Box, CoreGraphics::RenderShape::RenderFlag(CoreGraphics::RenderShape::CheckDepth | CoreGraphics::RenderShape::Wireframe), color, a->boundingBox.to_mat4());
            CoreGraphics::ShapeRenderer::Instance()->AddShape(shape);
        }
    }

    Util::Array<VisibilityResultArray>& vis = observerAllocator.GetArray<Observer_ResultArray>();
    Util::FixedArray<SizeT> insideCounters(vis.Size(), 0);
    Util::FixedArray<SizeT> clippedCounters(vis.Size(), 0);
    Util::FixedArray<SizeT> totalCounters(vis.Size(), 0);
    for (IndexT i = 0; i < vis.Size(); i++)
    {
        auto res = vis[i];
        for (IndexT j = 0; j < res.Size(); j++)
            switch (res[j])
            {
            case Math::ClipStatus::Inside:
                insideCounters[i]++;
                totalCounters[i]++;
                break;
            case Math::ClipStatus::Clipped:
                clippedCounters[i]++;
                totalCounters[i]++;
                break;
            default:
                break;
            }
    }
    if (ImGui::Begin("Visibility"))
    {
        ImGui::Checkbox("Single atom mode", &singleAtomMode);
        if (singleAtomMode)
        {
            ImGui::SliderInt("AtomIndex", &atomIndex, 0, (int)foo[visIndex].drawPackets.Size() - 1);
        }
        ImGui::SliderInt("visIndex", &visIndex, 0, (int)foo.size() - 1);
        for (IndexT i = 0; i < vis.Size(); i++)
        {
            ImGui::Text("Entities visible for observer %d: %d (inside [%d], clipped [%d])", i, totalCounters[i], insideCounters[i], clippedCounters[i]);
        }
    }   
    ImGui::End();
}
#endif

//------------------------------------------------------------------------------
/**
*/
const ObserverContext::VisibilityDrawList*
ObserverContext::GetVisibilityDrawList(const Graphics::GraphicsEntityId id)
{
    const Graphics::ContextEntityId cid = ObserverContext::GetContextId(id);
    if (cid == Graphics::InvalidContextEntityId)
        return nullptr;
    else 
        return &observerAllocator.Get<Observer_DrawList>(cid.id);
}

//------------------------------------------------------------------------------
/**
*/
Graphics::ContextEntityId
ObserverContext::Alloc()
{
    return observerAllocator.Alloc();
}

//------------------------------------------------------------------------------
/**
*/
void
ObserverContext::Dealloc(Graphics::ContextEntityId id)
{
    VisibilityDrawList& draws = observerAllocator.Get<Observer_DrawList>(id.id);
    draws.visibilityTable.Clear();
    draws.drawPackets.Clear();
    observerAllocator.Dealloc(id.id);
}

__ImplementContext(ObservableContext, ObservableContext::observableAllocator);

//------------------------------------------------------------------------------
/**
*/
void
ObservableContext::Setup(const Graphics::GraphicsEntityId id, VisibilityEntityType entityType)
{
    const Graphics::ContextEntityId cid = ObservableContext::GetContextId(id);
    observableAllocator.Set<Observable_EntityId>(cid.id, id);
    
    if (entityType == Model || entityType == Particle)
    {
        // Get node instance ranges
        const Models::NodeInstanceRange& nodeInstanceRange = Models::ModelContext::GetModelRenderableRange(id);

        // Create new node range to remember where in the visibility context this model belongs
        SizeT numNodes = nodeInstanceRange.end - nodeInstanceRange.begin;
        observableAllocator.Set<Observable_NumNodes>(cid.id, numNodes);

        // All we need is to have as many clip statuses as we have nodes,
        // the ids and how they relate to a visibility value is resolved at runtime
        const Util::Array<ObserverContext::VisibilityResultArray>& visAllocators = ObserverContext::observerAllocator.GetArray<Observer_ResultArray>();
        for (IndexT i = 0; i < visAllocators.Size(); i++)
        {
            ObserverContext::VisibilityResultArray& alloc = visAllocators[i];
            alloc.Reserve(numNodes);
            for (IndexT j = 0; j < numNodes; j++)
                alloc.Append(Math::ClipStatus::Outside);
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
ObservableContext::Create()
{
    __CreateContext();
    Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&ObservableContext::__bundle, &ObservableContext::__state);
}

//------------------------------------------------------------------------------
/**
*/
Graphics::ContextEntityId 
ObservableContext::Alloc()
{
    Ids::Id32 id = observableAllocator.Alloc();
    observableAllocator.Set<Observable_EntityId>(id, Graphics::GraphicsEntityId::Invalid());
    return id;
}

//------------------------------------------------------------------------------
/**
*/
void 
ObservableContext::Dealloc(Graphics::ContextEntityId id)
{
    uint32 numNodes = observableAllocator.Get<Observable_NumNodes>(id.id);

    // add as many atoms to each visibility result allocator
    const Util::Array<ObserverContext::VisibilityResultArray>& visAllocators = ObserverContext::observerAllocator.GetArray<Observer_ResultArray>();
    for (IndexT i = 0; i < visAllocators.Size(); i++)
    {
        ObserverContext::VisibilityResultArray& alloc = visAllocators[i];
        alloc.EraseRange(0, numNodes);
    }
    observableAllocator.Dealloc(id.id);
}

} // namespace Visibility
