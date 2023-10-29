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

// batch of frame callbacks
class FrameEventBatch
{
public:
    FrameEventBatch() = default;

    void
    Execute() 
    {
        // TODO: 1. Do query
        //       2. queue one job per table view, for every callback, 
        //       3. wait until all jobs finish executing
        //       4. return
    }

    /// Try to insert a processor into the batch.
    /// Can reject the processor if the batch is executed
    /// async, and the processor is not allowed to run 
    /// simultaneously as the other processor. In this
    /// case, use linear probing to insert the processor
    /// into a new batch
    bool TryInsert(Processor* processor);

    /// sorting order in frame event
    int order;
    bool async;
    Util::Array<Processor*> processors;
};

struct FrameEvent
{
    void AddProcessor(Processor* processor);

    Util::StringAtom name;
    int order = 100;

    Util::Array<FrameEventBatch*> batches;
};

} // namespace Game
