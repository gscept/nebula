using System;
using System.Reflection;
using System.Runtime.InteropServices;
using ConsoleHook;
using Mathf;
using Nebula.Game;
using NebulaEngine;

namespace Nebula
{
    public class Runtime
    {
        private class RuntimeData
        {
            public NebulaApp App = new NebulaApp();
        }
        private static readonly RuntimeData runtimeData = new RuntimeData();

        // Explicit static constructor to tell C# compiler
        // not to mark type as beforefieldinit
        static Runtime()
        {

        }
        private Runtime() { }

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

            if (currentAssembly != typeof(NebulaEngine.AppEntry).Assembly)
            {
                NativeLibrary.SetDllImportResolver(typeof(NebulaEngine.AppEntry).Assembly, Nebula.Runtime.ImportResolver);
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
