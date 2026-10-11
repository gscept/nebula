using System;
using System.IO;
using System.Text;
using Nebula;

namespace ConsoleHook
{

    public class ConsoleEvents
    {
        public static void WriteLineFunc(object sender, ConsoleWriterEventArgs e)
        {
            //Debug.Log($"{e.Value}\n");
        }

        public static void WriteFunc(object sender, ConsoleWriterEventArgs e)
        {
            Debug.Log(e.Value);
        }
    }

    public class ConsoleWriterEventArgs(string value) : EventArgs
    {
        public string Value { get; private set; } = value;
    }

    public class ConsoleWriter : TextWriter
    {
        public override Encoding Encoding => Encoding.UTF8;

        public override void Write(string value)
        {
            WriteEvent?.Invoke(this, new ConsoleWriterEventArgs(value));
            base.Write(value);
        }

        public override void WriteLine(string value)
        {
            WriteLineEvent?.Invoke(this, new ConsoleWriterEventArgs(value));
            base.WriteLine(value);
        }

        public event EventHandler<ConsoleWriterEventArgs> WriteEvent;
        public event EventHandler<ConsoleWriterEventArgs> WriteLineEvent;
    }

}
