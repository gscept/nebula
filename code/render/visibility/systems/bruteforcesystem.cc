//------------------------------------------------------------------------------
//  bruteforcesystem.cc
//  (C) 2018-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "bruteforcesystem.h"
#include "visibility/visibilitycontext.h"
#include "graphics/graphicsserver.h"
#include "jobs2/jobs2.h"
#include "math/mat4.h"
#include "math/clipstatus.h"
namespace Visibility
{



//------------------------------------------------------------------------------
/**
*/
void
BruteforceSystem::Setup(const BruteforceSystemLoadInfo& info)
{
}

//------------------------------------------------------------------------------
/**
*/
void
BruteforceSystem::Run(Threading::Event* previousSystemEvent)
{
    // This is the context used to provide the job with
    struct Context
    {
        Math::vec4 colX[4], colY[4], colZ[4], colW[4];
        uint32 objectCount;
        const uint32* ids;
        const Math::bbox* boundingBoxes;
        Math::ClipStatus::Type* clipStatuses;
    } static ctx;

    ctx.ids = this->ent.ids;
    ctx.boundingBoxes = this->ent.boxes;
    ctx.objectCount = this->ent.count;

    IndexT i;
    for (i = 0; i < this->obs.count; i++)
    {
        Math::mat4 camera = this->obs.transforms[i];

        // Splat the matrix such that all _x, _y, ... will contain the column values of x, y, ...
        // This provides a way to rearrange the camera transform into a more SSE friendly matrix transform in the job
        ctx.colX[0] = Math::splat_x((camera).r[0]);
        ctx.colX[1] = Math::splat_x((camera).r[1]);
        ctx.colX[2] = Math::splat_x((camera).r[2]);
        ctx.colX[3] = Math::splat_x((camera).r[3]);

        ctx.colY[0] = Math::splat_y((camera).r[0]);
        ctx.colY[1] = Math::splat_y((camera).r[1]);
        ctx.colY[2] = Math::splat_y((camera).r[2]);
        ctx.colY[3] = Math::splat_y((camera).r[3]);

        ctx.colZ[0] = Math::splat_z((camera).r[0]);
        ctx.colZ[1] = Math::splat_z((camera).r[1]);
        ctx.colZ[2] = Math::splat_z((camera).r[2]);
        ctx.colZ[3] = Math::splat_z((camera).r[3]);

        ctx.colW[0] = Math::splat_w((camera).r[0]);
        ctx.colW[1] = Math::splat_w((camera).r[1]);
        ctx.colW[2] = Math::splat_w((camera).r[2]);
        ctx.colW[3] = Math::splat_w((camera).r[3]);

        ctx.clipStatuses = this->obs.results[i].Begin();

        // Make sure we're done with the previous execution, and that the event is reset
        this->systemDoneEvent.Wait();
        this->systemDoneEvent.Reset();

        // All set, run the job
        Jobs2::JobDispatch([](SizeT totalJobs, SizeT groupSize, IndexT groupIndex, SizeT invocationOffset, void* ctx)
        {
            N_SCOPE(BruteforceViewFrustumCulling, Visibility);
            Context* context = static_cast<Context*>(ctx);

            // Iterate over work group
            for (IndexT i = 0; i < groupSize; i++)
            {
                // Get item index
                IndexT index = i + invocationOffset;
                if (index >= context->objectCount)
                    break;

                uint32 objectId = context->ids[index];

                // Run bounding box check and store output in clip statuses, if clip status is still outside
                if (context->clipStatuses[index] == Math::ClipStatus::Outside)
                    context->clipStatuses[index] = context->boundingBoxes[objectId].clipstatus(context->colX, context->colY, context->colZ, context->colW);
            }
        }, ctx.objectCount, 1024, &ctx, previousSystemEvent, &this->systemDoneEvent);

        /*
        Jobs::JobContext ctx;

        // uniform data is the observer transform
        ctx.uniform.numBuffers = 2;
        ctx.uniform.scratchSize = 0;

        // Observer transforms
        ctx.uniform.data[0] = (unsigned char*)&this->obs.transforms[i];
        ctx.uniform.dataSize[0] = sizeof(Math::mat4);

        // Entity boxes (this is from the global array from ModelContext)
        ctx.uniform.data[1] = (unsigned char*)this->ent.boxes;
        ctx.uniform.dataSize[1] = sizeof(Math::bbox);

        ctx.input.numBuffers = 1;
        ctx.input.data[0] = (unsigned char*)this->ent.ids;
        ctx.input.dataSize[0] = sizeof(uint32) * this->ent.count;
        ctx.input.sliceSize[0] = sizeof(uint32);

        // the output is the visibility result
        ctx.output.numBuffers = 1;
        ctx.output.data[0] = (unsigned char*)this->obs.results[i].Begin();
        ctx.output.dataSize[0] = sizeof(Math::ClipStatus::Type) * this->ent.count;
        ctx.output.sliceSize[0] = sizeof(Math::ClipStatus::Type);

        // create and run job
        Jobs::JobId job = Jobs::CreateJob({ BruteforceSystemJobFunc });
        Jobs::JobSchedule(job, Graphics::GraphicsServer::renderSystemsJobPort, ctx);

        // enqueue here, but don't dequeue as VisibilityContext will do it for us
        ObserverContext::runningJobs.Enqueue(job);
        */
    }
}
} // namespace Visibility
