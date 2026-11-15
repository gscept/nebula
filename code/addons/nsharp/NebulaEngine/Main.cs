#define NEBULA_HOTRELOAD

using System;
using System.Reflection;
using System.Runtime.InteropServices;
using ConsoleHook;
using Nebula;
using Nebula.Game;

using Api = Nebula.Game.NebulaApiV1;

namespace Nebula
{
    public static class HotReload
    {
        public static void Request()
        {
            NebulaEngine.AppEntry.RequestManagedReload();
        }
    }

    public class Runtime
    {
        private class RuntimeData
        {
            public NebulaApp App = new NebulaApp();
            public bool EngineSetup;
        }
        private static readonly RuntimeData runtimeData = new RuntimeData();
        // Explicit static constructor to tell C# compiler
        // not to mark type as beforefieldinit
        static Runtime() { }
        private Runtime() { }

        /// <summary>
        /// Overrides the default nebula app. Call this if you have inherited from NebulaApp to create your own App.
        /// </summary>
        /// <param name="app"></param>
        public static void OverrideNebulaApp(NebulaApp app)
        {
            runtimeData.App = app;
        }

        internal static void ResetNebulaApp()
        {
            runtimeData.App = new NebulaApp();
        }

        /// <summary> Call to initialize the Nebula C# runtime </summary>
        /// <param name="currentAssembly"> 
        /// The assembly that is currently executing. You can get this via Assembly.GetExecutingAssembly()
        /// </param>
        public static void Setup(Assembly currentAssembly)
        {
            NativeLibrary.SetDllImportResolver(currentAssembly, Nebula.Runtime.ImportResolver);

            Nebula.Game.ComponentManager componentManager = Nebula.Game.ComponentManager.Instance;

            Assembly engineAssembly = typeof(NebulaEngine.AppEntry).Assembly;
            if (!runtimeData.EngineSetup)
            {
                // Register all native components and tie them to their respective component id in the unmanaged runtime
                foreach (var type in engineAssembly.GetTypes())
                {
                    Type[] interfaces = type.GetInterfaces();
                    for (int i = 0; i < interfaces.Length; i++)
                    {
                        if (interfaces[i] == typeof(NativeComponent))
                        {
                            string name = type.Name;
                            uint id = Api.GetComponentId(name);
                            componentManager.RegisterComponent(type, id);
                        }
                    }
                }
                runtimeData.EngineSetup = true;
            }

            if (currentAssembly != engineAssembly)
            {
                foreach (var type in currentAssembly.GetTypes())
                {
                    Type[] interfaces = type.GetInterfaces();
                    for (int i = 0; i < interfaces.Length; i++)
                    {
                        if (interfaces[i] == typeof(NativeComponent))
                        {
                            string name = type.Name;
                            uint id = Api.GetComponentId(name);
                            componentManager.RegisterComponent(type, id);
                        }
                    }
                }
            }
        }

        /// <summary>
        /// Start the nebula runtime
        /// </summary>
        public static void Start()
        {
            runtimeData.App.OnStart();
        }

        /// <summary>
        /// Shutdown the nebula runtime
        /// </summary>
        public static void Close()
        {
            runtimeData.App.OnShutdown();
        }

        public static void OnBeginFrame()
        {
            if (runtimeData.App.IsRunning)
            {
                runtimeData.App.OnBeginFrame();
            }
        }

        public static void OnFrame()
        {
            if (runtimeData.App.IsRunning)
            {
                runtimeData.App.OnFrame();
            }
        }

        public static void OnEndFrame()
        {
            if (runtimeData.App.IsRunning)
            {
                runtimeData.App.OnEndFrame();
            }
        }

        // When using DLLImport, we normally cannot hardcode the application name since we don't have the name of the final binary.
        // This should solve the problem by checking what the current process name is, and then just resolving __Internal to that.
        private static IntPtr ImportResolver(string libraryName, Assembly assembly, DllImportSearchPath? searchPath)
        {
            IntPtr libHandle = IntPtr.Zero;
            if (libraryName == "__Internal")
            {
                libHandle = NativeLibrary.GetMainProgramHandle();
            }
            return libHandle;
        }
    }
}

namespace NebulaEngine
{
    public class AppEntry
    {
#if NEBULA_HOTRELOAD
        private static HotReloadManager hotReloadManager;
#endif

        [UnmanagedCallersOnly]
        public static int Main(IntPtr pluginPath, int pluginPathLength)
        {
            try
            {
                Nebula.Runtime.Setup(Assembly.GetExecutingAssembly());

                // Setup console redirect / log hook
                var consoleWriter = new ConsoleWriter();
                consoleWriter.WriteEvent += ConsoleEvents.WriteFunc;
                consoleWriter.WriteLineEvent += ConsoleEvents.WriteLineFunc;
                Console.SetOut(consoleWriter);

#if NEBULA_HOTRELOAD
                string path = Marshal.PtrToStringUTF8(pluginPath, pluginPathLength);
                hotReloadManager = new HotReloadManager();
                hotReloadManager.Load(path);
#endif

                return 0;
            }
            catch (Exception exception)
            {
                Console.Error.WriteLine("[NSharp] Failed to load game plugin: " + exception);
                return 1;
            }
        }

        [UnmanagedCallersOnly]
        public static void Exit()
        {
#if NEBULA_HOTRELOAD
            if (hotReloadManager != null)
            {
                hotReloadManager.Unload();
                hotReloadManager = null;
            }
#endif
        }

        [UnmanagedCallersOnly]
        public static int RequestReload(IntPtr pluginPath, int pluginPathLength)
        {
#if NEBULA_HOTRELOAD
            if (hotReloadManager == null)
            {
                return 0;
            }
            string path = Marshal.PtrToStringUTF8(pluginPath, pluginPathLength);
            hotReloadManager.RequestReload(path);
#endif
            return 0;
        }

        internal static void RequestManagedReload()
        {
#if NEBULA_HOTRELOAD
            if (hotReloadManager != null)
            {
                hotReloadManager.RequestReload(true);
            }
#endif
        }

        [UnmanagedCallersOnly]
        public static void OnBeginFrame()
        {
            Nebula.Runtime.OnBeginFrame();
        }

        [UnmanagedCallersOnly]
        public static void OnFrame()
        {
            Nebula.Runtime.OnFrame();
        }

        [UnmanagedCallersOnly]
        public static void OnEndFrame()
        {
            Nebula.Runtime.OnEndFrame();
#if NEBULA_HOTRELOAD
            if (hotReloadManager != null)
            {
                hotReloadManager.ProcessPendingReload();
            }
#endif
        }

        /// <summary>
        /// Called by the script feature unit at the beginning of the frame with the entities created
        /// since the last flush. The buffer is a blittable array of entity ids and is only valid for
        /// the duration of the call, so the mirror has to be populated here.
        /// </summary>
        [UnmanagedCallersOnly]
        public static int FlushEntityCreateQueue(IntPtr ids, int count)
        {
            if (ids == IntPtr.Zero || count <= 0)
            {
                return 0;
            }

            unsafe
            {
                ulong* buffer = (ulong*)ids;
                for (int i = 0; i < count; i++)
                {
                    ulong id = buffer[i];

                    // Game::Entity stores the world id in bits 32-39
                    World world = World.Get((uint)(id >> 32));
                    if (world == null)
                    {
                        continue;
                    }

                    world.RegisterEntityFromNative(id);
                }
            }

            return 0;
        }

        /// <summary>
        /// Called by the script feature unit after the game server has managed its entities, for every
        /// world whose entity count changed since the last sweep.
        /// </summary>
        [UnmanagedCallersOnly]
        public static int CollectGarbage(uint worldId)
        {
            World world = World.Get(worldId);
            if (world == null)
            {
                return 0;
            }

            world.CollectGarbage();
            return 0;
        }
    }
}
