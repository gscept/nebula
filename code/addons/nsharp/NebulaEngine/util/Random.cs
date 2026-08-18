using Nebula;

namespace Util
{
    public static class Random
    {
        // Arbitrary non-zero initial state.
        private static uint s0 = 0xA3C59AC3u;
        private static uint s1 = 0x3C6EF372u;
        private static uint s2 = 0xDAA66D2Bu;
        private static uint s3 = 0x78DDE6E4u;

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        private static uint Next()
        {
            // xoshiro128++
            uint result = RotateLeft(s0 + s3, 7) + s0;

            uint t = s1 << 9;

            s2 ^= s0;
            s3 ^= s1;
            s1 ^= s2;
            s0 ^= s3;

            s2 ^= t;
            s3 = RotateLeft(s3, 11);

            return result;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        private static uint RotateLeft(uint x, int k)
            => (x << k) | (x >> (32 - k));

        //--------------------------------------------------------------------------
        /** <summary>
            Select a specific seed for the RNG
            </summary>
        */
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void Seed(ulong seed)
        {
            s0 = SplitMix32(ref seed);
            s1 = SplitMix32(ref seed);
            s2 = SplitMix32(ref seed);
            s3 = SplitMix32(ref seed);
        }

        //--------------------------------------------------------------------------
        /** <summary>
            Randomize the seed for the RNG
            </summary>
        */
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void RandomizeSeed()
        {
            System.Random random = new System.Random();
            ulong seed = random.NextInt64;
            Seed(seed);
        }
        
        //--------------------------------------------------------------------------
        /** <summary>
            Random float in [min, max).
            </summary>
        */
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static float Range(float min, float max)
        {
            // Top 24 bits -> exactly representable float fraction [0, 1).
            float t = (Next() >> 8) * (1.0f / 16777216.0f);
            return min + (max - min) * t;
        }

        //--------------------------------------------------------------------------
        /** <summary>
            Random integer in [min, max).
            </summary>
        */
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static int Range(int min, int max)
        {
            uint range = (uint)(max - min);
            return min + (int)(((ulong)Next() * range) >> 32);
        }

        //--------------------------------------------------------------------------
        /** <summary>
            Random float in [0, 1).
            </summary>
        */
        public static float Value
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => (Next() >> 8) * (1.0f / 16777216.0f);
        }

        //--------------------------------------------------------------------------
        /** <summary>
            Random vector3 point inside unit sphere.
            </summary>
            <remarks>
            Note that this produces a non uniform distribution.
            </remarks>
        */
        public static Vector3 InUnitSphere()
        {
            float azimuth = Range(0, Mathf.Pi * 2.0f);
            float zenith = Range(0, Mathf.Pi);
            float distance = Value();

            float x = distance * Mathf.Sin(zenith) * Mathf.Cos(azimuth);
            float y = distance * Mathf.Sin(zenith) * Mathf.Sin(azimuth);
            float z = distance * Mathf.Cos(zenith);

            return new Vector3(x, y, z);
        }

        //--------------------------------------------------------------------------
        /** <summary>
            Returns a smooth "random" number based on seed.
            Random value will be between approximately -1.0f and 1.0f (not exact!).
            If seed is increased, the random number will smoothly "slide" towards a new "point".
            </summary>
        */
        public static float Smooth(float seed)
        {
            const float factor = -0.161f;

            const float alphaFactor = -3.2f;
            const float alphaScale = -1.3f;

            const float eFactor = -1.2f;
            const float eScale = -1.7f;

            const float piFactor = 1.9f;
            const float piScale = 0.7f;

            return factor * (alphaFactor * Mathf.Sin(alphaScale * seed) +
                             eFactor * Mathf.Sin(eScale * seed * Mathf.E) +
                             piFactor * Mathf.Sin(piScale * seed * Mathf.Pi)
                            );
        }
    }
}
