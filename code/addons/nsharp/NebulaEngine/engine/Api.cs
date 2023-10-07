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
        class NebulaApiV1
        {
            // ...
        }

        class NebulaApp
        {
            void OnStart(NebulaApiV1 api)
            {
                //PropertyManager.Register<AudioEmitterProperty>();

                // api.DoSomething();???
            }

            void OnShutdown()
            {
                // Might be able to automate deregistering, if we keep track of the registered proeprties per api object passed  to the dll.
                // PropertyManager.Deregister<AudioEmitterProperty>();
            }
        }
    }
}
