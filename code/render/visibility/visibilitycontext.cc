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

#include "systems/boxsystem.h"
#include "systems/octreesystem.h"
#include "systems/portalsystem.h"
#include "systems/quadtreesystem.h"
#include "systems/bruteforcesystem.h"

#include "system/cpu.h"

#ifndef PUBLIC_BUILD
#include "imgui.h"
#endif

namespace Visibility
{

ObserverContext::ObserverAllocator ObserverContext::observerAllocator;
ObservableContext::ObserveeAllocator ObservableContext::observeeAllocator;

Util::Array<VisibilitySystem*> ObserverContext::systems;

Jobs::JobPortId ObserverContext::jobPort;
Jobs::JobSyncId ObserverContext::jobInternalSync;
Jobs::JobSyncId ObserverContext::jobHostSync;
Util::Queue<Jobs::JobId> ObserverContext::runningJobs;

extern void VisibilitySortJob(const Jobs::JobFuncContext& ctx);

_ImplementContext(ObserverContext, ObserverContext::observerAllocator);

//------------------------------------------------------------------------------
/**
*/
void
ObserverContext::Setup(const Graphics::GraphicsEntityId id, VisibilityEntityType entityType)
{
	const Graphics::ContextEntityId cid = GetContextId(id);
	observerAllocator.Get<ObserverEntityType>(cid.id) = entityType;
	observerAllocator.Get<ObserverEntityId>(cid.id) = id;

	// go through observerable objects and allocate a slot for the object, and set it to the default visible state
	const Util::Array<Graphics::GraphicsEntityId>& ids = ObservableContext::observeeAllocator.GetArray<1>();
	for (IndexT i = 0; i < ids.Size(); i++)
	{
		Ids::Id32 res = observerAllocator.Get<ObserverResultAllocator>(cid.id).Alloc();
		observerAllocator.Get<ObserverResultAllocator>(cid.id).Get<VisibilityResultFlag>(res) = true;

		if (entityType == Model)
		{
			const Util::Array<Models::ModelNode::Instance*>& nodes = Models::ModelContext::GetModelNodeInstances(id);
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
ObserverContext::OnBeforeFrame(const Graphics::FrameContext& ctx)
{
	const Util::Array<VisibilityEntityType>& observerTypes = observerAllocator.GetArray<ObserverEntityType>();
	const Util::Array<VisibilityEntityType>& observeeTypes = ObservableContext::observeeAllocator.GetArray<ObservableEntityType>();

	const Util::Array<Graphics::GraphicsEntityId>& observerIds = observerAllocator.GetArray<ObserverEntityId>();
	const Util::Array<Graphics::GraphicsEntityId>& observeeIds = ObservableContext::observeeAllocator.GetArray<ObservableEntityId>();

	Util::Array<Math::matrix44>& observerTransforms = observerAllocator.GetArray<ObserverMatrix>();
	Util::Array<Math::matrix44>& observeeTransforms = ObservableContext::observeeAllocator.GetArray<ObservableTransform>();

	const Util::Array<VisibilityResultAllocator>& results = observerAllocator.GetArray<ObserverResultAllocator>();
	Util::Array<bool*> observerResults = observerAllocator.GetArray<ObserverResults>();

	IndexT i;
	for (i = 0; i < observeeIds.Size(); i++)
	{
		const Graphics::GraphicsEntityId id = observeeIds[i];
		const VisibilityEntityType type = observeeTypes[i];

		switch (type)
		{
		case Model:
			observeeTransforms[i] = Models::ModelContext::GetBoundingBox(id).to_matrix44();
			break;
		case Light:
			observeeTransforms[i] = Lighting::LightContext::GetTransform(id);
			break;
		case LightProbe:
			observeeTransforms[i] = Graphics::LightProbeContext::GetTransform(id);
			break;
		}
	}

	for (i = 0; i < observerIds.Size(); i++)
	{
		const Graphics::GraphicsEntityId id = observerIds[i];
		const VisibilityEntityType type = observerTypes[i];

		VisibilityResultAllocator& result = results[i];

		// fetch current context ids
		IndexT j;
		for (j = 0; j < observeeIds.Size(); j++)
		{
			const VisibilityEntityType type = observeeTypes[j];
			Util::Array<Graphics::ContextEntityId>& contextIds = result.GetArray<VisibilityResultCtxId>();

			switch (type)
			{
			case Model:
				contextIds[j] = Models::ModelContext::GetContextId(observeeIds[j]);
				break;
			case Light:
				contextIds[j] = Lighting::LightContext::GetContextId(observeeIds[j]);
				break;
			case LightProbe:
				contextIds[j] = Graphics::LightProbeContext::GetContextId(observeeIds[j]);
				break;
			}
		}

		switch (type)
		{
		case Camera:
			observerTransforms[i] = Graphics::CameraContext::GetViewProjection(id);
			break;
		case Light:
			observerTransforms[i] = Lighting::LightContext::GetViewProjTransform(id);
			break;
		case LightProbe:
			observerTransforms[i] = Graphics::LightProbeContext::GetTransform(id);
			break;
		}
	}

	// first step, go through list of visible entities and reset
	Util::Array<VisibilityResultAllocator>& vis = observerAllocator.GetArray<ObserverResultAllocator>();
	Util::Array<VisibilityDrawList>& draws = observerAllocator.GetArray<ObserverDrawList>();

	// reset all lists to that all entities are visible
	for (i = 0; i < vis.Size(); i++)
	{
		VisibilityResultAllocator& list = vis[i];
		Util::Array<bool>& flags = list.GetArray<VisibilityResultFlag>();
		observerResults[i] = flags.Begin();

		for (IndexT j = 0; j < flags.Size(); j++)
		{
			flags[j] = true;
		}

		// clear draw lists
		VisibilityDrawList& draw = draws[i];
		auto it1 = draw.Begin();
		while (it1 != draw.End())
		{
			auto it2 = it1.val->Begin();
			while (it2 != it1.val->End())
			{
				it2.val->Reset();
				it2++;
			}
			it1.val->Reset();
			it1++;
		}
	}

	// prepare visibility systems
	if (observerTransforms.Size() > 0) for (i = 0; i < ObserverContext::systems.Size(); i++)
	{
		VisibilitySystem* sys = ObserverContext::systems[i];
		sys->PrepareObservers(observerTransforms.Begin(), observerResults.Begin(), observerTransforms.Size());
	}

	// setup observerable entities
	const Util::Array<Graphics::GraphicsEntityId>& ids = ObservableContext::observeeAllocator.GetArray<ObservableEntityId>();
	if (observeeTransforms.Size() > 0) for (i = 0; i < ObserverContext::systems.Size(); i++)
	{
		VisibilitySystem* sys = ObserverContext::systems[i];
		sys->PrepareEntities(observeeTransforms.Begin(), ids.Begin(), observeeTransforms.Size());
	}

	// run all visibility systems
	IndexT j;
	if ((observerTransforms.Size() > 0) && (observeeTransforms.Size() > 0))
		for (j = 0; j < ObserverContext::systems.Size(); j++)
		{
			VisibilitySystem* sys = ObserverContext::systems[j];
			sys->Run();
		}

	// put a sync point for the jobs
	Jobs::JobSyncSignal(ObserverContext::jobInternalSync, ObserverContext::jobPort);
	Jobs::JobSyncThreadWait(ObserverContext::jobInternalSync, ObserverContext::jobPort);

	for (i = 0; i < vis.Size(); i++)
	{
		const Util::Array<bool>& flags = vis[i].GetArray<VisibilityResultFlag>();
		const Util::Array<Graphics::ContextEntityId>& entities = vis[i].GetArray<VisibilityResultCtxId>();
		VisibilityDrawList& visibilities = observerAllocator.Get<ObserverDrawList>(i);
		Memory::ArenaAllocator<1024>& allocator = observerAllocator.Get<ObserverDrawListAllocator>(i);
		allocator.Release();

        if (entities.Size() == 0)
        {
            continue;
        }

		// then execute sort job, which only runs the function once
		Jobs::JobContext ctx;
		ctx.uniform.scratchSize = 0;
		ctx.uniform.numBuffers = 1;
		ctx.input.numBuffers = 2;
		ctx.output.numBuffers = 1;

		ctx.input.data[0] = flags.Begin();
		ctx.input.dataSize[0] = sizeof(bool) * flags.Size();
		ctx.input.sliceSize[0] = sizeof(bool) * flags.Size();
		
		ctx.input.data[1] = entities.Begin();
		ctx.input.dataSize[1] = sizeof(Graphics::ContextEntityId) * entities.Size();
		ctx.input.sliceSize[1] = sizeof(Graphics::ContextEntityId) * entities.Size();

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
	Jobs::JobSyncSignal(ObserverContext::jobHostSync, ObserverContext::jobPort);
}

//------------------------------------------------------------------------------
/**
*/
void
ObserverContext::Create()
{
	_CreateContext();
    
	__bundle.OnBeforeFrame = ObserverContext::OnBeforeFrame;
	__bundle.OnWaitForWork = ObserverContext::WaitForVisibility;
	__bundle.StageBits = &ObservableContext::__state.currentStage;
#ifndef PUBLIC_BUILD
	__bundle.OnRenderDebug = ObserverContext::OnRenderDebug;
#endif 

	ObserverContext::__state.allowedRemoveStages = Graphics::OnBeforeFrameStage;
	Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&__bundle, &__state);

	Jobs::CreateJobPortInfo info =
	{
		"VisibilityJobPort",
		4,
		System::Cpu::Core1 | System::Cpu::Core2 | System::Cpu::Core3 | System::Cpu::Core4,
		UINT_MAX
	};
	ObserverContext::jobPort = Jobs::CreateJobPort(info);

	Jobs::CreateJobSyncInfo sinfo =
	{
		nullptr
	};
	ObserverContext::jobInternalSync = Jobs::CreateJobSync(sinfo);
	ObserverContext::jobHostSync = Jobs::CreateJobSync(sinfo);

	_CreateContext();
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

	Util::Array<VisibilityResultAllocator>& vis = observerAllocator.GetArray<3>();
	Util::FixedArray<SizeT> visCounters(vis.Size(), 0);
	for (IndexT i = 0; i < vis.Size(); i++)
	{
		auto res = vis[i].GetArray<0>();
		for (IndexT j = 0; j < res.Size(); j++)
			if (res[j])
				visCounters[i]++;
	}
	ImGui::Begin("Visibility", nullptr, 0);
	ImGui::SetWindowSize(ImVec2(240, 100));
	for (IndexT i = 0; i < vis.Size(); i++)
	{
		ImGui::Text("Entities visible for observer %d: [%d]", i, visCounters[i]);
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
	if (cid == Graphics::ContextEntityId::Invalid()) return nullptr;
	else return &observerAllocator.Get<ObserverDrawList>(cid.id);
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
	Util::Array<VisibilityDrawList>& draws = observerAllocator.GetArray<ObserverDrawList>();

	// reset all lists to that all entities are visible
	IndexT i;
	for (i = 0; i < draws.Size(); i++)
	{
		// clear draw lists
		VisibilityDrawList& draw = draws[i];
		auto it1 = draw.Begin();
		while (it1 != draw.End())
		{
			auto it2 = it1.val->Begin();
			while (it2 != it1.val->End())
			{
				it2.val->Clear();
				it2++;
			}
			it1.val->Clear();
			it1++;
		}
	}
	observerAllocator.Dealloc(id.id);
}

_ImplementContext(ObservableContext, ObservableContext::observeeAllocator);

//------------------------------------------------------------------------------
/**
*/
void
ObservableContext::Setup(const Graphics::GraphicsEntityId id, VisibilityEntityType entityType)
{
	const Graphics::ContextEntityId cid = ObservableContext::GetContextId(id);
	observeeAllocator.Get<ObservableEntityId>(cid.id) = id;
	observeeAllocator.Get<ObservableEntityType>(cid.id) = entityType;

	// go through observers and allocate visibility slot for this object
	const Util::Array<ObserverContext::VisibilityResultAllocator>& visAllocators = ObserverContext::observerAllocator.GetArray<ObserverResultAllocator>();
	Graphics::ContextEntityId cid2;
	switch (entityType)
	{
	case Model:
		cid2 = Models::ModelContext::GetContextId(id);
		break;
	case Light:
		cid2 = Lighting::LightContext::GetContextId(id);
		break;
	case LightProbe:
		cid2 = Graphics::LightProbeContext::GetContextId(id);
		break;
	}

	for (IndexT i = 0; i < visAllocators.Size(); i++)
	{
		ObserverContext::VisibilityResultAllocator& alloc = visAllocators[i];
		Ids::Id32 obj = alloc.Alloc();
		alloc.Get<VisibilityResultFlag>(obj) = true;
		alloc.Get<VisibilityResultCtxId>(obj) = cid2; // get context Id since model can be loaded later...
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
ObservableContext::Create()
{
	_CreateContext();
    ObservableContext::__state.OnInstanceMoved = OnInstanceMoved;
	ObservableContext::__state.allowedRemoveStages = Graphics::OnBeforeFrameStage;
    Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&ObservableContext::__bundle, &ObservableContext::__state);
}

//------------------------------------------------------------------------------
/**
*/
Graphics::ContextEntityId 
ObservableContext::Alloc()
{
	return observeeAllocator.Alloc();
}

//------------------------------------------------------------------------------
/**
*/
void 
ObservableContext::Dealloc(Graphics::ContextEntityId id)
{
	observeeAllocator.Dealloc(id.id);
}

//------------------------------------------------------------------------------
/**
*/
void
ObservableContext::OnInstanceMoved(uint32_t toIndex, uint32_t fromIndex)
{
    n_assert2(fromIndex >= observeeAllocator.Size(), "Instance is assumed to be erased but wasn't!\n");
    
    auto size = observeeAllocator.Size();
    // go through observers and deallocate visibility slot for this object
    const Util::Array<ObserverContext::VisibilityResultAllocator>& visAllocators = ObserverContext::observerAllocator.GetArray<ObserverResultAllocator>();
    for (IndexT i = 0; i < visAllocators.Size(); i++)
    {
        ObserverContext::VisibilityResultAllocator& alloc = visAllocators[i];
        alloc.EraseIndexSwap(toIndex);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
ObservableContext::UpdateModelContextId(Graphics::GraphicsEntityId id, Graphics::ContextEntityId modelCid)
{
    auto cid = GetContextId(id);
    const Util::Array<ObserverContext::VisibilityResultAllocator>& visAllocators = ObserverContext::observerAllocator.GetArray<ObserverResultAllocator>();
    for (IndexT i = 0; i < visAllocators.Size(); i++)
    {
        ObserverContext::VisibilityResultAllocator& alloc = visAllocators[i];
        alloc.Get<VisibilityResultCtxId>(cid.id) = modelCid;
    }
}

} // namespace Visibility
