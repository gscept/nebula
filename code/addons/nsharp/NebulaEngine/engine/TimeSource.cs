using System;

using Api = Nebula.Game.NebulaApiV1;

namespace Nebula
{
    namespace Game
    {
        /// <summary>
        /// The fourcc hashes of the time sources the application layer provides.
        /// </summary>
        public static class TimeSourceHash
        {
            public static readonly uint System = FourCC("SYTS");
            public static readonly uint Gameplay = FourCC("GPTS");
            public static readonly uint Input = FourCC("IPTS");

            /// <summary>
            /// A fourcc as the engine writes one. The characters are read in order, which is what a multi
            /// character character constant gives on the platforms the engine builds on.
            /// </summary>
            public static uint FourCC(string fourcc)
            {
                return (uint)((fourcc[0] << 24) | (fourcc[1] << 16) | (fourcc[2] << 8) | fourcc[3]);
            }
        }

        /// <summary>
        /// A time source. The values are the mirror's copy of the native source, refreshed once per frame at
        /// the beginning of the frame, so reading them is a field read rather than a call through the api.
        /// </summary>
        /// <remarks>
        /// A source appears in the mirror after the first sync, and the engine has authority over the time.
        /// </remarks>
        public class TimeSource
        {
            private readonly uint hash;
            private float timeFactor;
            private int pauseCounter;

            // Not part of the public interface. It is stored so the property manager can run the fixed
            // events for the steps this frame has available
            internal int AvailableSteps { get; private set; }

            public uint Hash => this.hash;
            public double FrameTime { get; internal set; }
            public double Time { get; internal set; }
            public long Ticks { get; internal set; }
            public double FixedFrameTime { get; internal set; }
            public double Accumulator { get; internal set; }
            public int MaxStepsPerFrame { get; internal set; }

            /// <summary>
            /// How fast this source runs relative to the frame time. The write goes through the api and the
            /// mirror keeps the same value, so a read in the same frame agrees with what was just written.
            /// </summary>
            public float TimeFactor
            {
                get { return this.timeFactor; }
                set
                {
                    Api.SetTimeSourceTimeFactor(this.hash, value);
                    this.timeFactor = value;
                }
            }

            /// <summary>
            /// Increment to pause this source, decrement to let it run again. A paused source does not
            /// advance and has no steps available.
            /// </summary>
            public int PauseCounter
            {
                get { return this.pauseCounter; }
                set
                {
                    Api.SetTimeSourcePauseCounter(this.hash, value);
                    this.pauseCounter = value;
                }
            }

            public bool IsPaused => this.pauseCounter > 0;

            public void Pause()
            {
                this.PauseCounter = this.pauseCounter + 1;
            }

            public void Resume()
            {
                this.PauseCounter = this.pauseCounter - 1;
            }

            internal TimeSource(uint hash)
            {
                this.hash = hash;
            }

            internal void Update(in TimeSourceSnapshot entry)
            {
                this.FrameTime = entry.frameTime;
                this.Time = entry.time;
                this.Ticks = entry.ticks;
                this.FixedFrameTime = entry.fixedFrameTime;
                this.Accumulator = entry.accumulator;
                this.MaxStepsPerFrame = entry.maxStepsPerFrame;
                this.AvailableSteps = entry.availableSteps;
                this.timeFactor = (float)entry.timeFactor;
                this.pauseCounter = entry.pauseCounter;
            }
        }
    }
}
