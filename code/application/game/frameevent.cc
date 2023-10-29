//------------------------------------------------------------------------------
//  @file frameevent.cc
//  @copyright (C) 2023 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "frameevent.h"
namespace Game
{

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
        batch->processors.Append(processor);
        this->batches.Insert(i, batch);
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

} // namespace Game
