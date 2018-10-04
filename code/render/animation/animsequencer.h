#pragma once
//------------------------------------------------------------------------------
/**
    @class Animation::AnimSequencer
  
    An AnimSequencer object arranges AnimJobs along the time line to
    produce a single, priority-blended result. AnimJobs which are overlapping
    on the time-line will be blended by the following rules:

    - AnimJobs with a higher blend priority dominate lower-priority anim jobs
    - if AnimJobs have the same blend priority, the start time of the anim
      job is used to determine blend priority (jobs which start later dominate
      jobs which start earlier)

    (C) 2008 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/    
#include "core/refcounted.h"
#include "animation/animjob.h"
#include "animation/animeventinfo.h"
#include "jobs/jobport.h"
#include "coregraphics/rendershape.h"

//------------------------------------------------------------------------------
namespace Animation
{
class AnimSequencer
{
public:
    /// constructor
    AnimSequencer();
    /// destructor
    ~AnimSequencer();

    /// setup the animation controller
    void Setup(const Ptr<CoreAnimation::AnimResource>& animResource);
    /// discard the anim sequencer
    void Discard();
    /// return true if between Setup/Discard
    bool IsValid() const;
    /// enable/disable the debug hud
    void SetDebugHudEnabled(bool b);
    /// get debug hud enabled state
    bool IsDebugHudEnabled() const;

    /// enqueue an anim job
    void EnqueueAnimJob(const Ptr<AnimJob>& animJob);
    /// stop all anim jobs on given track
    void StopTrack(IndexT trackIndex, bool allowFadeOut=true);
    /// stop all animations on all tracks
    void StopAllTracks(bool allowFadeOut=true);
	/// pauses all anim jobs on a given track
	void PauseTrack(IndexT trackIndex, bool pause);
	/// pauses all animations on all tracks
	void PauseAllTracks(bool pause);

	/// sets time for all tracks in sequencer
	void SetTime(Timing::Tick time);

    /// update the animation sequencer time
    void UpdateTime(Timing::Tick time);
    /// start asynchronous animation update, returns false if nothing had to be done
    bool StartAsyncEvaluation(const Ptr<Jobs::JobPort>& jobPort);

    /// get the currently set time
    Timing::Tick GetTime() const;
    /// get the final sampled result of the last evaluation
    const Ptr<CoreAnimation::AnimSampleBuffer>& GetResult() const;
    /// get pointer to animation resource object
    const Ptr<CoreAnimation::AnimResource>& GetAnimResource() const;
    /// get all anim jobs
    Util::Array<Ptr<AnimJob> > GetAllAnimJobs() const;
    /// get currently active anim jobs filtered by track index
    Util::Array<Ptr<AnimJob> > GetAnimJobsByTrackIndex(IndexT trackIndex) const;
    /// get anim jobs by name
    Util::Array<Ptr<AnimJob> > GetAnimJobsByName(const Util::StringAtom& name) const;

    /// FIXME FIXME FIXME: emit anim event infos
    Util::Array<AnimEventInfo> EmitAnimEvents(Timing::Tick startTime, Timing::Tick endTime, bool justDominatingJob, const Util::String& optionalCategory = "") const;

private:
    /// stop a single anim job allowing fade-out (private helper method)
    void EnqueueAnimJobForStopping(const Ptr<AnimJob>& animJob);
    /// discard a single anim job immediately from the sequencer
    void DiscardAnimJob(Ptr<AnimJob> animJob);
    /// insert new anim jobs (called from UpdateTime)
    void InsertEnqueuedAnimJobs(Timing::Tick time);
    /// update stopped anim jobs (called from UpdateTime)
    void UpdateStoppedAnimJobs(Timing::Tick time);
    /// delete expired anim jobs
    void RemoveExpiredAnimJobs(Timing::Tick time);
    /// update active animjobs times
    void UpdateTimeActiveAnimJobs(Timing::Tick time);

    /// FIXME FIXME FIXME: helper method, for determining dominating job
    IndexT FindDominatingAnimJobIndex(Timing::Tick startTime, Timing::Tick endTime) const;

    #if NEBULA_ANIMATIONSYSTEM_FRAMEDUMP
    /// dump debug information for the current frame
    void DumpFrameDebugInfo(Timing::Tick time);
    #endif
    /// render a graphical debug hud 
    void DebugRenderHud(Timing::Tick time);
    /// generate a debug visualization rectangle for an anim job
    void DebugGenAnimJobRectangle(Timing::Tick timeOrigin, const Ptr<AnimJob>& animJob, Util::Array<CoreGraphics::RenderShape::RenderShapeVertex>& outVertices);
    /// compute a screen coordinate (0..1) for a time and track index
    CoreGraphics::RenderShape::RenderShapeVertex DebugViewSpaceCoord(Timing::Tick timeOrigin, Timing::Tick time, IndexT trackIndex);
    /// debug visualization: render a line list (FIXME: something like this should be offered by the debug rendering system directly)
    void DebugRenderLineList(const Util::Array<CoreGraphics::RenderShape::RenderShapeVertex>& vertices, const Math::float4& color);
    /// debug visualization: render the anim job text
    void DebugRenderAnimJobText(Timing::Tick timeOrigin, const Ptr<AnimJob>& animJob, const Math::float4& color);
    /// debug visualization: generate the lines for the track list
    void DebugGenTrackLines(SizeT maxTracks, Util::Array<CoreGraphics::RenderShape::RenderShapeVertex>& outVertices);

    Timing::Tick time;
    Ptr<CoreAnimation::AnimResource> animResource;
    Ptr<CoreAnimation::AnimSampleBuffer> dstSampleBuffer;

    Util::Array<Ptr<AnimJob> > enqueuedAnimJobs;     // anim jobs started this "frame"
    Util::Array<Ptr<AnimJob> > stoppedAnimJobs;      // anim jobs stopped this "frame"
    Util::Array<Ptr<AnimJob> > animJobs;             // currently sequenced anim jobs

    Util::Array<Ptr<Jobs::Job> > jobChain;      // need to keep jobs around until they are finished!
    bool debugHudEnabled;
};

//------------------------------------------------------------------------------
/**
*/
inline Timing::Tick
AnimSequencer::GetTime() const
{
    return this->time;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
AnimSequencer::IsValid() const
{
    return this->animResource.isvalid();
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<CoreAnimation::AnimSampleBuffer>&
AnimSequencer::GetResult() const
{
    return this->dstSampleBuffer;
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<CoreAnimation::AnimResource>&
AnimSequencer::GetAnimResource() const
{
    return this->animResource;
}

//------------------------------------------------------------------------------
/**
*/
inline void
AnimSequencer::SetDebugHudEnabled(bool b)
{
    this->debugHudEnabled = b;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
AnimSequencer::IsDebugHudEnabled() const
{
    return this->debugHudEnabled;
}

} // namespace Animation
//------------------------------------------------------------------------------
