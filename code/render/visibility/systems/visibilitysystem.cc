//------------------------------------------------------------------------------
//  system.cc
//  (C) 2018-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "visibilitysystem.h"
namespace Visibility
{

//------------------------------------------------------------------------------
/**
*/
VisibilitySystem::VisibilitySystem()
{
}

//------------------------------------------------------------------------------
/**
*/
void
VisibilitySystem::PrepareObservers(const Math::mat4* transforms, const bool* orthoFlags, const Graphics::StageMask* stages, Util::Array<Math::ClipStatus::Type>* results, const SizeT count)
{
    this->obs.completionCounters.Resize(count);
    for (auto& counter : this->obs.completionCounters)
        counter = 0;
    this->obs.transforms = transforms;
    this->obs.isOrtho = orthoFlags;
    this->obs.stages = stages;
    this->obs.results = results;
    this->obs.count = count;
}

//------------------------------------------------------------------------------
/**
*/
void
VisibilitySystem::PrepareEntities(const Math::bbox* boxes, const uint32_t* ids, const Graphics::StageMask* stages, const Graphics::GraphicsEntityId* entities, const uint32_t* entityFlags, const SizeT count)
{
    this->ent.boxes = boxes;
    this->ent.entities = entities;
    this->ent.ids = ids;
    this->ent.stages = stages;
    this->ent.entityFlags = entityFlags;
    this->ent.count = count;
}

//------------------------------------------------------------------------------
/**
*/
void
VisibilitySystem::Run(const Threading::Interlocked::AtomicCounter* previousSystemCompletionCounters, const Util::FixedArray<const Threading::Interlocked::AtomicCounter*, true>& extraCounters)
{
    // do nothing
}

//------------------------------------------------------------------------------
/**
*/
const Threading::Interlocked::AtomicCounter
VisibilitySystem::GetCompletionCounter(IndexT i) const
{
    return this->obs.completionCounters[i];
}

//------------------------------------------------------------------------------
/**
*/
const Threading::Interlocked::AtomicCounter*
VisibilitySystem::GetCompletionCounters() const
{
    return this->obs.completionCounters.ConstBegin();
}

} // namespace Visibility
