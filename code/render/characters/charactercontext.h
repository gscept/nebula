#pragma once
//------------------------------------------------------------------------------
/**
    @class Characters::CharacterContext

    Assumes control over the character animation functionalities
    of a model if it contains a character definition and an animation set.

    Internally, the character context is responsible for dispatching animation jobs,
    blending animation, and keeping track of the current time playing for each registered entity.

    The character context also handles animation events, which are triggers to play when animation
    hits a certain time stamp.

    How the context works is like this:
        Every character has a buffer of samples which is updated every frame.
        Every character has a set of tracks to play animations on, which can be blended
        between if requested. Tracks are blended in sequential order.
        Every character has a list per track of pending animations, which are
        ticked off whenever the current animation has finished.
        Animations can be played without enqueueing, which replaces the currently
        playing animation on that track.


    @copyright
    (C) 2018-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "graphics/graphicscontext.h"
#include "resources/resourceid.h"
#include "io/path.h"
#include "coreanimation/animclip.h"
#include "coreanimation/animsamplemask.h"
#include "characters/skeleton.h"
#include "coreanimation/animation.h"
#include "coreanimation/animsamplebuffer.h"
#include "characters/skeletonjoint.h"
#include "jobs/jobs.h"

namespace CoreAnimation
{

extern void AnimSampleJob(SizeT totalJobs, SizeT groupSize, IndexT groupIndex, SizeT invocationOffset, void* ctx);
extern void AnimSampleJobWithMix(SizeT totalJobs, SizeT groupSize, IndexT groupIndex, SizeT invocationOffset, void* ctx);

}

namespace Characters
{

enum EnqueueMode
{
    Append,             // adds clip to the queue to play after current on the track
    Replace,            // replace current clip on the track
    IgnoreIfSame        // ignore enqueue if same clip is playing
};

class CharacterContext : public Graphics::GraphicsContext
{
    __DeclareContext();
public:

    /// an event a clip played on a track passed
    struct FiredEvent
    {
        CoreAnimation::AnimEvent event;
        IndexT clip;
        IndexT track;
    };

    /// constructor
    CharacterContext();
    /// destructor
    virtual ~CharacterContext();

    /// setup character context
    static void Create();

    /// setup character context, assumes there is already a model attached, by loading a skeleton and animation resource
    static void Setup(
        const Graphics::GraphicsEntityId id
        , const Resources::ResourceName& skeletonResource
        , const IndexT skeletonIndex
        , const Resources::ResourceName& animationResource
        , const IndexT animationIndex
        , const Util::StringAtom& tag
        , bool supportBlending = true
    );

    /// if the entity's model is skinned, register and setup the entity as a character using the skeleton and animation packaged
    /// with the model (folder:file:ske and folder:file:ani), and play its first clip. Call when the model is loaded and the entity
    /// is observable. Returns false if the model isn't skinned or doesn't have both
    static bool SetupFromModel(const Graphics::GraphicsEntityId id, const IO::Path& model, const Util::StringAtom& tag);

    /// perform a clip name lookup
    static IndexT GetClipIndex(const Graphics::GraphicsEntityId id, const Util::StringAtom& name);

    /// play animation on a specific track
    static bool PlayClip(
        const Graphics::GraphicsEntityId id,
        const CoreAnimation::AnimSampleMask* mask,
        const IndexT clipIndex,
        const IndexT track,
        const EnqueueMode mode,
        const float weight = 1.0f,
        const SizeT loopCount = 1,
        const float startTime = 0.0f,
        const float fadeIn = 0.0f,
        const float fadeOut = 0.0f,
        const float timeOffset = 0.0f,
        const float timeFactor = 1.0f);
    /// stops all animations queued on track (and resets their time)
    static void StopTrack(const Graphics::GraphicsEntityId id, const IndexT track);
    /// stops all animations on all tracks
    static void StopAllTracks(const Graphics::GraphicsEntityId id);
    /// pause animation on track, which doesn't reset time
    static void PauseTrack(const Graphics::GraphicsEntityId id, const IndexT track);
    /// seek animation to a certain time, clamps to length
    static void Seek(const Graphics::GraphicsEntityId id, const float time);

    /// get current time of animation
    static const float GetTime(const Graphics::GraphicsEntityId id);
    /// check if any animation is playing
    static bool IsPlaying(const Graphics::GraphicsEntityId id);

    /// set weight for track
    static void SetTrackWeight(const Graphics::GraphicsEntityId id, const IndexT track, const float weight);
    /// set time factor for track
    static void SetTrackTimeFactor(const Graphics::GraphicsEntityId id, const IndexT track, const float factor);

    /// retrieve animation clip names and lengths
    static void QueryClips(const Graphics::GraphicsEntityId id, Util::FixedArray<CoreAnimation::AnimClip>& outClips);
    /// get the skeleton, invalid until it's loaded
    static const SkeletonId GetSkeleton(const Graphics::GraphicsEntityId id);
    /// get the animation, invalid until it's loaded
    static const CoreAnimation::AnimationId GetAnimation(const Graphics::GraphicsEntityId id);
    /// get the model space transforms of the joints in the last evaluated pose, one per joint of the skeleton
    static const Util::FixedArray<Math::mat4>& GetJointTransforms(const Graphics::GraphicsEntityId id);
    /// get the clip playing on a track, InvalidIndex if none is
    static const IndexT GetPlayingClip(const Graphics::GraphicsEntityId id, const IndexT track);
    /// check if the clip playing on a track is paused
    static bool IsTrackPaused(const Graphics::GraphicsEntityId id, const IndexT track);
    /// get the time in the clip playing on a track in seconds, within its duration for looped clips, 0 if none is playing
    static Timing::Time GetTrackTime(const Graphics::GraphicsEntityId id, const IndexT track);
    /// move the clip playing on a track to a time in it in seconds, it plays on from there, a paused one shows the pose at that time.
    /// The events between the time it was at and the time it's moved to aren't fired
    static void SeekTrack(const Graphics::GraphicsEntityId id, const IndexT track, const Timing::Time time);
    /// get the events the clips played on the tracks passed in the last update that finished, in the order of the tracks.
    /// A clip fires the events at its start when it starts, and every time it passes them while looping, a clip that
    /// stops fires the ones up to its end. Paused clips fire none
    static const Util::Array<FiredEvent>& GetFiredEvents(const Graphics::GraphicsEntityId id);

    /// runs before frame is updated
    static void UpdateAnimations(const Graphics::FrameContext& ctx);
    /// run after frame
    static void WaitForCharacterJobs(const Graphics::FrameContext& ctx);

    /// register anim sample mask, and return pointer
    static CoreAnimation::AnimSampleMask* CreateAnimSampleMask(const Util::StringAtom& name, const Util::FixedArray<Math::scalar>& weights);
    /// get anim sample mask by name
    static CoreAnimation::AnimSampleMask* GetAnimSampleMask(const Util::StringAtom& name);

#ifndef PUBLIC_DEBUG    
    /// debug rendering
    static void OnRenderDebug(uint32_t flags);
#endif

    enum LoadState
    {
        NoneLoaded = 0,
        SkeletonLoaded = N_BIT(1),
        AnimationLoaded = N_BIT(2),
    };

    static Threading::Interlocked::AtomicCounter ConstantUpdateCounter;
private:
    friend struct CharacterJobContext;

    struct AnimationRuntime
    {
        IndexT clip;
        EnqueueMode enqueueMode;
        Timing::Tick baseTime, startTime;
        Timing::Tick duration;
        Timing::Tick fadeInTime, fadeOutTime;
        Timing::Tick evalTime, prevEvalTime;
        Timing::Tick sampleTime, prevSampleTime;
        Timing::Tick timeOffset;
        Util::Array<uint> curveSampleIndices;
        const CoreAnimation::AnimSampleMask* mask;
        float timeFactor;
        float blend;
        bool paused;
        /// the base time is set once the clip starts playing, a clip replacing the playing one starts on the next update
        bool started;
#if NEBULA_DEBUG
        Util::String name;
#endif
    };

    friend const bool IsExpired(const CharacterContext::AnimationRuntime& runtime, const Timing::Time time);
    friend const bool IsInfinite(const CharacterContext::AnimationRuntime& runtime);
    friend Timing::Tick GetAbsoluteStopTime(const CharacterContext::AnimationRuntime& runtime);
    friend void EvalCharacter(SizeT totalJobs, SizeT groupSize, IndexT groupIndex, SizeT invocationOffset, void* ctx);

    static const SizeT MaxNumTracks = 16;
    struct AnimationTracks
    {
        Util::StackArray<AnimationRuntime, 8>   pendingAnimations[MaxNumTracks]; // max 16 tracks
        AnimationRuntime                        playingAnimations[MaxNumTracks]; // max 16 tracks
    };

    enum
    {
        Skeleton,
        Animation,
        Loaded,
        TrackController,
        AnimTime,
        JointPalette,
        JointPaletteScaled,
        UserControlledJoint,
        JobJoints,
        SampleBuffer,
        SupportMix,
        EntityId,
        CharacterSkinNodeIndexOffset,
        AnimationResource,
        AnimationIndex,
        FiredEvents,
        PendingFiredEvents
    };

    typedef Ids::IdAllocator<
        SkeletonId,
        CoreAnimation::AnimationId,
        LoadState,
        AnimationTracks,
        Timing::Time,
        Util::FixedArray<Math::mat4>,
        Util::FixedArray<Math::mat4>,
        Util::FixedArray<Math::mat4>,
        Util::FixedArray<SkeletonJobJoint>,
        CoreAnimation::AnimSampleBuffer,
        bool,
        Graphics::GraphicsEntityId,
        IndexT,
        Resources::ResourceId,
        IndexT,
        Util::Array<FiredEvent>,
        Util::Array<FiredEvent>
    > CharacterContextAllocator;
    static CharacterContextAllocator characterContextAllocator;

    /// allocate a new slice for this context
    static Graphics::ContextEntityId Alloc();
    /// deallocate a slice
    static void Dealloc(Graphics::ContextEntityId id);

    static Util::HashTable<Util::StringAtom, CoreAnimation::AnimSampleMask> masks;
    static Threading::Event totalCompletionEvent;
};

__ImplementEnumBitOperators(CharacterContext::LoadState);

//------------------------------------------------------------------------------
/**
*/
inline Graphics::ContextEntityId
CharacterContext::Alloc()
{
    return characterContextAllocator.Alloc();
}

//------------------------------------------------------------------------------
/**
*/
inline void
CharacterContext::Dealloc(Graphics::ContextEntityId id)
{
    characterContextAllocator.Dealloc(id.id);
}

} // namespace Characters
