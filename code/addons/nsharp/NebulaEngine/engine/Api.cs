using System;
using System.Text;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;

/*
Entities are only ids as per usual, pointing into their respective tables in unmanaged code
Components in managed code are objects that are allocated linearly per type by some object pool manager.
 */

namespace Nebula
{
    public class Debug
    {
        [DllImport("__Internal", EntryPoint = "N_Print")]
        private static extern void Print(IntPtr data, int isStdout);

        public static unsafe void Log(string val)
        {
            int byteCount = Encoding.UTF8.GetByteCount(val);
            Span<byte> buffer = stackalloc byte[byteCount + 1];
            byteCount = Encoding.UTF8.GetBytes(val.AsSpan(), buffer);
            buffer[byteCount] = 0;
            fixed (byte* ptr = &buffer[0])
            {
                Print((IntPtr)ptr, 1);
            }
        }

        public static void Log<T>(T value)
        {
            Log(value?.ToString() ?? "null");
        }

        [DllImport("__Internal", EntryPoint = "N_Assert")]
        public static extern void Assert([MarshalAs(UnmanagedType.I1)] bool value);

        public static void Assert(bool value, string message)
        {
            Assert(value);
        }
    }

    namespace Game
    {
        public interface INebulaApi { };

        public class NebulaApiV1 : INebulaApi
        {
            [DllImport("__Internal", EntryPoint = "EntityCreate")]
            public static extern UInt64 CreateEntity(uint worldId);

            [DllImport("__Internal", EntryPoint = "EntityIsValid")]
            [return: MarshalAs(UnmanagedType.I1)]
            public static extern bool IsValid(UInt64 entityId);

            [DllImport("__Internal", EntryPoint = "EntityHasInstance")]
            [return: MarshalAs(UnmanagedType.I1)]
            public static extern bool HasInstance(UInt64 entityId);

            [DllImport("__Internal", EntryPoint = "EntityDelete")]
            public static extern void DeleteEntity(UInt64 entityId);

            [DllImport("__Internal", EntryPoint = "EntityGetPosition")]
            public static extern Vector3 GetPosition(UInt64 entityId);

            [DllImport("__Internal", EntryPoint = "EntitySetPosition")]
            public static extern void SetPosition(UInt64 entityId, Vector3 position);

            [DllImport("__Internal", EntryPoint = "EntityGetOrientation")]
            public static extern Quaternion GetOrientation(UInt64 entityId);

            [DllImport("__Internal", EntryPoint = "EntitySetOrientation")]
            public static extern void SetOrientation(UInt64 entityId, Quaternion orientation);

            [DllImport("__Internal", EntryPoint = "EntityGetScale")]
            public static extern Vector3 GetScale(UInt64 entityId);

            [DllImport("__Internal", EntryPoint = "EntitySetScale")]
            public static extern void SetScale(UInt64 entityId, Vector3 scale);

            [DllImport("__Internal", EntryPoint = "EntityHasComponent")]
            [return: MarshalAs(UnmanagedType.I1)]
            public static extern bool HasComponent(UInt64 entityId, uint componentId);

            [DllImport("__Internal", EntryPoint = "ComponentGetIdUtf8")]
            private static extern uint GetComponentIdUtf8(IntPtr name, uint length);

            public static unsafe uint GetComponentId(string name)
            {
                int byteCount = Encoding.UTF8.GetByteCount(name);
                Span<byte> buffer = stackalloc byte[byteCount + 1];
                byteCount = Encoding.UTF8.GetBytes(name.AsSpan(), buffer);
                fixed (byte* ptr = &buffer[0])
                {
                    return GetComponentIdUtf8((IntPtr)ptr, (uint)byteCount);
                }
            }

            [DllImport("__Internal", EntryPoint = "ComponentGetData")]
            public static extern void GetComponentData(UInt64 entityId, uint componentId, IntPtr data, int dataSize);
            
            [DllImport("__Internal", EntryPoint = "ComponentSetData")]
            public static extern void SetComponentData(UInt64 entityId, uint componentId, IntPtr data, int dataSize);

            [DllImport("__Internal", EntryPoint = "WorldGetDefaultWorldId")]
            public static extern uint GetDefaultWorldId();

            [DllImport("__Internal", EntryPoint = "WorldHasWorld")]
            [return: MarshalAs(UnmanagedType.I1)]
            public static extern bool HasWorld(uint worldId);

            [DllImport("__Internal", EntryPoint = "WorldIsSimulating")]
            [return: MarshalAs(UnmanagedType.I1)]
            public static extern bool IsSimulating(uint worldId);

            [DllImport("__Internal", EntryPoint = "TimeSourceSync")]
            public static extern int SyncTimeSources(IntPtr buffer, int maxEntries);

            [DllImport("__Internal", EntryPoint = "TimeSourceSetTimeFactor")]
            public static extern void SetTimeSourceTimeFactor(uint hash, float factor);

            [DllImport("__Internal", EntryPoint = "TimeSourceSetPauseCounter")]
            public static extern void SetTimeSourcePauseCounter(uint hash, int counter);

            [DllImport("__Internal", EntryPoint = "TimeSourceCreate")]
            public static extern uint CreateTimeSource(uint hash, double fixedFrameTime, int maxStepsPerFrame);

            [DllImport("__Internal", EntryPoint = "TimeSetGlobalTimeFactor")]
            public static extern void SetGlobalTimeFactor(float factor);

            [DllImport("__Internal", EntryPoint = "TimeGetGlobalTimeFactor")]
            public static extern float GetGlobalTimeFactor();
        }

        public class NebulaApp
        {
            private bool isRunning = false;

            public bool IsRunning => this.isRunning;

            public virtual void OnStart()
            {
                this.isRunning = true;
            }

            public virtual void OnShutdown()
            {
                this.isRunning = false;
            }

            internal void Resume()
            {
                this.isRunning = true;
            }

            public virtual void OnBeginFrame()
            {
                // The engine has already advanced the time sources for this frame, so the mirror reads this
                // frame's values and the fixed steps are still all there
                TimeManager.Instance.Sync();

                // The events for this frame have been pumped by the render feature unit, which is attached
                // before the script feature unit, so the input mirror reads this frame's state
                Input.Sync();

                foreach (World world in World.AllWorlds())
                {
                    world.CollectGarbage();
                }

                foreach (World world in World.AllWorlds())
                {
                    world.RefreshMode();
                }
                
                foreach (World world in World.AllWorlds())
                {
                    world.UpdateProperties();
                }

                PropertyManager.Instance.OnBeginFrame();
            }

            public virtual void OnFrame()
            {
                PropertyManager.Instance.OnFixedFrame();
                PropertyManager.Instance.OnFrame();
            }

            /// <summary>
            /// The managed physics phase. Called from a callback inside the physics frame event, after the
            /// step, which is the point where the physics engine allows a body to be read.
            /// </summary>
            public virtual void OnPhysicsFrame()
            {
                PropertyManager.Instance.OnPhysicsFrame();
            }

            public virtual void OnEndFrame()
            {
                PropertyManager.Instance.OnEndFrame();
            }
        }
    }
}
