//------------------------------------------------------------------------------
//  animsequencer.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "animation/animsequencer.h"
#include "animation/playclipjob.h"

using namespace Util;
using namespace CoreAnimation;
using namespace Jobs;
using namespace Math;

// for debug visualization
#include "coregraphics/shaperenderer.h"
#include "coregraphics/transformdevice.h"
#include "coregraphics/textrenderer.h"
#include "threading/thread.h"
using namespace CoreGraphics;

namespace Animation
{
//------------------------------------------------------------------------------
/**
*/
AnimSequencer::AnimSequencer() :
    time(0),
    debugHudEnabled(false)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
AnimSequencer::~AnimSequencer()
{
    if (this->IsValid())
    {
        this->Discard();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
AnimSequencer::Setup(const Ptr<AnimResource>& animRsrc)
{
    n_assert(!this->IsValid());
    n_assert(animRsrc.isvalid());
    this->time = 0;
    this->animResource = animRsrc;
    this->dstSampleBuffer = AnimSampleBuffer::Create();
    this->dstSampleBuffer->Setup(animRsrc);

    #if NEBULA_ANIMATIONSYSTEM_VERBOSELOG
    n_dbgout("AnimSequencer::Setup(%d)\n", animRsrc->GetResourceId().Value());
    #endif
}

//------------------------------------------------------------------------------
/**
    FIXME: do we have to wait for the anim jobs to finish here??
*/
void
AnimSequencer::Discard()
{
    n_assert(this->IsValid());

    #if NEBULA_ANIMATIONSYSTEM_VERBOSELOG
    n_dbgout("AnimSequencer::Discard(%d)\n", this->animResource->GetResourceId().Value());
    #endif

    this->animResource = 0;
    this->dstSampleBuffer->Discard();
    this->dstSampleBuffer = 0;    
    IndexT i;
    for (i = 0; i < this->enqueuedAnimJobs.Size(); i++)
    {
        this->enqueuedAnimJobs[i]->OnRemoveFromSequencer();
        this->enqueuedAnimJobs[i] = 0;
    }
    for (i = 0; i < this->animJobs.Size(); i++)
    {
        this->animJobs[i]->OnRemoveFromSequencer();
        this->animJobs[i] = 0;
    }
    this->animJobs.Clear();
    this->enqueuedAnimJobs.Clear();
    this->stoppedAnimJobs.Clear();
    this->jobChain.Clear();
}

//------------------------------------------------------------------------------
/**
    Enqueue an anim job. This will schedule the anim job for insertion in
    the next Evaluate(). This deferred handling is necessary because the
    actual base time of the anim job job is only known in Evaluate() (need
    to be careful to prevent those pesky one-frame problems).
*/
void
AnimSequencer::EnqueueAnimJob(const Ptr<AnimJob>& animJob)
{
    n_assert(!animJob->IsAttachedToSequencer());
    n_assert((InvalidIndex == this->enqueuedAnimJobs.FindIndex(animJob)) &&
             (InvalidIndex == this->animJobs.FindIndex(animJob)) &&
             (InvalidIndex == this->stoppedAnimJobs.FindIndex(animJob)));
    //n_assert(this->enqueuedAnimJobs.Size() < 16);
    this->enqueuedAnimJobs.Append(animJob);
    animJob->OnAttachedToSequencer(*this);
}

//------------------------------------------------------------------------------
/**
    Stop an anim job (allow fade-out), this is a private helper method. The 
    anim job will simply be appended to the stoppedAnimJobs array, and it will
    be taken care of during the next UpdateStoppedAnimJobs().

    NOTE: anim jobs which may already be stopping will be handled here as
    well, but those will be skipped in UpdateStoppedAnimJobs()!
*/
void
AnimSequencer::EnqueueAnimJobForStopping(const Ptr<AnimJob>& animJob)
{
    n_assert(this->stoppedAnimJobs.Size() < 16);
    if (InvalidIndex == this->stoppedAnimJobs.FindIndex(animJob))
    {
        this->stoppedAnimJobs.Append(animJob);

        #if NEBULA_ANIMATIONSYSTEM_VERBOSELOG
        n_dbgout("AnimSequencer::EnqueueAnimJobForStopping(animRsrc='%s' animJob='%s')\n", 
            this->animResource->GetResourceId().Value(),
            animJob->GetName().Value());
        #endif
    }
}

//------------------------------------------------------------------------------
/**
    Cancel an anim job, this will immediately remove the anim job from the
    sequencer. This is a private helper method. The anim job may either
    reside in the animJobs or enqueuedAnimJobs array.

    NOTE: the ref-by-copy argument is intended!
*/
void
AnimSequencer::DiscardAnimJob(Ptr<AnimJob> animJob)
{
    n_assert(animJob->IsAttachedToSequencer());

    #if NEBULA_ANIMATIONSYSTEM_VERBOSELOG
    n_dbgout("AnimSequencer::DiscardAnimJob(animRsrc='%s' animJob='%s')\n", 
        this->animResource->GetResourceId().Value(),
        animJob->GetName().Value());
    #endif

    animJob->OnRemoveFromSequencer();
    IndexT index = this->animJobs.FindIndex(animJob);
    if (InvalidIndex != index)
    {
        this->animJobs.EraseIndex(index);
    }
    index = this->enqueuedAnimJobs.FindIndex(animJob);
    if (InvalidIndex != index)    
    {
        this->enqueuedAnimJobs.EraseIndex(index);
    }
    index = this->stoppedAnimJobs.FindIndex(animJob);
    if (InvalidIndex != index)
    {
        this->stoppedAnimJobs.EraseIndex(index);
    }
}

//------------------------------------------------------------------------------
/**
    Stop or cancel all anim jobs on a given track.
*/
void
AnimSequencer::StopTrack(IndexT trackIndex, bool allowFadeOut)
{
    #if NEBULA_ANIMATIONSYSTEM_VERBOSELOG
    // n_dbgout("AnimSequencer::StopTrack(trackIndex=%d, allowFadeOut=%s)\n", trackIndex, allowFadeOut ? "true" : "false");
    #endif

    // start at end of arrays since anim jobs may be removed from the
    // array during the loop
    IndexT i;
    for (i = this->animJobs.Size() - 1; i != InvalidIndex; i--)
    {
        if (this->animJobs[i]->GetTrackIndex() == trackIndex)
        {
            if (allowFadeOut) this->EnqueueAnimJobForStopping(this->animJobs[i]);
            else              this->DiscardAnimJob(this->animJobs[i]);
        }
    }
    for (i = this->enqueuedAnimJobs.Size() - 1; i != InvalidIndex; i--)
    {
        if (this->enqueuedAnimJobs[i]->GetTrackIndex() == trackIndex)
        {
            this->DiscardAnimJob(this->enqueuedAnimJobs[i]);
        }
    }
}

//------------------------------------------------------------------------------
/**
    Stop or cancel all anim jobs in the sequencer.
*/
void
AnimSequencer::StopAllTracks(bool allowFadeOut)
{
    #if NEBULA_ANIMATIONSYSTEM_VERBOSELOG
    n_dbgout("AnimSequencer::StopAllTracks(allowFadeOut=%s)\n", allowFadeOut ? "true" : "false");
    #endif

    // start at end of array since anim jobs may be removed from the
    // array during the loop
    IndexT i;
    for (i = this->animJobs.Size() - 1; i != InvalidIndex; i--)
    {
        if (allowFadeOut) this->EnqueueAnimJobForStopping(this->animJobs[i]);
        else              this->DiscardAnimJob(this->animJobs[i]);
    }
    for (i = this->enqueuedAnimJobs.Size() - 1; i != InvalidIndex; i--)
    {
        this->DiscardAnimJob(this->enqueuedAnimJobs[i]);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
AnimSequencer::PauseTrack(IndexT trackIndex, bool pause)
{
	IndexT i;
	for (i = 0; i < this->animJobs.Size(); i++)
	{
		if (this->animJobs[i]->GetTrackIndex() == trackIndex)
		{
			this->animJobs[i]->Pause(pause);
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
AnimSequencer::PauseAllTracks(bool pause)
{
	IndexT i;
	for (i = 0; i < this->animJobs.Size(); i++)
	{
		this->animJobs[i]->Pause(pause);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
AnimSequencer::SetTime(Timing::Tick time)
{
	IndexT i;
	for (i = 0; i < this->animJobs.Size(); i++)
	{
		this->animJobs[i]->SetTime(time);
	}
}

//------------------------------------------------------------------------------
/**
    This method priority-inserts new anim jobs from the startedAnimJobs
    array into the animJobs array and sets the base time of the anim job so
    that it is properly synchronized with the current time. This method
    is called from Evaluate().

    Anim jobs will always be sorted behind existing jobs with the same
    track index. The base time will be set to the finish time of the
    last anim job on the same track.

    There are 2 exceptions to the rule:

    * if an infinite animation track with the same track index exists
      (which is not currently stopping), then the new anim job
      will be dropped
    * if another, active anim job with the same exclusive tag exists
      on the same track, the new anim job will be dropped      
*/
void
AnimSequencer::InsertEnqueuedAnimJobs(Timing::Tick time)
{
    // for each new anim job...
    IndexT enqueuedAnimJobIndex;
    for (enqueuedAnimJobIndex = 0; enqueuedAnimJobIndex < this->enqueuedAnimJobs.Size(); enqueuedAnimJobIndex++)
    {
        const Ptr<AnimJob>& newAnimJob = this->enqueuedAnimJobs[enqueuedAnimJobIndex];
        IndexT newAnimJobExclTag = newAnimJob->GetExclusiveTag();
        const StringAtom& newAnimJobName = newAnimJob->GetName();
        AnimJobEnqueueMode::Code enqueueMode = newAnimJob->GetEnqueueMode();
        Timing::Tick baseTime = time;
        bool dropAnimJob = false;

        // find the right insertion index for the current anim job
        IndexT insertionIndex;
        for (insertionIndex = 0; insertionIndex < this->animJobs.Size(); insertionIndex++)
        {
            bool stopCurJob = false;
            const Ptr<AnimJob>& curAnimJob = this->animJobs[insertionIndex];
            if (curAnimJob->GetTrackIndex() > newAnimJob->GetTrackIndex())
            {
                break;
            }
            else if (curAnimJob->GetTrackIndex() == newAnimJob->GetTrackIndex())
            {
                // check for conditions to drop (ignore) the new anim job
                if (AnimJobEnqueueMode::Intercept == enqueueMode)
                {
                    // intercept mode: stop other anim jobs on same track
                    curAnimJob->Stop(time);

                    // need to proceed to next job without actually updating the baseTime!
                    // otherweise we would be inserted BEFORE the current job, which
                    // would mess up priority blending
                    continue;
                }
                else if (AnimJobEnqueueMode::IgnoreIfSameClipActive == enqueueMode)
                {
                    if ((curAnimJob->GetName() == newAnimJobName) && (!curAnimJob->IsStoppingOrExpired(time)))
                    {
                        dropAnimJob = true;
                        #if NEBULA_ANIMATIONSYSTEM_VERBOSELOG
                        n_dbgout("AnimSequencer::InsertEnqueuedAnimJobs(): dropping new anim job '%s' (reason: IgnoreIfSameClipActive)!\n",
                            newAnimJob->GetName().Value());
                        #endif
                        break;
                    }
                    else
                    {
                        // ...otherwise behave like Intercept mode
                        curAnimJob->Stop(time);
                        continue;
                    }
                }
                else if (AnimJobEnqueueMode::IgnoreIfSameExclTagActive == enqueueMode)
                {
                    if ((curAnimJob->GetExclusiveTag() == newAnimJobExclTag) && (!curAnimJob->IsStoppingOrExpired(time)))
                    {
                        dropAnimJob = true;
                        #if NEBULA_ANIMATIONSYSTEM_VERBOSELOG
                        n_dbgout("AnimSequencer::InsertEnqueuedAnimJobs(): dropping new anim job '%s' (reason: IgnoreIfSameExclTagActive)!\n",
                            newAnimJob->GetName().Value());
                        #endif
                        break;
                    }
                    else
                    {
                        // ...otherwise behave like Intercept mode
                        curAnimJob->Stop(time);
                        continue;
                    }
                }
                else if ((AnimJobEnqueueMode::Append == enqueueMode) && curAnimJob->IsInfinite())
                {
                    n_error("AnimSequencer::InsertEnqueuedAnimJobs(): cannot insert anim job '%s' because track is blocked by infinite clip!\n", curAnimJob->GetName().AsString().AsCharPtr());
                }

                // track the new base time (must be pre-fadeout-time of previous anim job on same track)
                baseTime = curAnimJob->GetAbsoluteStopTime();
            }
        }

        // baseTime, insertionIndex and dropAnimJob are now set
        if (!dropAnimJob)
        {
            this->animJobs.Insert(insertionIndex, newAnimJob);
            newAnimJob->SetBaseTime(baseTime);

            #if NEBULA_ANIMATIONSYSTEM_VERBOSELOG
            n_dbgout("AnimSequencer::InsertEnqueuedAnimJobs(): inserting new anim job '%s' at index %d (animRes=%s)\n", 
                newAnimJob->GetName().Value(), insertionIndex, this->animResource->GetResourceId().Value());
            #endif
        }
        else
        {
            // ignore the animation job because of enqueue mode 
            newAnimJob->OnRemoveFromSequencer();
        }
    }

    // clear the started anim jobs array for the next "frame"
    this->enqueuedAnimJobs.Clear();
}

//------------------------------------------------------------------------------
/**
    This method updates the duration of each new stopped anim job
    so that it will stop at "now + fadeOutTime". This method is called
    from Evaluate() with the current evaluation time.
*/
void
AnimSequencer::UpdateStoppedAnimJobs(Timing::Tick time)
{
    // for each stopped anim job...
    // start at end of array in case some called method removes jobs from the array
    IndexT stoppedAnimJobIndex;
    for (stoppedAnimJobIndex = this->stoppedAnimJobs.Size() - 1; 
         stoppedAnimJobIndex != InvalidIndex; 
         stoppedAnimJobIndex--)
    {    
        // NOTE: use pointer copy because the array slot may become
        // invalid after DiscardAnimJob!
        Ptr<AnimJob> animJob = this->stoppedAnimJobs[stoppedAnimJobIndex];

        if (animJob->IsPending(time))
        {
            // if the anim job is expired or hasn't started yet, discard it right away
            #if NEBULA_ANIMATIONSYSTEM_VERBOSELOG
            n_dbgout("AnimSequencer::UpdateStoppedAnimJobs(): discard anim job because it is PENDING (%s)\n", animJob->GetName().Value());
            #endif
            this->DiscardAnimJob(animJob);
        }
        else if (animJob->IsExpired(time))
        {
            #if NEBULA_ANIMATIONSYSTEM_VERBOSELOG
            n_dbgout("AnimSequencer::UpdateStoppedAnimJobs(): discard anim job because it is EXPIRED (%s)\n", animJob->GetName().Value());
            #endif
            this->DiscardAnimJob(animJob);
        }
        else if (animJob->IsStoppingOrExpired(time))
        {
            // if the anim job is already stopping (fadeout-phase), don't do anything
            #if NEBULA_ANIMATIONSYSTEM_VERBOSELOG
            n_dbgout("AnimSequencer::UpdateStoppedAnimJobs(): anim job already STOPPING (%s)\n", animJob->GetName().Value());
            #endif
        }
        else
        {
            // stop the animation job (this will just compute a new duration)
            #if NEBULA_ANIMATIONSYSTEM_VERBOSELOG
            n_dbgout("AnimSequencer::UpdateStoppedAnimJobs(): actually stop anim job '%s'\n", animJob->GetName().Value());
            #endif
            animJob->Stop(time);
        }
    }
                     
    // clear the stopped anim jobs array for the next "frame"
    this->stoppedAnimJobs.Clear();
}

//------------------------------------------------------------------------------
/**
    Iterates through the currently attached anim jobs and
    removes all anim jobs which are expired. This method is called from
    Evaluate().
*/
void
AnimSequencer::RemoveExpiredAnimJobs(Timing::Tick time)
{
    IndexT i;
    for (i = this->animJobs.Size() - 1; i != InvalidIndex; i--)
    {
        if (this->animJobs[i]->IsExpired(time))
        {
            #if NEBULA_ANIMATIONSYSTEM_VERBOSELOG
            n_dbgout("AnimSequencer::RemoveExpiredAnimJobs(): discard anim job because it is expired (%s)\n", this->animJobs[i]->IsPending(time) ? "pending" : "expired");
            #endif
            this->DiscardAnimJob(this->animJobs[i]);
        }
    }
}   

//------------------------------------------------------------------------------
/**
    Update the current time of the sequencer. This should be called
    exactly once per frame, even if the animated object is currently
    invisible (near the camera but outside the view volume). This will
    update the anim jobs which have been started or stopped this frame,
    and it will remove expired anim jobs, but will not sample the animation.
*/
void
AnimSequencer::UpdateTime(Timing::Tick curTime)
{
    // update our current time
    this->time = curTime;

    // update any new stopped anim jobs (before inserting new jobs)
    this->UpdateStoppedAnimJobs(time);

    // insert new anim jobs
    this->InsertEnqueuedAnimJobs(time);
    
    // discard any expired anim jobs
    this->RemoveExpiredAnimJobs(time);

    // update time of all active anim jobs
    this->UpdateTimeActiveAnimJobs(time);

    // render debug visualization
    if (this->debugHudEnabled)
    {
        this->DebugRenderHud(curTime);
    }

    // dump anim job info
    #if NEBULA_ANIMATIONSYSTEM_FRAMEDUMP
    this->DumpFrameDebugInfo(time);
    #endif
}

//------------------------------------------------------------------------------
/**
    This method should be called once per-frame for each visible animated
    object AFTER UpdateTime() has been called. Actual animation sampling
    and mixing happens here.
*/
bool
AnimSequencer::StartAsyncEvaluation(const Ptr<JobPort>& jobPort)
{
    // clear previous jobs which may still be lying around
    this->jobChain.Clear();

    // get the number of active anim jobs beforehand, if only one active anim job
    // exists, sample directly into the destination buffer without mixing
    IndexT i;
    IndexT numActiveAnimJobs = 0;
    for (i = 0; i < this->animJobs.Size(); i++)
    {
        if (this->animJobs[i]->IsActive(this->time))
        {
            numActiveAnimJobs++;
        }
    }

    // if no active anim job exists, return false
    if (0 == numActiveAnimJobs)
    {
        return false;
    }
    else
    {
        // more then one active anim job at the current time stamp, need to perform mixing
        bool firstActiveAnimJob = true;
        for (i = 0; i < this->animJobs.Size(); i++)
        {
            if (this->animJobs[i]->IsActive(this->time))
            {
                if (firstActiveAnimJob)
                {
                    // first anim job doesn't require mixing, samples into dstSampleBuffer
                    this->jobChain.Append(this->animJobs[i]->CreateEvaluationJob(this->time, 0, this->dstSampleBuffer));
                    firstActiveAnimJob = false;
                }
                else
                {
                    // a following anim job, uses the dstSampleBuffer (with the previous result)
                    // as mixing input, and writes the result back into dstSampleBuffer, 
                    // we also need to wait for the previous job to finish...
                    this->jobChain.Append(this->animJobs[i]->CreateEvaluationJob(this->time, this->dstSampleBuffer, this->dstSampleBuffer));
                }
            }
        }
        
        // start the created job chain
        if (!this->jobChain.IsEmpty())
        {
            jobPort->PushJobChain(this->jobChain);
        }

        return true;
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
AnimSequencer::UpdateTimeActiveAnimJobs(Timing::Tick time)
{
    // method must be called after new anim jobs are inserted
    n_assert(this->enqueuedAnimJobs.IsEmpty());
    IndexT i;
    for (i = 0; i < this->animJobs.Size(); i++)
    {
        this->animJobs[i]->UpdateTimes(time);        
    }
}

//------------------------------------------------------------------------------
/**
*/
Array<Ptr<AnimJob> >
AnimSequencer::GetAllAnimJobs() const
{
    Array<Ptr<AnimJob> > result;
    SizeT numJobs = this->animJobs.Size() + this->enqueuedAnimJobs.Size();
    if (numJobs > 0)
    {
        result.Reserve(numJobs);
        result.AppendArray(this->animJobs);
        result.AppendArray(this->enqueuedAnimJobs);
    }             
    return result;
}

//------------------------------------------------------------------------------
/**
*/
Array<Ptr<AnimJob> >
AnimSequencer::GetAnimJobsByTrackIndex(IndexT trackIndex) const
{
    Array<Ptr<AnimJob> > result;
    result.Reserve(this->animJobs.Size() + this->enqueuedAnimJobs.Size());
    IndexT i;
    for (i = 0; i < this->animJobs.Size(); i++)
    {
        if (this->animJobs[i]->GetTrackIndex() == trackIndex)
        {
            result.Append(this->animJobs[i]);
        }
    }
    for (i = 0; i < this->enqueuedAnimJobs.Size(); i++)
    {
        if (this->enqueuedAnimJobs[i]->GetTrackIndex() == trackIndex)
        {
            result.Append(this->enqueuedAnimJobs[i]);
        }
    }
    return result;
}

//------------------------------------------------------------------------------
/**
*/
Array<Ptr<AnimJob> >
AnimSequencer::GetAnimJobsByName(const Util::StringAtom& name) const
{
    Array<Ptr<AnimJob> > result;
	if(this->animJobs.Size() + this->enqueuedAnimJobs.Size() == 0)
	{
		return result;
	}
    result.Reserve(this->animJobs.Size() + this->enqueuedAnimJobs.Size());
    IndexT i;
    for (i = 0; i < this->animJobs.Size(); i++)
    {
        if (this->animJobs[i]->GetName() == name)
        {
            result.Append(this->animJobs[i]);
        }
    }
    for (i = 0; i < this->enqueuedAnimJobs.Size(); i++)
    {
        if (this->enqueuedAnimJobs[i]->GetName() == name)
        {
            result.Append(this->enqueuedAnimJobs[i]);
        }
    }
    return result;
}

//------------------------------------------------------------------------------
/**
    Collects all AnimEventInfos of all animjobs which are active in the given time range.
    If justDominatingJob flag is set, just use the clip with most blend factor.
*/
Util::Array<AnimEventInfo> 
AnimSequencer::EmitAnimEvents(Timing::Tick startTime, Timing::Tick endTime, bool justDominatingJob, const Util::String& optionalCatgeory) const
{
    Util::Array<AnimEventInfo> eventInfos;
    SizeT jobCount = this->animJobs.Size();

    IndexT i = 0;
    if (justDominatingJob)
    {
        // overwrite loop parameters
        i = this->FindDominatingAnimJobIndex(startTime, endTime);
        jobCount = (InvalidIndex == i) ? -1 : 1 + i;
    }

    for (; i < jobCount; i++)
    {
		const Ptr<AnimJob>& job = this->animJobs[i];
		if (job->IsActive(startTime) && !job->IsPaused())
        {
			Timing::Tick effectiveEndTime;
			if (job->IsInfinite())
			{
				effectiveEndTime = endTime;
			}
			else
			{
				effectiveEndTime = n_min(endTime, job->GetAbsoluteEndTime());
			}			
			eventInfos.AppendArray(job->EmitAnimEvents(startTime, effectiveEndTime, optionalCatgeory));
        }
    }
    return eventInfos;
}

//------------------------------------------------------------------------------
/**
    Find the dominating, active job.
*/
IndexT
AnimSequencer::FindDominatingAnimJobIndex(Timing::Tick startTime, Timing::Tick endTime) const
{
    IndexT maxTrackIndex = 0;
    IndexT maxIndex = InvalidIndex;
    IndexT i;
    for (i = 0; i < this->animJobs.Size(); i++)
    {
        const Ptr<AnimJob> curAnimJob = this->animJobs[i];
        if (curAnimJob->IsActive(startTime) && 
            curAnimJob->IsActive(endTime) &&
            curAnimJob->GetTrackIndex() >= maxTrackIndex)
        {
            maxTrackIndex = curAnimJob->GetTrackIndex();
            maxIndex = i;
        }
    }
    return maxIndex;
}

//------------------------------------------------------------------------------
/**
    Dump debug info about this frame's state.
*/
#if NEBULA_ANIMATIONSYSTEM_FRAMEDUMP
void
AnimSequencer::DumpFrameDebugInfo(Timing::Tick time)
{
    n_dbgout("=== AnimSequencer(ticks=%d, resId=%s) Frame Dump:\n", time, this->animResource->GetResourceId().Value());
    IndexT i;
    for (i = 0; i < this->animJobs.Size(); i++)
    {
        const Ptr<AnimJob> curAnimJob = this->animJobs[i];
        const char* state;
        if (curAnimJob->IsExpired(time)) state = "expired";
        else if (curAnimJob->IsStoppingOrExpired(time)) state = "stopping";
        else if (curAnimJob->IsPending(time)) state = "pending";
        else if (curAnimJob->IsActive(time)) state = "active";
        else state = "<invalid>";

        n_dbgout("    name(%s) track(%d) start(%d) end(%s) fade(%d,%d) weight(%.2f) timeFactor(%.2f) state(%s)\n",
            curAnimJob->GetName().Value(), 
            curAnimJob->GetTrackIndex(),
            curAnimJob->GetStartTime(),
            curAnimJob->IsInfinite() ? "infinite" : String::FromInt(curAnimJob->GetStartTime() + curAnimJob->GetDuration()).AsCharPtr(),
            curAnimJob->GetFadeInTime(), curAnimJob->GetFadeOutTime(),
            curAnimJob->GetBlendWeight(),
            curAnimJob->GetTimeFactor(),
            state);
    }
}
#endif

//------------------------------------------------------------------------------
/**
    Render a debug visualisation hud of the AnimSequencer.
    FIXME: hmm 2D visualization is tricky ATM.
*/
void
AnimSequencer::DebugRenderHud(Timing::Tick time)
{
    if (!this->animJobs.IsEmpty())
    {
        // render the track grid
        Array<CoreGraphics::RenderShape::RenderShapeVertex> vertices;
        vertices.Reserve(this->animJobs.Size() * 8);
        this->DebugGenTrackLines(12, vertices);
        this->DebugRenderLineList(vertices, float4(1.0f, 0.0f, 0.0f, 0.25f));
        
        // generate rectangles for anim jobs
        vertices.Clear();
        IndexT i;
        for (i = 0; i < this->animJobs.Size(); i++)
        {
            this->DebugGenAnimJobRectangle(time, this->animJobs[i], vertices);
            this->DebugRenderAnimJobText(time, this->animJobs[i], float4(0.5f, 0.0f, 0.0f, 1.0f));
        }
        this->DebugRenderLineList(vertices, float4(0.0f, 0.0f, 0.5f, 1.0f));

        // create the "now" line
        vertices.Clear();
        vertices.Append(this->DebugViewSpaceCoord(time, time, 0));
        vertices.Append(this->DebugViewSpaceCoord(time, time, 12));
        this->DebugRenderLineList(vertices, float4(1.0f, 0.0f, 0.0f, 1.0f));
    }
}

//------------------------------------------------------------------------------
/**
    Generate the track lines for debug visualization.
*/
void
AnimSequencer::DebugGenTrackLines(SizeT maxTracks, Array<CoreGraphics::RenderShape::RenderShapeVertex>& outVertices)
{
    IndexT i;
    for (i = 0; i < maxTracks; i++)
    {
        CoreGraphics::RenderShape::RenderShapeVertex left = this->DebugViewSpaceCoord(0, -100000, i);
        CoreGraphics::RenderShape::RenderShapeVertex right = this->DebugViewSpaceCoord(0, 100000, i);
        outVertices.Append(left);
        outVertices.Append(right);
    }
}

//------------------------------------------------------------------------------
/**
    Generate vertices to render an anim job rectangle and append them
    to the provided vertex array.
*/
void
AnimSequencer::DebugGenAnimJobRectangle(Timing::Tick timeOrigin, const Ptr<AnimJob>& animJob, Array<CoreGraphics::RenderShape::RenderShapeVertex>& outVertices)
{
    Timing::Tick startTime = animJob->GetAbsoluteStartTime();
    Timing::Tick endTime = animJob->IsInfinite() ? (timeOrigin + 1000000) : (animJob->GetAbsoluteEndTime());
    IndexT trackIndex = animJob->GetTrackIndex();
    CoreGraphics::RenderShape::RenderShapeVertex topLeft = this->DebugViewSpaceCoord(timeOrigin, startTime, trackIndex);
    CoreGraphics::RenderShape::RenderShapeVertex topRight = this->DebugViewSpaceCoord(timeOrigin, endTime, trackIndex);
    CoreGraphics::RenderShape::RenderShapeVertex bottomRight = this->DebugViewSpaceCoord(timeOrigin, endTime, trackIndex + 1);
    CoreGraphics::RenderShape::RenderShapeVertex bottomLeft = this->DebugViewSpaceCoord(timeOrigin, startTime, trackIndex + 1);
    outVertices.Append(topLeft); outVertices.Append(topRight);
    outVertices.Append(topRight); outVertices.Append(bottomRight);
    outVertices.Append(bottomRight); outVertices.Append(bottomLeft);
    outVertices.Append(bottomLeft); outVertices.Append(topLeft);
}

//------------------------------------------------------------------------------
/**
    Render the debug text for an anim job.
    FIXME: these coordinate transformations suck... the debug visualization
    subsystem should allow to render text either in screen space or world
    space.
*/
void
AnimSequencer::DebugRenderAnimJobText(Timing::Tick timeOrigin, const Ptr<AnimJob>& animJob, const float4& color)
{
    String text;
    text.Format("  %d: %s w=%.1f f=%.1f", animJob->GetTrackIndex(), animJob->GetName().Value(), animJob->GetBlendWeight(), animJob->GetTimeFactor());

    // argh... compute a 2d screen space coordinate (0..1)
    CoreGraphics::RenderShape::RenderShapeVertex viewSpaceCoord = this->DebugViewSpaceCoord(timeOrigin, animJob->GetAbsoluteStartTime(), animJob->GetTrackIndex());
    const matrix44& proj = TransformDevice::Instance()->GetProjTransform();
    Math::float4 screenPos = matrix44::transform(viewSpaceCoord.pos, proj);
    screenPos.x() /= screenPos.w();
    screenPos.y() /= screenPos.w();
    screenPos.x() = (screenPos.x() + 1.0f) * 0.5f;
    screenPos.y() = 1.0f - ((screenPos.y() + 1.0f) * 0.5f);

    TextElement textElement(Threading::Thread::GetMyThreadId(), text, color, float2(screenPos.x(), screenPos.y()), 15);
    TextRenderer::Instance()->AddTextElement(textElement);
}

//------------------------------------------------------------------------------
/**
    Debug visualization: convert a time value and track index into a view space
    coordinates. Range is from -1.0 to +1.0 for x and y, and a negative value
    for z.
*/
CoreGraphics::RenderShape::RenderShapeVertex
AnimSequencer::DebugViewSpaceCoord(Timing::Tick timeOrigin, Timing::Tick time, IndexT trackIndex)
{
    Timing::Tick relTime = (time - timeOrigin) + 1000;  // show 1 second of the past
    float trackHeight = 0.025f;
    float tickWidth  = 0.0003f;
    float x = (n_clamp(relTime * tickWidth, 0.0f, 1.0f) * 2.0f) - 1.0f;
    float y = -((n_clamp(trackIndex * trackHeight, 0.0f, 1.0f) * 2.0f) - 1.0f);
    CoreGraphics::RenderShape::RenderShapeVertex vert;
    vert.pos = point(x * 0.5f, y * 0.25f, -0.5f);
    return vert;
}

//------------------------------------------------------------------------------
/**
    Render a debug line list.
    FIXME: the DebugShapeRenderer should allow for a method like this!
*/
void
AnimSequencer::DebugRenderLineList(const Array<RenderShape::RenderShapeVertex>& vertices, const float4& color)
{
    matrix44 invView = TransformDevice::Instance()->GetInvViewTransform();
    RenderShape shape;
    shape.SetupPrimitives(Threading::Thread::GetMyThreadId(), 
                          invView, 
                          PrimitiveTopology::LineList,
                          vertices.Size() / 2,
                          &(vertices[0]),
                          color, 
						  CoreGraphics::RenderShape::CheckDepth);
    CoreGraphics::ShapeRenderer::Instance()->AddShape(shape);
}

} // namespace Animation