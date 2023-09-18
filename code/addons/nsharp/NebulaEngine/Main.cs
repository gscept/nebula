using System;
using ConsoleHook;
using Mathf;

namespace Nebula
{
    public class AppEntry
    {
        static public void Main()
        {
            // Setup console redirect / log hook
            using (var consoleWriter = new ConsoleWriter())
            {
                consoleWriter.WriteEvent += ConsoleEvents.WriteFunc;
                consoleWriter.WriteLineEvent += ConsoleEvents.WriteLineFunc;
                Console.SetOut(consoleWriter);
            }

#if DEBUG
            Console.WriteLine("[NSharp] Nebula.AppEntry.Main() returned with code 0");
#endif
        }
    }
}
