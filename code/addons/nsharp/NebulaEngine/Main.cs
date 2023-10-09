using System;
using System.Reflection;
using System.Runtime.InteropServices;
using ConsoleHook;
using Mathf;
using Nebula.Game;
using NebulaEngine;

using Api = Nebula.Game.NebulaApiV1;

namespace Nebula
{
    public class Runtime
    {
        #region Singleton data and functions
        private class RuntimeData
        {
            public NebulaApp App = new NebulaApp();
        }
        private static readonly RuntimeData runtimeData = new RuntimeData();
        // Explicit static constructor to tell C# compiler
        // not to mark type as beforefieldinit
        static Runtime() { }
        private Runtime() { }
        #endregion

        #region Runtime management methods

        /// <summary>
        /// Overrides the default nebula app. Call this if you have inherited from NebulaApp to create your own App.
        /// </summary>
        /// <param name="app"></param>
        public static void OverrideNebulaApp(NebulaApp app)
        {
            runtimeData.App = app;
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
            if (currentAssembly != engineAssembly)
            {
                NativeLibrary.SetDllImportResolver(engineAssembly, Nebula.Runtime.ImportResolver);

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
            }

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

        static public void OnBeginFrame()
        {
            if (runtimeData.App.IsRunning)
            {
                runtimeData.App.OnBeginFrame();
            }
        }

        static public void OnFrame()
        {
            if (runtimeData.App.IsRunning)
            {
                runtimeData.App.OnFrame();
            }
        }

        static public void OnEndFrame()
        {
            if (runtimeData.App.IsRunning)
            {
                runtimeData.App.OnEndFrame();
            }
        }

        #endregion

        #region Private methods

        // When using DLLImport, we normally cannot hardcode the application name since we don't have the name of the final binary.
        // This should solve the problem by checking what the current process name is, and then just resolving __Internal to that.
        private static IntPtr ImportResolver(string libraryName, Assembly assembly, DllImportSearchPath? searchPath)
        {
            IntPtr libHandle = IntPtr.Zero;
            string strExeFilePath = System.Diagnostics.Process.GetCurrentProcess().ProcessName;
            if (libraryName == "__Internal")
            {
                if (RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
                {
                    libHandle = NativeLibrary.Load(strExeFilePath + ".exe");
                }
                else if (RuntimeInformation.IsOSPlatform(OSPlatform.Linux))
                {
                    libHandle = NativeLibrary.Load(strExeFilePath);
                }
            }
            return libHandle;
        }

        #endregion
    }
}

namespace NebulaEngine
{
    public class AppEntry
    {
        [UnmanagedCallersOnly]
        static public void Main()
        {
            Nebula.Runtime.Setup(Assembly.GetExecutingAssembly());

            // Setup console redirect / log hook
            using (var consoleWriter = new ConsoleWriter())
            {
                consoleWriter.WriteEvent += ConsoleEvents.WriteFunc;
                consoleWriter.WriteLineEvent += ConsoleEvents.WriteLineFunc;
                Console.SetOut(consoleWriter);
            }

            Nebula.Game.PropertyManager propertyManager = Nebula.Game.PropertyManager.Instance;

#if DEBUG
            Console.WriteLine("[NSharp] NebulaEngine.AppEntry.Main() returned with code 0");
#endif
        }

        [UnmanagedCallersOnly]
        static public void Shutdown()
        {
            Nebula.Runtime.Close();
        }
    }
}
