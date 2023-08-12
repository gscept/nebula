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
VisibilitySystem::PrepareObservers(const Math::mat4* transforms, bool* orthoFlags, Util::Array<Math::ClipStatus::Type>* results, const SizeT count)
{
    this->obs.completionCounters.Reserve(count);
    for (IndexT i = 0; i < count; i++)
        this->obs.completionCounters.Append(new Threading::AtomicCounter(0));
    this->obs.transforms = transforms;
    this->obs.isOrtho = orthoFlags;
    this->obs.results = results;
    this->obs.count = count;
}

//------------------------------------------------------------------------------
/**
*/
void
VisibilitySystem::PrepareEntities(const Math::bbox* boxes, const uint32* ids, const Graphics::GraphicsEntityId* entities, const uint32_t* entityFlags, const SizeT count)
{
    this->ent.boxes = boxes;
    this->ent.entities = entities;
    this->ent.ids = ids;
    this->ent.entityFlags = entityFlags;
    this->ent.count = count;
}

//------------------------------------------------------------------------------
/**
*/
void
VisibilitySystem::Run(const Threading::AtomicCounter* const* previousSystemCompletionCounters, const Util::FixedArray<const Threading::AtomicCounter*>& extraCounters)
{
    // do nothing
}

//------------------------------------------------------------------------------
/**
*/
const Threading::AtomicCounter*
VisibilitySystem::GetCompletionCounter(IndexT i) const
{
    return this->obs.completionCounters[i];
}

//------------------------------------------------------------------------------
/**
*/
const Threading::AtomicCounter* const*
VisibilitySystem::GetCompletionCounters() const
{
    return this->obs.completionCounters.ConstBegin();
}

} // namespace Visibility
