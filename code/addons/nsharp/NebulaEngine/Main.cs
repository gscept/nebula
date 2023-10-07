using System;
using System.Reflection;
using System.Runtime.InteropServices;
using ConsoleHook;
using Mathf;
using NebulaEngine;

namespace Nebula
{
    public class Runtime
    {
        /// <summary>
        /// Call to initialize the Nebula C# runtime
        /// </summary>
        public static void Setup()
        {
            NativeLibrary.SetDllImportResolver(Assembly.GetExecutingAssembly(), Nebula.Runtime.ImportResolver);

            if (Assembly.GetExecutingAssembly() != typeof(NebulaEngine.AppEntry).Assembly)
            {
                NativeLibrary.SetDllImportResolver(typeof(NebulaEngine.AppEntry).Assembly, Nebula.Runtime.ImportResolver);
            }
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
    }
}

namespace NebulaEngine
{
    public class AppEntry
    {
        [UnmanagedCallersOnly]
        static public void Main()
        {
            Nebula.Runtime.Setup();

            // Setup console redirect / log hook
            using (var consoleWriter = new ConsoleWriter())
            {
                consoleWriter.WriteEvent += ConsoleEvents.WriteFunc;
                consoleWriter.WriteLineEvent += ConsoleEvents.WriteLineFunc;
                Console.SetOut(consoleWriter);
            }
            
            Nebula.Game.PropertyManager propertyManager = Nebula.Game.PropertyManager.Instance;
            propertyManager.RegisterProperty(new Nebula.Game.Property());

#if DEBUG
            Console.WriteLine("[NSharp] NebulaEngine.AppEntry.Main() returned with code 0");
#endif
        }
    }
}
