using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

using Api = Nebula.Game.NebulaApiV1;

namespace Nebula
{
    namespace Game
    {
        /// <summary>
        /// The entry the sync writes into. The field order is the layout the engine writes with, and the
        /// eight byte fields before the four byte ones are what make the entry 64 bytes on both sides
        /// without padding.
        /// </summary>
        [StructLayout(LayoutKind.Sequential)]
        internal struct TimeSourceSnapshot
        {
            internal double frameTime;
            internal double time;
            internal long ticks;
            internal double fixedFrameTime;
            internal double accumulator;
            internal double timeFactor;
            internal uint hash;
            internal int availableSteps;
            internal int maxStepsPerFrame;
            internal int pauseCounter;
        }

        /// <summary>
        /// The mirror of the engine's time sources. The engine has authority over the time, and this holds
        /// what the last sync saw, so a read is a field read rather than a call through the api.
        /// </summary>
        public sealed class TimeManager
        {
            private const int MaxSources = 32;

            private static readonly TimeManager instance = new TimeManager();

            // Explicit static constructor to tell C# compiler
            // not to mark type as beforefieldinit
            static TimeManager()
            {
            }

            private TimeManager()
            {
                this.buffer = new TimeSourceSnapshot[MaxSources];
                this.handle = GCHandle.Alloc(this.buffer, GCHandleType.Pinned);
                this.sources = new Dictionary<uint, TimeSource>();
            }

            /// <summary>
            /// Returns the time manager singleton instance.
            /// </summary>
            public static TimeManager Instance => instance;

            private readonly TimeSourceSnapshot[] buffer;
            private readonly GCHandle handle;
            private readonly Dictionary<uint, TimeSource> sources;
            private float globalTimeFactor;

            /// <summary>
            /// The global time scale. This should be used sparingly; a source's own factor is usually what
            /// you want.
            /// </summary>
            public float GlobalTimeFactor
            {
                get { return this.globalTimeFactor; }
                set
                {
                    Api.SetGlobalTimeFactor(value);
                    this.globalTimeFactor = value;
                }
            }

            public TimeSource System => this.Get(TimeSourceHash.System);
            public TimeSource Gameplay => this.Get(TimeSourceHash.Gameplay);
            public TimeSource Input => this.Get(TimeSourceHash.Input);

            /// <summary>
            /// The source the physics feature runs its fixed event on. Null until the mirror has seen it,
            /// which is the first sync after the physics feature has created it.
            /// </summary>
            public TimeSource Physics => this.Get(TimeSourceHash.Physics);

            /// <summary>
            /// The mirror for a time source, or null when the runtime has not seen one with this hash.
            /// </summary>
            public TimeSource Get(uint hash)
            {
                if (this.sources.TryGetValue(hash, out TimeSource source))
                {
                    return source;
                }

                return null;
            }

            /// <summary>
            /// Create a time source in the engine and mirror it here. Asking for one that already exists
            /// gives the mirror for it.
            /// </summary>
            public TimeSource Create(uint hash, double fixedFrameTime, int maxStepsPerFrame = 5)
            {
                Api.CreateTimeSource(hash, fixedFrameTime, maxStepsPerFrame);

                if (!this.sources.TryGetValue(hash, out TimeSource source))
                {
                    source = new TimeSource(hash);
                    this.sources.Add(hash, source);
                }

                // The engine has not advanced this source yet, so the values here are what it was created with
                source.FixedFrameTime = fixedFrameTime;
                source.MaxStepsPerFrame = maxStepsPerFrame;
                return source;
            }

            /// <summary>
            /// Refresh the mirror from the engine. Called once per frame at the beginning of the frame, after
            /// the engine has advanced the sources, so the values are this frame's and the fixed steps have
            /// not been consumed yet.
            /// </summary>
            public void Sync()
            {
                int count = Api.SyncTimeSources(this.handle.AddrOfPinnedObject(), MaxSources);
                this.globalTimeFactor = Api.GetGlobalTimeFactor();

                for (int i = 0; i < count; i++)
                {
                    TimeSourceSnapshot entry = this.buffer[i];
                    if (!this.sources.TryGetValue(entry.hash, out TimeSource source))
                    {
                        source = new TimeSource(entry.hash);
                        this.sources.Add(entry.hash, source);
                    }

                    source.Update(in entry);
                }
            }
        }
    }
}
