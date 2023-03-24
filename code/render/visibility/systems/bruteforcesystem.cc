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
BruteforceSystem::Run(const Threading::AtomicCounter* const* previousSystemCompletionCounters, const Util::FixedArray<const Threading::AtomicCounter*>& extraCounters)
{
    // This is the context used to provide the job with
    struct Context
    {
        Math::vec4 colX[4], colY[4], colZ[4], colW[4];
        uint32 objectCount;
        const uint32* ids;
        const Math::bbox* boundingBoxes;
        const uint32_t* flags;
        Math::ClipStatus::Type* clipStatuses;
    };

    IndexT i;
    for (i = 0; i < this->obs.count; i++)
    {
        Math::mat4 camera = this->obs.transforms[i];

        n_assert(*this->obs.completionCounters[i] == 0);
        (*this->obs.completionCounters[i]) = 1;

        Context ctx;
        ctx.ids = this->ent.ids;
        ctx.boundingBoxes = this->ent.boxes;
        ctx.flags = this->ent.entityFlags;

        // Setup counters
        Util::FixedArray<const Threading::AtomicCounter*> counters(extraCounters.Size() + (previousSystemCompletionCounters == nullptr ? 0 : 1));
        if (!extraCounters.IsEmpty())
            Memory::CopyElements(extraCounters.Begin(), counters.Begin(), extraCounters.Size());
        if (previousSystemCompletionCounters != nullptr)
            counters[extraCounters.Size()] = previousSystemCompletionCounters[i];

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

        // All set, run the job
        Jobs2::JobDispatch([](SizeT totalJobs, SizeT groupSize, IndexT groupIndex, SizeT invocationOffset, void* ctx)
        {
            N_SCOPE(BruteforceViewFrustumCulling, Visibility);
            auto context = static_cast<Context*>(ctx);

            // Iterate over work group
            for (IndexT i = 0; i < groupSize; i++)
            {
                // Get item index
                IndexT index = i + invocationOffset;
                if (index >= totalJobs)
                    return;

                uint32 objectId = context->ids[index];

                if (AllBits(context->flags[objectId], (uint32_t)Models::NodeInstanceFlags::NodeInstance_AlwaysVisible))
                {
                    context->clipStatuses[index] = Math::ClipStatus::Inside;
                    continue;
                }

                // Run bounding box check and store output in clip statuses, if clip status is still outside
                if (context->clipStatuses[index] == Math::ClipStatus::Outside)
                    context->clipStatuses[index] = context->boundingBoxes[objectId].clipstatus(context->colX, context->colY, context->colZ, context->colW);
            }
        }
        , this->ent.count
        , 1024
        , ctx
        , counters
        , this->obs.completionCounters[i]
        , nullptr);
    }
}
} // namespace Visibility
