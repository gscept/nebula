using System;
using System.Text;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;
using System.Collections;
using System.Collections.Generic;

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
            public static extern void SetScale(UInt64 entityId, Vector3 position);

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
        }

        public class NebulaApp
        {
            private bool isRunning = false;

            public bool IsRunning { get { return isRunning; } }

            public virtual void OnStart()
            {
                this.isRunning = true;
            }

            public virtual void OnShutdown()
            {
                this.isRunning = false;
            }

            public virtual void OnBeginFrame()
            {
                // TODO: loop over all worlds and collect garbage
                World.Get(World.DEFAULT_WORLD).CollectGarbage();

                PropertyManager.Instance.OnBeginFrame();
            }

            public virtual void OnFrame()
            {
                PropertyManager.Instance.OnFrame();
            }

            public virtual void OnEndFrame()
            {
                PropertyManager.Instance.OnEndFrame();
            }
        }
    }
}
