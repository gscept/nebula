#pragma once
//------------------------------------------------------------------------------
/**
    @class Game::FrameBuilder

    @copyright
    (C) 2023 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "game/processor.h"

namespace Game
{

class FramePipeline;

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

    void Run(World* world);
    void AddProcessor(Processor* processor);

    /// prefilter all processors. Should not be done per frame - instead use CacheTable if you need to do incremental caching
    void Prefilter(World* world, bool force = false);
    /// add a table to the caches of any processors that accepts it, that is attached to this frame event
    void CacheTable(MemDb::TableId, MemDb::TableSignature const&);

    Util::StringAtom name;
    int order = 100;

private:
    friend FramePipeline;

    class Batch;

    /// Which pipeline is this event attached to
    FramePipeline* pipeline;

    /// Batches that this event will execute
    Util::Array<Batch*> batches;
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

    /// prefilter all processors. Should not be done per frame - instead use CacheTable if you need to do incremental caching
    void Prefilter(World* world, bool force = false);
    /// add a table to the cache of any processors that accepts it, that is attached to this frame batch
    void CacheTable(MemDb::TableId, MemDb::TableSignature const&);

    /// sorting order in frame event
    int order = 100;
    bool async = false;

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

private:
    friend FrameEvent;

    World* world;
    bool isRunning = false;
    volatile bool inAsync = false;
    IndexT currentIndex = 0;
    Util::Array<FrameEvent*> frameEvents;
};

} // namespace Game
