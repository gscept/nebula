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
    for (i = 0; i < this->batches.Size(); i++)
    {
        if (this->batches[i]->async == processor->async &&
            this->batches[i]->order == processor->order)
        {
            accepted = this->batches[i]->TryInsert(processor);
            if (accepted)
                break;
        }
        if (this->batches[i]->order < processor->order)
        {
            break;
        }
    }
    if (i == this->batches.Size() || !accepted)
    {
        FrameEventBatch* batch = new FrameEventBatch();
        batch->processors.Append(processor);

        this->batches.Append(batch);
    }
}

bool
FrameEventBatch::TryInsert(Processor* processor)
{
    this->processors.Append(processor);
    return true;
}

} // namespace Game
