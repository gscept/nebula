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
        FrameEventBatch* batch = new FrameEventBatch();
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
FrameEventBatch::~FrameEventBatch()
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
FrameEventBatch::Execute(World* world)
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
FrameEventBatch::TryInsert(Processor* processor)
{
    // TODO: if the batch is async, check so that we don't
    // have multiple writers to the same components
    this->processors.Append(processor);
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
FrameEventBatch::Prefilter(World* world, bool force)
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
FrameEventBatch::CacheTable(MemDb::TableId tid, MemDb::TableSignature const& signature)
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
void
FrameEventBatch::ExecuteAsync(World* world)
{
    Util::FixedArray<Dataset> datasets(this->processors.Size());

    uint32_t numJobs = 0;

    for (IndexT i = 0; i < this->processors.Size(); i++)
    {
        Processor* processor = this->processors[i];
        datasets[i] = world->Query(processor->filter, processor->cache);
        numJobs += datasets[i].numViews;
    }

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
FrameEventBatch::ExecuteSequential(World* world)
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

} // namespace Game

