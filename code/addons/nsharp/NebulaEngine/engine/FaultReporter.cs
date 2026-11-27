using System;
using System.Collections.Generic;

namespace Nebula
{
    /// <summary>
    /// The single place a plugin failure is reported.
    /// </summary>
    /// <remarks>
    /// A managed exception cannot cross the [UnmanagedCallersOnly] boundary. On Linux the runtime fails fast and
    /// the process goes with it, so everything the engine calls into the plugin has to be caught on this side,
    /// and this is where the report is made.
    ///
    /// A property that throws keeps being called. The report says what failed and the rest of the frame runs.
    /// The same failure in every frame is the same failure, so a signature is reported the first time it is seen
    /// and then once every RepeatEvery times.
    ///
    /// The counters are here for a test to read. The report itself goes to the engine's log, and this side has no
    /// way to read it back.
    /// </remarks>
    public static class FaultReporter
    {
        // A signature is the call site and the failure, which is what makes the same failure in the next frame
        // the same signature
        private static readonly Dictionary<string, int> seen = new Dictionary<string, int>();

        private const int RepeatEvery = 300;

        public static int ReportCount { get; private set; }

        // The last failure reported, as this side knows it. For an assert the context is the C# call site the
        // compiler put into the assembly
        public static string LastContext { get; private set; }
        public static string LastMessage { get; private set; }

        /// <summary>
        /// Reports a plugin exception from a guarded call. The context is the call site as this side knows it:
        /// the entry point, or the property's type and the method the guard ran.
        /// </summary>
        public static void Report(string context, Exception exception)
        {
            if (exception == null)
            {
                return;
            }

            string message = exception.GetType().Name + ": " + exception.Message;
            if (!ShouldReport(context + "|" + message))
            {
                return;
            }

            LastContext = context;
            LastMessage = message;
            Debug.LogError("[NSharp] " + context + " failed: " + message + "\n" + exception.StackTrace);
        }

        /// <summary>
        /// Reports a plugin exception from a guarded call on a plugin object. The owner is what the report names,
        /// because a plugin's type is the thing a developer recognizes.
        /// </summary>
        public static void Report(object owner, string operation, Exception exception)
        {
            Report(owner.GetType().Name + "." + operation, exception);
        }

        /// <summary>
        /// Reports a managed assertion that failed, with the call site the compiler put into the assembly.
        /// </summary>
        public static void ReportAssert(string expression, string file, int line, string member, string message, string stack)
        {
            string signature = file + ":" + line + "|" + expression + "|" + message;
            if (!ShouldReport(signature))
            {
                return;
            }

            LastContext = file + "(" + line + ") in " + member;
            LastMessage = message;
            Debug.AssertFailed(expression, file, line, message, stack);
        }

        /// <summary>
        /// Whether this failure should be reported. The count is what makes a failure that happens every frame
        /// report once and then every RepeatEvery times.
        /// </summary>
        private static bool ShouldReport(string signature)
        {
            int count;
            if (!seen.TryGetValue(signature, out count))
            {
                count = 0;
            }

            count++;
            seen[signature] = count;

            if (count != 1 && count % RepeatEvery != 0)
            {
                return false;
            }

            ReportCount++;
            return true;
        }
    }
}
