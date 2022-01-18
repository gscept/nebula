//------------------------------------------------------------------------------
//  system.cc
//  (C) 2018-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
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
VisibilitySystem::PrepareObservers(const Math::mat4* transforms, Util::Array<Math::ClipStatus::Type>* results, const SizeT count)
{
    const Jobs2::CompletionCounter c = 0;
    this->obs.completionCounters.Fill(0, count, c);
    this->obs.transforms = transforms;
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
VisibilitySystem::Run(const Jobs2::CompletionCounter* previousSystemCompletionCounters, const Util::FixedArray<const Jobs2::CompletionCounter*>& extraCounters)
{
    // do nothing
}

//------------------------------------------------------------------------------
/**
*/
Jobs2::CompletionCounter*
VisibilitySystem::GetCompletionCounter(IndexT i)
{
    return &this->obs.completionCounters[i];
}

//------------------------------------------------------------------------------
/**
*/
const Jobs2::CompletionCounter*
VisibilitySystem::GetCompletionCounters() const
{
    return this->obs.completionCounters.Begin();
}

} // namespace Visibility
