//------------------------------------------------------------------------------
//  visobservercontext.cc
//  (C) 2018-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "visibilitycontext.h"
#include "graphics/graphicsserver.h"

#include "graphics/cameracontext.h"
#include "lighting/lightcontext.h"
#include "lighting/lightprobecontext.h"
#include "models/modelcontext.h"
#include "particles/particlecontext.h"

#include "systems/boxsystem.h"
#include "systems/octreesystem.h"
#include "systems/portalsystem.h"
#include "systems/quadtreesystem.h"
#include "systems/bruteforcesystem.h"

#include "system/cpu.h"
#include "profiling/profiling.h"

#include "dynui/im3d/im3d.h"
#include "dynui/im3d/im3dcontext.h"
#include "util/randomnumbertable.h"

#ifndef PUBLIC_BUILD
#include "imgui.h"
#endif

namespace Visibility
{

ObserverContext::ObserverAllocator ObserverContext::observerAllocator;
ObservableContext::ObservableAllocator ObservableContext::observableAllocator;
ObservableContext::ObservableAtomAllocator ObservableContext::observableAtomAllocator;


Util::Array<VisibilitySystem*> ObserverContext::systems;

Jobs::JobPortId ObserverContext::jobPort;
Jobs::JobSyncId ObserverContext::jobInternalSync;
Jobs::JobSyncId ObserverContext::jobInternalSync2;
Jobs::JobSyncId ObserverContext::jobInternalSync3;
Jobs::JobSyncId ObserverContext::jobHostSync;
Util::Queue<Jobs::JobId> ObserverContext::runningJobs;

extern void VisibilitySortJob(const Jobs::JobFuncContext& ctx);
extern void VisibilityDependencyJob(const Jobs::JobFuncContext& ctx);
extern void VisibilityDrawListUpdateJob(const Jobs::JobFuncContext& ctx);

__ImplementContext(ObserverContext, ObserverContext::observerAllocator);

//------------------------------------------------------------------------------
/**
*/
void
ObserverContext::Setup(const Graphics::GraphicsEntityId id, VisibilityEntityType entityType)
{
    const Graphics::ContextEntityId cid = GetContextId(id);
    observerAllocator.Get<Observer_EntityType>(cid.id) = entityType;
    observerAllocator.Get<Observer_EntityId>(cid.id) = id;
}

//------------------------------------------------------------------------------
/**
*/
void 
ObserverContext::MakeDependency(const Graphics::GraphicsEntityId a, const Graphics::GraphicsEntityId b, const DependencyMode mode)
{
    const Graphics::ContextEntityId cid = GetContextId(b);
    observerAllocator.Get<Observer_Dependency>(cid.id) = a;
    observerAllocator.Get<Observer_DependencyMode>(cid.id) = mode;
}

//------------------------------------------------------------------------------
/**
*/
void 
ObserverContext::RunVisibilityTests(const Graphics::FrameContext& ctx)
{
    N_SCOPE(RunVisibilityTests, Visibility);

    Util::Array<Math::mat4>& observableAtomTransforms = ObservableContext::observableAtomAllocator.GetArray<ObservableAtom_Transform>();
    Util::Array<VisibilityEntityType>& observableAtomTypes = ObservableContext::observableAtomAllocator.GetArray<ObservableAtom_VisibilityEntityType>();
    Util::Array<Graphics::GraphicsEntityId>& observableAtomEntities = ObservableContext::observableAtomAllocator.GetArray<ObservableAtom_GraphicsEntityId>();
    Util::Array<bool>& observableAtomActiveFlags = ObservableContext::observableAtomAllocator.GetArray<ObservableAtom_Active>();

    // go through all transforms and update
    IndexT i;
    for (i = 0; i < observableAtomEntities.Size(); i++)
    {
        const VisibilityEntityType type = observableAtomTypes[i];
        const Graphics::GraphicsEntityId id = observableAtomEntities[i];

        if (id == Graphics::GraphicsEntityId::Invalid())
            continue;

        switch (type)
        {
        case Model:
        {
            Models::ShaderStateNode::Instance* sinst = reinterpret_cast<Models::ShaderStateNode::Instance*>(ObservableContext::observableAtomAllocator.Get<ObservableAtom_Node>(i));
            observableAtomActiveFlags[i] = sinst->active;
            observableAtomTransforms[i] = sinst->boundingBox.to_mat4();
            break;
        }
        case Particle:
            observableAtomTransforms[i] = Particles::ParticleContext::GetBoundingBox(id).to_mat4();
            observableAtomActiveFlags[i] = true;
            break;
        case Light:
            observableAtomTransforms[i] = Lighting::LightContext::GetTransform(id);
            break;
        case LightProbe:
            observableAtomTransforms[i] = Graphics::LightProbeContext::GetTransform(id);
            break;
        }
    }

    Util::Array<Math::mat4>& observerTransforms = observerAllocator.GetArray<Observer_Matrix>();
    const Util::Array<Graphics::GraphicsEntityId>& observerIds = observerAllocator.GetArray<Observer_EntityId>();
    const Util::Array<VisibilityEntityType>& observerTypes = observerAllocator.GetArray<Observer_EntityType>();
    Util::Array<VisibilityResultArray>& vis = observerAllocator.GetArray<Observer_ResultArray>();
    Util::Array<Math::ClipStatus::Type*>& observerResults = observerAllocator.GetArray<Observer_Results>();

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
        }
    }

    // reset all lists to that all entities are visible
    for (i = 0; i < vis.Size(); i++)
    {
        Util::Array<Math::ClipStatus::Type>& flags = vis[i];
        observerResults[i] = flags.Begin();

        VisibilityDrawList& visibilities = observerAllocator.Get<Observer_DrawList>(i);
        visibilities.visibilityTable.Clear();
        visibilities.drawPackets.Clear();
    }

    // prepare visibility systems
    if (observerTransforms.Size() > 0)
    {
        for (i = 0; i < ObserverContext::systems.Size(); i++)
        {
            VisibilitySystem* sys = ObserverContext::systems[i];
            sys->PrepareObservers(observerTransforms.Begin(), observerResults.Begin(), observerTransforms.Size());
        }
    }

    // setup observerable entities
    const Util::Array<Graphics::GraphicsEntityId>& ids = ObservableContext::observableAllocator.GetArray<Observable_EntityId>();
    if (observableAtomTransforms.Size() > 0)
    {
        for (i = 0; i < ObserverContext::systems.Size(); i++)
        {
            VisibilitySystem* sys = ObserverContext::systems[i];
            sys->PrepareEntities(observableAtomTransforms.Begin(), ids.Begin(), observableAtomActiveFlags.Begin(), observableAtomTransforms.Size());
        }
    }

    // run all visibility systems
    if ((observerTransforms.Size() > 0) && (observableAtomTransforms.Size() > 0))
    {
        for (i = 0; i < ObserverContext::systems.Size(); i++)
        {
            VisibilitySystem* sys = ObserverContext::systems[i];
            sys->Run();
        }
    }

    // put a sync point for the jobs so all results are done when doing the sorting
    Jobs::JobSyncSignal(ObserverContext::jobInternalSync, ObserverContext::jobPort);
    Jobs::JobSyncThreadWait(ObserverContext::jobInternalSync, ObserverContext::jobPort);

    // handle dependencies
    bool dependencyNeeded = false;
    for (i = 0; i < vis.Size(); i++)
    {
        const Util::Array<Math::ClipStatus::Type>& flags = vis[i];
        Graphics::GraphicsEntityId& dependency = observerAllocator.Get<Observer_Dependency>(i);

        // run dependency resolve job
        if (dependency != Graphics::GraphicsEntityId::Invalid())
        {
            Jobs::JobContext ctx;
            ctx.uniform.scratchSize = 0;
            ctx.uniform.numBuffers = 2;
            ctx.input.numBuffers = 1;
            ctx.output.numBuffers = 1;

            const Graphics::ContextEntityId& ctxId = GetContextIdRef(dependency);

            ctx.uniform.data[0] = &observerAllocator.Get<Observer_DependencyMode>(i);
            ctx.uniform.dataSize[0] = sizeof(DependencyMode);
            ctx.uniform.data[1] = &ctxId;
            ctx.uniform.dataSize[1] = sizeof(uint32);

            const Util::Array<Math::ClipStatus::Type>& otherFlags = vis[ctxId.id];

            ctx.input.data[0] = otherFlags.Begin();
            ctx.input.dataSize[0] = sizeof(Math::ClipStatus::Type) * otherFlags.Size();
            ctx.input.sliceSize[0] = sizeof(Math::ClipStatus::Type) * otherFlags.Size();

            ctx.output.data[0] = flags.Begin();
            ctx.output.dataSize[0] = sizeof(Math::ClipStatus::Type) * flags.Size();
            ctx.output.sliceSize[0] = sizeof(Math::ClipStatus::Type) * flags.Size();

            // schedule job
            Jobs::JobId job = Jobs::CreateJob({ VisibilityDependencyJob });
            Jobs::JobSchedule(job, ObserverContext::jobPort, ctx, false);

            // add to delete list
            ObserverContext::runningJobs.Enqueue(job);
            dependencyNeeded = true;
        }
    }

    // again, put sync if we needed to resolve dependency
    if (dependencyNeeded)
    {
        Jobs::JobSyncSignal(ObserverContext::jobInternalSync2, ObserverContext::jobPort);
        Jobs::JobSyncThreadWait(ObserverContext::jobInternalSync2, ObserverContext::jobPort);
    }

    for (i = 0; i < vis.Size(); i++)
    {
        const Util::Array<Models::ModelNode::Instance*>& nodes = ObservableContext::observableAtomAllocator.GetArray<ObservableAtom_Node>();

        // early abort empty visibility queries
        if (nodes.Size() == 0)
        {
            continue;
        }

        const Util::Array<Math::ClipStatus::Type>& flags = vis[i];
        VisibilityDrawList& visibilities = observerAllocator.Get<Observer_DrawList>(i);
        Memory::ArenaAllocator<1024>& allocator = observerAllocator.Get<Observer_DrawListAllocator>(i);

        // then execute sort job, which only runs the function once
        Jobs::JobContext ctx;
        ctx.uniform.scratchSize = 0;
        ctx.uniform.numBuffers = 1;
        ctx.input.numBuffers = 2;
        ctx.output.numBuffers = 1;

        ctx.input.data[0] = flags.Begin();
        ctx.input.dataSize[0] = sizeof(Math::ClipStatus::Type) * flags.Size();
        ctx.input.sliceSize[0] = sizeof(Math::ClipStatus::Type) * flags.Size();

        ctx.input.data[1] = nodes.Begin();
        ctx.input.dataSize[1] = sizeof(Models::ModelNode::Instance*) * nodes.Size();
        ctx.input.sliceSize[1] = sizeof(Models::ModelNode::Instance*) * nodes.Size();

        ctx.output.data[0] = &visibilities;
        ctx.output.dataSize[0] = sizeof(VisibilityDrawList);
        ctx.output.sliceSize[0] = sizeof(VisibilityDrawList);

        ctx.uniform.data[0] = &allocator;
        ctx.uniform.dataSize[0] = sizeof(allocator);

        // schedule job
        Jobs::JobId job = Jobs::CreateJob({ VisibilitySortJob });
        Jobs::JobSchedule(job, ObserverContext::jobPort, ctx);

        // add to delete list
        ObserverContext::runningJobs.Enqueue(job);
    }

    // insert sync after all visibility systems are done
    Jobs::JobSyncSignal(ObserverContext::jobInternalSync3, ObserverContext::jobPort);
}

//------------------------------------------------------------------------------
/**
*/
void
ObserverContext::GenerateDrawLists(const Graphics::FrameContext& ctx)
{
    N_SCOPE(GenerateDrawLists, Visibility);

    Jobs::JobSyncThreadWait(ObserverContext::jobInternalSync3, ObserverContext::jobPort);
    IndexT i;
    Util::Array<VisibilityResultArray>& vis = observerAllocator.GetArray<Observer_ResultArray>();
    for (i = 0; i < vis.Size(); i++)
    {
        VisibilityDrawList& visibilities = observerAllocator.Get<Observer_DrawList>(i);

        Jobs::JobContext ctx;
        ctx.uniform.scratchSize = 0;
        ctx.uniform.numBuffers = 0;
        ctx.input.numBuffers = 1;
        ctx.output.numBuffers = 1;

        ctx.input.data[0] = &visibilities;
        ctx.input.dataSize[0] = sizeof(VisibilityDrawList);
        ctx.input.sliceSize[0] = sizeof(VisibilityDrawList);

        ctx.output.data[0] = &visibilities;
        ctx.output.dataSize[0] = sizeof(VisibilityDrawList);
        ctx.output.sliceSize[0] = sizeof(VisibilityDrawList);

        // schedule job
        Jobs::JobId job = Jobs::CreateJob({ VisibilityDrawListUpdateJob });
        Jobs::JobSchedule(job, ObserverContext::jobPort, ctx, false);

        // add to delete list
        ObserverContext::runningJobs.Enqueue(job);
    }

    // insert sync after all visibility systems are done
    Jobs::JobSyncSignal(ObserverContext::jobHostSync, ObserverContext::jobPort);
}

//------------------------------------------------------------------------------
/**
*/
void
ObserverContext::Create()
{
    __CreateContext();
    
    __bundle.OnBegin = ObserverContext::RunVisibilityTests;
    __bundle.OnBeforeFrame = ObserverContext::GenerateDrawLists;
    __bundle.OnWaitForWork = ObserverContext::WaitForVisibility;
    __bundle.StageBits = &ObservableContext::__state.currentStage;
#ifndef PUBLIC_BUILD
    __bundle.OnRenderDebug = ObserverContext::OnRenderDebug;
#endif 

    ObserverContext::__state.allowedRemoveStages = Graphics::OnBeginStage;
    Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&__bundle, &__state);
    ObserverContext::jobPort = Graphics::GraphicsServer::renderSystemsJobPort;

    Jobs::CreateJobSyncInfo sinfo =
    {
        nullptr
    };
    ObserverContext::jobInternalSync = Jobs::CreateJobSync(sinfo);
    ObserverContext::jobInternalSync2 = Jobs::CreateJobSync(sinfo);
    ObserverContext::jobInternalSync3 = Jobs::CreateJobSync(sinfo);
    ObserverContext::jobHostSync = Jobs::CreateJobSync(sinfo);

    __CreateContext();
}

//------------------------------------------------------------------------------
/**
*/
void 
ObserverContext::Discard()
{
    Jobs::DestroyJobPort(ObserverContext::jobPort);
    Jobs::DestroyJobSync(ObserverContext::jobInternalSync);
    Jobs::DestroyJobSync(ObserverContext::jobHostSync);

    for (int i = 0; i < ObserverContext::systems.Size(); i++)
    {
        n_delete(ObserverContext::systems[i]);
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
    BoxSystem* system = n_new(BoxSystem);
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
    PortalSystem* system = n_new(PortalSystem);
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
    OctreeSystem* system = n_new(OctreeSystem);
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
    QuadtreeSystem* system = n_new(QuadtreeSystem);
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
    BruteforceSystem* system = n_new(BruteforceSystem);
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
    if (ObserverContext::runningJobs.Size() > 0)
    {
        // wait for all jobs to finish
        Jobs::JobSyncHostWait(ObserverContext::jobHostSync);

        // destroy jobs
        while (!ObserverContext::runningJobs.IsEmpty())
            Jobs::DestroyJob(ObserverContext::runningJobs.Dequeue());
    }
}

#ifndef PUBLIC_BUILD
//------------------------------------------------------------------------------
/**
*/
void 
ObserverContext::OnRenderDebug(uint32_t flags)
{
    // wait for all jobs to finish
    Jobs::JobSyncHostWait(ObserverContext::jobHostSync);

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
            Models::ShaderStateNode::Instance* sinst = reinterpret_cast<Models::ShaderStateNode::Instance*>(a->node);
            Math::vec4 color = {
                    Util::RandomNumberTable::Rand(a->node->node->HashCode()),
                    Util::RandomNumberTable::Rand(a->node->node->HashCode() + 1),
                    Util::RandomNumberTable::Rand(a->node->node->HashCode() + 2),
                    1
            };
            shape.SetupSimpleShape(CoreGraphics::RenderShape::Box, CoreGraphics::RenderShape::RenderFlag(CoreGraphics::RenderShape::CheckDepth | CoreGraphics::RenderShape::Wireframe), sinst->boundingBox.to_mat4(), color);
            CoreGraphics::ShapeRenderer::Instance()->AddShape(shape);
        }
    }
    else
    {
        for (auto const& a : foo[visIndex].drawPackets)
        {
            CoreGraphics::RenderShape shape;
            Models::ShaderStateNode::Instance* sinst = reinterpret_cast<Models::ShaderStateNode::Instance*>(a->node);
            Math::vec4 color = { 
                Util::RandomNumberTable::Rand(a->node->node->HashCode()),
                Util::RandomNumberTable::Rand(a->node->node->HashCode() + 1),
                Util::RandomNumberTable::Rand(a->node->node->HashCode() + 2),
                1 
            };
            Math::mat4 t = sinst->boundingBox.to_mat4();
            shape.SetupSimpleShape(CoreGraphics::RenderShape::Box, CoreGraphics::RenderShape::RenderFlag(CoreGraphics::RenderShape::CheckDepth | CoreGraphics::RenderShape::Wireframe), sinst->boundingBox.to_mat4(), color);
            CoreGraphics::ShapeRenderer::Instance()->AddShape(shape);
        }
    }

    Util::Array<VisibilityResultArray>& vis = observerAllocator.GetArray<3>();
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
    observableAllocator.Get<Observable_EntityId>(cid.id) = id;
    
    if (entityType == Model || entityType == Particle)
    {
        // get nodes
        const Util::Array<Models::ModelNode::Instance*>& nodes = Models::ModelContext::GetModelNodeInstances(id);

        SizeT numAtoms = 0;
        // produce as many atoms as we have nodes
        for (IndexT j = 0; j < nodes.Size(); j++)
        {
            Models::ModelNode::Instance* node = nodes[j];
            if ((node->node->GetBits() & Models::HasStateBit) == Models::HasStateBit)
            {
                Ids::Id32 obj = ObservableContext::observableAtomAllocator.Alloc();

                ObservableContext::observableAtomAllocator.Get<ObservableAtom_GraphicsEntityId>(obj) = id;
                ObservableContext::observableAtomAllocator.Get<ObservableAtom_Node>(obj) = nodes[j];
                ObservableContext::observableAtomAllocator.Get<ObservableAtom_VisibilityEntityType>(obj) = entityType;

                // append id to observable so we can track it
                observableAllocator.Get<Observable_Atoms>(cid.id).Append(obj);
                numAtoms++;
            }
        }

        const Util::Array<ObserverContext::VisibilityResultArray>& visAllocators = ObserverContext::observerAllocator.GetArray<Observer_ResultArray>();
        // add as many atoms to each visibility result allocator
        for (IndexT i = 0; i < visAllocators.Size(); i++)
        {
            ObserverContext::VisibilityResultArray& alloc = visAllocators[i];
            for (IndexT j = 0; j < numAtoms; j++)
            {
                alloc.Append(Math::ClipStatus::Inside);
            }
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
    ObservableContext::__state.allowedRemoveStages = Graphics::OnBeforeFrameStage;
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
    Graphics::GraphicsEntityId const eid = __state.entities[id.id];

    // find atoms and dealloc
    Util::ArrayStack<Ids::Id32, 1>& atoms = observableAllocator.Get<Observable_Atoms>(id.id);
    const Util::Array<ObserverContext::VisibilityResultArray>& visAllocators = ObserverContext::observerAllocator.GetArray<Observer_ResultArray>();

    // cleanup visibility allocator first
    // just pop the last n number of atoms, since the order doesn't matter.
    for (IndexT i = 0; i < visAllocators.Size(); i++)
    {
        ObserverContext::VisibilityResultArray& alloc = visAllocators[i];
        for (IndexT j = 0; j < atoms.Size(); j++)
        {
            alloc.EraseIndex(alloc.Size() - 1);
        }
    }

    for (IndexT i = 0; i < atoms.Size(); i++)
    {
        observableAtomAllocator.Dealloc(atoms[i]);
    }

    // Pop'n'swap all invalid atoms
    auto& freeIds = observableAtomAllocator.FreeIds();
    uint32_t index;
    uint32_t oldIndex;
    Graphics::GraphicsEntityId lastId;
    uint32_t allocatorSize;
    SizeT size = freeIds.Size();
    for (SizeT i = size - 1; i >= 0; --i)
    {
        index = freeIds.Back();
        freeIds.EraseBack();
        allocatorSize = (uint32_t)observableAtomAllocator.Size();
        if (index >= allocatorSize)
        {
            continue;
        }

        oldIndex = allocatorSize - 1;
        lastId = observableAtomAllocator.Get<ObservableAtom_GraphicsEntityId>(oldIndex);
        
        if (lastId == eid)
        {
            // the ID that we're erase-swapping with belongs to this entity as well, which means it exists in the freeIds list.
            // Add the index to the freeIds list again so that we don't miss it
            freeIds.Append(index);
            i++;
        }

        // Update the index for the atom in the atoms list
        Graphics::ContextEntityId lastCid = GetContextId(lastId);
        ObservableAtoms& lastCidAtoms = observableAllocator.Get<Observable_Atoms>(lastCid.id);
        IndexT atomIndex = lastCidAtoms.FindIndex(oldIndex);
        lastCidAtoms[atomIndex] = index;

        observableAtomAllocator.EraseIndexSwap(index);
    }
    freeIds.Clear();

    observableAllocator.Dealloc(id.id);
}

} // namespace Visibility
