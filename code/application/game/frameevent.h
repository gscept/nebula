#pragma once
//------------------------------------------------------------------------------
/**
    @class Game::FrameBuilder

    @copyright
    (C) 2023 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include <functional>
#include "game/processor.h"
#include "game/timesource.h"
#include "threading/assertingmutex.h"

namespace Game
{

class FramePipeline;

//------------------------------------------------------------------------------
/**
    Used to register a frame event in the frame pipeline.
*/
struct FrameEventCreateInfo
{
    /// sorting order of the event in the pipeline
    int order = 100;
    /// name of the event, processors are attached to it with ProcessorBuilder::On
    Util::StringAtom name;
    /// 0 means the event runs once per app frame. When non-zero the event runs once for every fixed step the
    /// time source has available this frame
    Timing::Time fixedStep = 0.0;
    /// time source the fixed steps come from. Only used when fixedStep is non-zero
    uint32_t timeSourceHash = 0;
};

struct ProcessorJobInput
{
    Game::Dataset::View* view;
    Processor* processor;
};

struct ProcessorJobContext
{
    Game::World* world;
    ProcessorJobInput* inputs;
};

//------------------------------------------------------------------------------
/**
*/
class FrameEvent
{
public:
    FrameEvent() = default;
    ~FrameEvent();

    class Batch;

    /// A callback which runs once per event run. Callbacks are executed in order together with the batches,
    /// which is what lets a callback sit between the processors of the event
    struct Callback
    {
        /// sorting order within the frame event
        int order = 100;
        /// which world mode the callback is allowed to run in
        ProcessorPhase phase = PlayPhase;
        std::function<void(World*)> func;
    };

    /// Runs the frame event.
    void Run(World* world);
    /// Adds a processor to the frame event.
    void AddProcessor(Processor* processor);
    /// Adds a callback to the frame event, sorted by order among the batches. The phase is which world mode the
    /// callback is allowed to run in, which is what keeps a callback out of a world that is not simulating
    void AddCallback(int order, ProcessorPhase phase, std::function<void(World*)> func);
    /// Removes a processor for the frame event. Does not free the memory, you need to handle this yourself.
    void RemoveProcessor(Processor* processor);

    /// prefilter all processors. Should not be done per frame - instead use CacheTable if you need to do incremental caching
    void Prefilter(World* world, bool force = false);
    /// add a table to the caches of any processors that accepts it, that is attached to this frame event
    void CacheTable(MemDb::TableId, MemDb::TableSignature const&);

    Util::StringAtom name;
    int order = 100;
    /// 0 when the event runs once per app frame, otherwise the fixed step it runs on
    Timing::Time fixedStep = 0.0;
    /// time source the fixed steps come from, 0 when the event is not fixed
    uint32_t timeSourceHash = 0;

    Util::Array<Batch const*> const GetBatches() const;

private:
    friend FramePipeline;

    /// Which pipeline is this event attached to
    FramePipeline* pipeline;

    /// Batches that this event will execute
    Util::Array<Batch*> batches;

    /// Callbacks that this event will execute, sorted by order
    Util::Array<Callback> callbacks;
};


//------------------------------------------------------------------------------
/**
    A batch of frame callbacks
*/
class FrameEvent::Batch
{
public:
    Batch() = default;
    ~Batch();

    void Execute(World* world);

    /// Try to insert a processor into the batch.
    /// Can reject the processor if the batch is executed
    /// async, and the processor is not allowed to run
    /// simultaneously as the other processor. In this
    /// case, use linear probing to insert the processor
    /// into a new batch
    bool TryInsert(Processor* processor);

    /// Tries to remove a processor from the batch, if it
    /// exists in the batch. returns true if removed.
    bool TryRemove(Processor* processor);

    /// prefilter all processors. Should not be done per frame - instead use CacheTable if you need to do incremental caching
    void Prefilter(World* world, bool force = false);
    /// add a table to the cache of any processors that accepts it, that is attached to this frame batch
    void CacheTable(MemDb::TableId, MemDb::TableSignature const&);

    /// sorting order in frame event
    int order = 100;
    bool async = false;

    Util::Array<Processor const*> GetProcessors() const;

private:
    void ExecuteAsync(World* world);
    void ExecuteSequential(World* world);

    Util::Array<Processor*> processors;
};


//------------------------------------------------------------------------------
/**
*/
class FramePipeline
{
public:
    FramePipeline() = delete;
    FramePipeline(World* world);
    ~FramePipeline();

    FrameEvent* RegisterFrameEvent(FrameEventCreateInfo const& info);
    FrameEvent* RegisterFrameEvent(int order, Util::StringAtom name);
    FrameEvent* GetFrameEvent(Util::StringAtom name);

    /// Start running the pipeline
    void Begin();
    /// Stop running the pipeline
    void End();
    /// Reset the pipeline to the start.
    void Reset();
    /// Run and execute until a certain event is executed (inclusive)
    void RunThru(Util::StringAtom name);
    /// Run until the pipeline ends
    void RunRemaining();

    /// check if the pipeline is currently executing an async frame event batch
    bool IsRunningAsync();

    /// prefilter all processors. Should not be done per frame - instead use CacheTable if you need to do incremental caching
    void Prefilter(bool force = false);
    /// add a table to the caches of any processors that accepts it, that is attached to this frame event
    void CacheTable(MemDb::TableId, MemDb::TableSignature const&);

    // get read copy of frame events. This is not thread safe to read from!
    Util::Array<FrameEvent const*> const GetFrameEvents() const;

private:
    friend FrameEvent;

    /// Runs an event. A fixed event runs once for every step its time source has available
    void RunEvent(FrameEvent* frameEvent);

    World* world;
    bool isRunning = false;
    volatile bool inAsync = false;
    IndexT currentIndex = 0;
    Util::Array<FrameEvent*> frameEvents;
};

} // namespace Game
