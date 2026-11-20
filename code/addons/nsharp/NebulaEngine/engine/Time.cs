namespace Nebula
{
    namespace Game
    {
        /// <summary>
        /// The time the game logic runs on. This is the convenience side of the mirror for the source a
        /// plugin normally wants, and it is the same values TimeManager exposes for it.
        /// </summary>
        public static class Time
        {
            /// <summary>
            /// The game logic source's delta for this frame. Game code works in float, so the mirror's double
            /// is narrowed here.
            /// </summary>
            public static float DeltaTime
            {
                get
                {
                    TimeSource source = TimeManager.Instance.Gameplay;
                    return source != null ? (float)source.FrameTime : 0.0f;
                }
            }
        }
    }
}
