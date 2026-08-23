using System;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Security;
using System.Text;
using ConsoleHook;
using Nebula;
using Nebula.Game;
using NST;

[assembly: NebulaGamePlugin(typeof(NST.TestPlugin))]

namespace NST
{
    public sealed class TestPlugin : IGamePlugin
    {
        public void OnLoad(GamePluginContext context)
        {
            context.SetApplication(new TestApp());
        }

        public void OnUnload()
        {
        }
    }

    public struct TestMsg : Msg
    {
        public float f;
    }

    public struct TestMsg2 : Msg
    {
        public float f;
    }

    public class TestProperty : Property, IMessageHandler<TestMsg>
    {
        public int i = 0;
        public float f;
        public string s;
        public Vector3 v;

        public override void OnBeginFrame()
        {
            i++;
            Debug.Log(String.Format("TestProperty OnBeginFrame() called {0} times\n", i));
        }

        public void OnMessage(in TestMsg msg)
        {
            Console.WriteLine("Received TestMsg : " + msg.f);
        }

        protected override void RegisterMessages()
        {
            this.RegisterMessage<TestMsg>();
        }

        public override FrameEvent[] AcceptedEvents()
        {
            return new[]
            {
                FrameEvent.OnBeginFrame
            };
        }
    }

    class AudioEmitterProperty : Property, IMessageHandler<PlayAudioMessage>
    {
        public bool autoplay = false;
        public bool loop = false;
        public float volume = 0;
        public float pitch = 0;

        public override void OnActivate()
        {
            base.OnActivate();

            // Do on activate stuff
        }

        protected override void RegisterMessages()
        {
            this.RegisterMessage<PlayAudioMessage>();
        }

        public void OnMessage(in PlayAudioMessage msg)
        {
        }

        public override void OnBeginFrame()
        {
            base.OnBeginFrame();

            Vector3 pos = this.Entity.GetPosition();
            AudioEmitterProperty prop = this.Entity.GetProperty<AudioEmitterProperty>();

            // Do on frame stuff
        }

        public override Nebula.Game.FrameEvent[] AcceptedEvents()
        {
            return new[]
            {
                Nebula.Game.FrameEvent.OnBeginFrame
            };
        }
    }

    public class PlayAudioMessage : Msg
    {
        public float volume;
        public bool looping;
    }

    public class Tests
    {
        [DllImport("__Internal", EntryPoint = "VerifyManaged")]
        private static extern void VERIFY(bool success, string filePath, int lineNumber);

        public static void Verify(bool test,
        [CallerFilePath] string filePath = "",
        [CallerLineNumber] int lineNumber = 0)
        {
            VERIFY(test, filePath, lineNumber);
        }

        static public void PerformTests()
        {
            World world = World.Get(World.DEFAULT_WORLD);

            TestProperty testProp0 = new TestProperty();
            Entity entity = world.CreateEntity();
            entity.AddProperty(testProp0);

            Verify(entity.IsValid());

            Entity entity2 = world.CreateEntity();
            Verify(entity2.IsValid());
            Verify(entity2.Id != entity.Id);

            Entity.Destroy(entity);

            Verify(!entity.IsValid());
            Verify(entity2.IsValid());

            Entity.Destroy(entity2);

            Verify(!entity2.IsValid());

            TestProperty p1 = new TestProperty();
            Entity entity3 = world.CreateEntity();
            entity3.AddProperty(p1);

            Vector3 position = entity3.GetPosition();
            Verify(position == Vector3.Zero);

            position.X = 1;
            position.Y = 2;
            position.Z = 3;
            
            entity3.SetPosition(position);
            Vector3 newPosition = entity3.GetPosition();
            Verify(position == newPosition);

            // TODO: Verify that orientation is OK, and that we have the same representation of orientations real value (w or x)

            TestMsg testmsg = new TestMsg
            {
                f = 100.0f
            };
            entity3.Send(testmsg);

            TestMsg2 testmsg2 = new TestMsg2
            {
                f = 200.0f
            };
            entity3.Send(testmsg2);

            Verify(1 == 1);
        }

        public class VariablePassing
        {
            [DllImport("__Internal", EntryPoint = "PassVec2"), SuppressUnmanagedCodeSecurity]
            public static extern void TestPassVec2(in Vector2 vec);
            [DllImport("__Internal", EntryPoint = "PassVec3"), SuppressUnmanagedCodeSecurity]
            public static extern void TestPassVec3(in Vector3 vec);
            [DllImport("__Internal", EntryPoint = "PassVec4"), SuppressUnmanagedCodeSecurity]
            public static extern void TestPassVec4(in Vector4 vec);

            public static void RunTests()
            {
                Vector2 vec2 = new Vector2(1, 2);
                TestPassVec2(vec2);
                Vector3 vec3 = new Vector3(1, 2, 3);
                TestPassVec3(vec3);
                Vector4 vec4 = new Vector4(1, 2, 3, 4);
                TestPassVec4(vec4);
            }
        }

        public class DLLImportCalls
        {
            [DllImport("__Internal", EntryPoint = "PassStringUtf8")]
            private static extern void PassStringUtf8(IntPtr data, int length);

            private static unsafe void PassString(string val)
            {
                int byteCount = Encoding.UTF8.GetByteCount(val);
                Span<byte> buffer = stackalloc byte[byteCount + 1];
                byteCount = Encoding.UTF8.GetBytes(val.AsSpan(), buffer);
                fixed (byte* ptr = &buffer[0])
                {
                    PassStringUtf8((IntPtr)ptr, byteCount);
                }
            }

            [DllImport("__Internal", EntryPoint = "TestArrayOfInt")]
            [SuppressGCTransition]
            public static extern void TestArrayOfInt(int[] arr, int size);

            [DllImport("__Internal", EntryPoint = "TestArrayOfVec3")]
            [SuppressGCTransition]
            public static extern void TestArrayOfVec3(Vector3[] arr, int size);

            [DllImport("__Internal", EntryPoint = "TestArrayOfVec4")]
            [SuppressGCTransition]
            public static extern void TestArrayOfVec4(Vector4[] arr, int size);

            public static void RunTests()
            {
                string myStr = "This is a C# string!\n";
                PassString(myStr);

                int[] arr = new int[10];
                for (int i = 0; i < arr.Length; i++)
                {
                    arr[i] = i + 1;
                }
                TestArrayOfInt(arr, arr.Length);

                Vector3[] vec3Arr = new Vector3[10];
                for (int i = 0; i < vec3Arr.Length; i++)
                {
                    vec3Arr[i] = new Vector3(1, 2, 3);
                }
                TestArrayOfVec3(vec3Arr, vec3Arr.Length);


                Vector4[] vec4Arr = new Vector4[10];
                for (int i = 0; i < vec4Arr.Length; i++)
                {
                    vec4Arr[i] = new Vector4(1, 2, 3, 4);
                }
                TestArrayOfVec4(vec4Arr, vec4Arr.Length);
            }
        }
    }

    public class TestApp : NebulaApp
    {
        private bool testsRun;

        public override void OnBeginFrame()
        {
            base.OnBeginFrame();
        }

        public override void OnEndFrame()
        {
            base.OnEndFrame();
        }

        public override void OnFrame()
        {
            if (!this.testsRun)
            {
                this.testsRun = true;
                Tests.VariablePassing.RunTests();
                Tests.DLLImportCalls.RunTests();
                Tests.PerformTests();
            }
            base.OnFrame();
        }

        public override void OnShutdown()
        {
            base.OnShutdown();
        }

        public override void OnStart()
        {
            base.OnStart();

            Console.Write("Console.Write works!\n");
            Console.WriteLine("Console.WriteLine works!");
            Debug.Log("Debug.Log works!\n");
        }
    }
}
