using System;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;
using System.Collections;
using System.Collections.Generic;
using Mathf;


/*
Entities are only ids as per usual, pointing into their respective tables in unmanaged code
Components in managed code are objects that are allocated linearly per type by some object pool manager.
 */

namespace Nebula
{
    public class Debug
    {
        [DllImport("__Internal", EntryPoint = "N_Print")]
        public static extern void Log(string val);

        [DllImport("__Internal", EntryPoint = "N_Assert")]
        public static extern void Assert(bool value);

        [DllImport("__Internal", EntryPoint = "N_Assert")]
        public static extern void Assert(bool value, string message);
    }

    namespace Game
    {
        public interface INebulaApi { };

        public class NebulaApiV1 : INebulaApi
        {
            [DllImport("__Internal", EntryPoint = "EntityCreateFromTemplate")]
            public static extern uint CreateEntity(uint worldId, string template);

            [DllImport("__Internal", EntryPoint = "EntityIsValid")]
            public static extern bool IsValid(uint worldId, uint entityId);

            [DllImport("__Internal", EntryPoint = "EntityDelete")]
            public static extern void DeleteEntity(uint worldId, uint entityId);

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
                PropertyManager.Instance.OnBeginFrame();
            }

            public virtual void OnFrame()
            {

            }

            public virtual void OnEndFrame()
            {

            }
        }
    }
}
