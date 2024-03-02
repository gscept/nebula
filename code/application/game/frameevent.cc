//------------------------------------------------------------------------------
//  @file frameevent.cc
//  @copyright (C) 2023 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "frameevent.h"
#include "world.h"
#include "memdb/database.h"
#include "jobs2/jobs2.h"

namespace Game
{

//------------------------------------------------------------------------------
/**
*/
void
FrameBatchJob(SizeT totalJobs, SizeT groupSize, IndexT groupIndex, SizeT invocationOffset, void* ctx)
{
    ProcessorJobContext* context = static_cast<ProcessorJobContext*>(ctx);
    for (uint i = 0; i < groupSize; i++)
    {
        IndexT index = i + invocationOffset;
        if (index >= totalJobs)
            return;

        ProcessorJobInput const& input = context->inputs[index];
        input.processor->callback(context->world, *input.view);
    }
}

//------------------------------------------------------------------------------
/**
*/
FrameEvent::~FrameEvent()
{
    for (SizeT i = 0; i < this->batches.Size(); i++)
    {
        delete this->batches[i];
    }
}

//------------------------------------------------------------------------------
/**
*/
void
FrameEvent::Run(World* world)
{
    for (SizeT i = 0; i < this->batches.Size(); i++)
    {
        if (this->batches[i]->async)
        {
            this->pipeline->inAsync = true;
        }

        this->batches[i]->Execute(world);

        this->pipeline->inAsync = false;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
FrameEvent::AddProcessor(Processor* processor)
{
    bool accepted = false;
    int i;

    // try to insert into existing batches
    for (i = 0; i < this->batches.Size(); i++)
    {
        if (this->batches[i]->async == processor->async &&
            this->batches[i]->order == processor->order)
        {
            accepted = this->batches[i]->TryInsert(processor);
            if (accepted)
                break;
        }
        if (this->batches[i]->order > processor->order)
        {
            break;
        }
    }

    if (!accepted)
    {
        FrameEvent::Batch* batch = new FrameEvent::Batch();
        batch->async = processor->async;
        batch->order = processor->order;
        bool res = batch->TryInsert(processor);
        n_assert(res);
        this->batches.Insert(i, batch);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
FrameEvent::Prefilter(World* world, bool force)
{
    for (SizeT i = 0; i < this->batches.Size(); i++)
    {
        this->batches[i]->Prefilter(world, force);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
FrameEvent::CacheTable(MemDb::TableId tid, MemDb::TableSignature const& signature)
{
    for (SizeT i = 0; i < this->batches.Size(); i++)
    {
        this->batches[i]->CacheTable(tid, signature);
    }
}

//------------------------------------------------------------------------------
/**
*/
Util::Array<FrameEvent::Batch const*> const
FrameEvent::GetBatches() const
{
    Util::Array<FrameEvent::Batch const*> batches;
    for (IndexT i = 0; i < this->batches.Size(); i++)
    {
        batches.Append(this->batches[i]);
    }

    return batches;
}

//------------------------------------------------------------------------------
/**
*/
FrameEvent::Batch::~Batch()
{
    for (SizeT i = 0; i < this->processors.Size(); i++)
    {
        delete this->processors[i];
    }
}

//------------------------------------------------------------------------------
/**
*/
void
FrameEvent::Batch::Execute(World* world)
{
    if (this->async)
    {
        this->ExecuteAsync(world);
    }
    else
    {
        this->ExecuteSequential(world);
    }
}

//------------------------------------------------------------------------------
/**
*/
bool
FrameEvent::Batch::TryInsert(Processor* processor)
{
    // if the batch is async, check so that we don't
    // have multiple writers to the same components
    if (processor->async)
    {
        Util::FixedArray<ComponentId> const& components = Game::ComponentsInFilter(processor->filter);
        Util::FixedArray<AccessMode> const& access = Game::AccessModesInFilter(processor->filter);
        for (IndexT i = 0; i < components.Size(); i++)
        {
            for (auto other : this->processors)
            {
                auto const& otherComponents = Game::ComponentsInFilter(other->filter);
                auto const& otherAccess = Game::AccessModesInFilter(other->filter);
                for (IndexT k = 0; k < otherComponents.Size(); k++)
                {
                    if (otherComponents[k] == components[i])
                    {
                        if (otherAccess[k] == AccessMode::WRITE || access[i] == AccessMode::WRITE)
                        {
                            // Some processor is already writing to this component, or we're trying to write, and some other is already reading.
                            // Either way, we cannot add the processor since it will cause races.
                            return false;
                        }
                        break; // we can break because a component should never exist twice in the filter
                    }
                }
            }
        }
    }   
    
    this->processors.Append(processor);
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
FrameEvent::Batch::Prefilter(World* world, bool force)
{
    for (SizeT i = 0; i < this->processors.Size(); i++)
    {
        Processor* processor = this->processors[i];
        if (!processor->cacheValid || force)
        {
            processor->cache = world->GetDatabase()->Query(GetInclusiveTableMask(processor->filter), GetExclusiveTableMask(processor->filter));
            processor->cacheValid = true;
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
FrameEvent::Batch::CacheTable(MemDb::TableId tid, MemDb::TableSignature const& signature)
{
    for (SizeT i = 0; i < this->processors.Size(); i++)
    {
        Processor* processor = this->processors[i];
        if (MemDb::TableSignature::CheckBits(signature, GetInclusiveTableMask(processor->filter)))
        {
            MemDb::TableSignature const& exclusive = GetExclusiveTableMask(processor->filter);
            if (exclusive.IsValid())
            {
                if (!MemDb::TableSignature::HasAny(signature, exclusive))
                    processor->cache.Append(tid);
            }
            else
            {
                processor->cache.Append(tid);
            }
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
Util::Array<Processor const*>
FrameEvent::Batch::GetProcessors() const
{
    Util::Array<Processor const*> procs;
    for (IndexT i = 0; i < this->processors.Size(); i++)
    {
        procs.Append(this->processors[i]);
    }

    return procs;
}

//------------------------------------------------------------------------------
/**
*/
void
FrameEvent::Batch::ExecuteAsync(World* world)
{
    Util::FixedArray<Dataset> datasets(this->processors.Size());

    uint32_t numJobs = 0;

    for (IndexT i = 0; i < this->processors.Size(); i++)
    {
        Processor* processor = this->processors[i];
        datasets[i] = world->Query(processor->filter, processor->cache);
        numJobs += datasets[i].numViews;
    }

    if (numJobs == 0)
        return;

    ProcessorJobContext context;
    context.world = world;
    context.inputs = new ProcessorJobInput[numJobs];

    IndexT inputIndex = 0;
    for (IndexT i = 0; i < datasets.Size(); i++)
    {
        for (IndexT v = 0; v < datasets[i].numViews; v++, inputIndex++)
        {
            context.inputs[inputIndex].processor = this->processors[i];
            context.inputs[inputIndex].view = datasets[i].views + v;
        }
    }

    Threading::Event event;
    Jobs2::JobDispatch(FrameBatchJob, numJobs, 1, context, nullptr, nullptr, &event);
    event.Wait();

    delete[] context.inputs;

    Jobs2::JobNewFrame();
}

//------------------------------------------------------------------------------
/**
*/
void
FrameEvent::Batch::ExecuteSequential(World* world)
{
    for (SizeT i = 0; i < this->processors.Size(); i++)
    {
        Processor* processor = this->processors[i];
        Dataset data = world->Query(processor->filter, processor->cache);
        for (int v = 0; v < data.numViews; v++)
        {
            processor->callback(world, data.views[v]);
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
FramePipeline::FramePipeline(World* world)
    : world(world)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
FramePipeline::~FramePipeline()
{
    for (SizeT i = 0; i < this->frameEvents.Size(); i++)
    {
        delete this->frameEvents[i];
    }
}

//------------------------------------------------------------------------------
/**
*/
FrameEvent*
FramePipeline::RegisterFrameEvent(int order, Util::StringAtom name)
{
    n_assert(!this->isRunning);

    FrameEvent* fEvent = new FrameEvent();
    fEvent->pipeline = this;
    fEvent->name = name;
    fEvent->order = order;

#if NEBULA_DEBUG
    for (int i = 0; i < this->frameEvents.Size(); i++)
    {
        n_assert2(this->frameEvents[i]->name != fEvent->name, "FrameEvent already registered!");
    }
#endif

    int i;
    for (i = 0; i < this->frameEvents.Size(); i++)
    {
        if (this->frameEvents[i]->order > fEvent->order)
        {
            break;
        }
    }
    this->frameEvents.Insert(i, fEvent);

    return fEvent;
}

//------------------------------------------------------------------------------
/**
*/
FrameEvent*
FramePipeline::GetFrameEvent(Util::StringAtom name)
{
    for (int i = 0; i < this->frameEvents.Size(); i++)
    {
        if (this->frameEvents[i]->name == name)
            return this->frameEvents[i];
    }

    n_error("FrameEvent `%s` not found!", name.Value());
    return nullptr;
}

//------------------------------------------------------------------------------
/**
*/
void
FramePipeline::Begin()
{
    n_assert(!this->isRunning);
    this->currentIndex = 0;
    this->isRunning = true;
}

//------------------------------------------------------------------------------
/**
*/
void
FramePipeline::End()
{
    n_assert(this->isRunning);
    this->isRunning = false;
}

void
FramePipeline::Reset()
{
    this->currentIndex = 0;
}

//------------------------------------------------------------------------------
/**
*/
void
FramePipeline::RunThru(Util::StringAtom name)
{
    FrameEvent* frameEvent;
    while (currentIndex < this->frameEvents.Size())
    {
        frameEvent = this->frameEvents[currentIndex++];
        frameEvent->Run(this->world);

        if (frameEvent->name == name)
        {
            break;
        }
    } 
}

//------------------------------------------------------------------------------
/**
*/
void
FramePipeline::RunRemaining()
{
    FrameEvent* frameEvent;
    while (currentIndex < this->frameEvents.Size())
    {
        frameEvent = this->frameEvents[currentIndex++];
        frameEvent->Run(this->world);
    } 
}

//------------------------------------------------------------------------------
/**
*/
bool
FramePipeline::IsRunningAsync()
{
    return this->inAsync;
}

//------------------------------------------------------------------------------
/**
*/
void
FramePipeline::Prefilter(bool force)
{
    for (SizeT i = 0; i < this->frameEvents.Size(); i++)
    {
        this->frameEvents[i]->Prefilter(this->world, force);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
FramePipeline::CacheTable(MemDb::TableId tid, MemDb::TableSignature const& signature)
{
    for (SizeT i = 0; i < this->frameEvents.Size(); i++)
    {
        this->frameEvents[i]->CacheTable(tid, signature);
    }
}

//------------------------------------------------------------------------------
/**
*/
Util::Array<FrameEvent const*> const
FramePipeline::GetFrameEvents() const
{
    Util::Array<FrameEvent const*> events;
    for (IndexT i = 0; i < this->frameEvents.Size(); i++)
    {
        events.Append(this->frameEvents[i]);
    }

    return events;
}

} // namespace Game

