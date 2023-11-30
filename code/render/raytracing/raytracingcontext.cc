//------------------------------------------------------------------------------
// raytracingcontext.cc
// (C) 2023 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "raytracingcontext.h"
#include "models/modelcontext.h"

namespace Raytracing
{

__ImplementContext(RaytracingContext, raytracingContextAllocator);

bool RaytracingContext::TopLevelNeedsRebuild = false;
bool RaytracingContext::ToplevelNeedsUpdate = false;

Util::Array<CoreGraphics::BottomLevelAccelerationId> RaytracingContext::BottomLevelAccelerationStructures;
Memory::RangeAllocator RaytracingContext::BottomLevelAccelerationAllocator;

//------------------------------------------------------------------------------
/**
*/
RaytracingContext::RaytracingContext()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
RaytracingContext::~RaytracingContext()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
RaytracingContext::Create()
{
    __CreateContext();

#ifndef PUBLIC_BUILD
    __bundle.OnRenderDebug = RaytracingContext::OnRenderDebug;
#endif
    Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&__bundle, &__state);
}

//------------------------------------------------------------------------------
/**
*/
void
RaytracingContext::Setup(const Graphics::GraphicsEntityId id)
{
    Graphics::ContextEntityId contextId = GetContextId(id);
    const Models::NodeInstanceRange& nodes = Models::ModelContext::GetModelRenderableRange(id);
    Memory::RangeAllocation alloc = RaytracingContext::BottomLevelAccelerationAllocator.Alloc(nodes.end - nodes.begin);
    SizeT numObjects = nodes.end - nodes.begin;
    
    raytracingContextAllocator.Set<Raytracing_Allocation>(contextId.id, alloc);
    raytracingContextAllocator.Set<Raytracing_NumStructures>(contextId.id, numObjects);

    for (IndexT i = 0; i < numObjects; i++)
    {
        CoreGraphics::BottomLevelAccelerationCreateInfo createInfo;
        createInfo.mesh = Models::ModelContext::NodeInstances.renderable.nodeMeshes[i];
        createInfo.flags = CoreGraphics::AccelerationBuildFlags::FastTrace;
        BottomLevelAccelerationStructures[alloc.offset + i] = CoreGraphics::CreateBottomLevelAcceleration(createInfo);
    }
    RaytracingContext::TopLevelNeedsRebuild = true;
}

//------------------------------------------------------------------------------
/**
*/
void
RaytracingContext::RebuildToplevelAcceleration()
{
    if (RaytracingContext::TopLevelNeedsRebuild)
    {
        if (RaytracingContext::ToplevelAccelerationStructure != CoreGraphics::InvalidTopLevelAccelerationId)
        {
            CoreGraphics::DestroyTopLevelAcceleration(RaytracingContext::ToplevelAccelerationStructure);
        }
        CoreGraphics::TopLevelAccelerationCreateInfo createInfo;
        createInfo.geometries = RaytracingContext::BottomLevelAccelerationStructures;
        createInfo.flags = CoreGraphics::AccelerationBuildFlags::FastBuild | CoreGraphics::AccelerationBuildFlags::Dynamic;
        RaytracingContext::ToplevelAccelerationStructure = CoreGraphics::CreateTopLevelAcceleration(createInfo);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
RaytracingContext::OnRenderDebug(uint32_t flags)
{
}

} // namespace Raytracing
