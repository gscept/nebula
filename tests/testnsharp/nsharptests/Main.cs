using System;
using System.ComponentModel;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Security;
using ConsoleHook;
using Mathf;
using Nebula.Game;
using NST;

namespace NST
{
    public class TestProperty : Property
    {
        public int i = 0;
        public float f;
        public string s;
        public Mathf.Vector3 v;

        public override void OnBeginFrame()
        {
            i++;
            Console.WriteLine(String.Format("TestProperty OnBeginFrame() called {0} times", i));
        }

        public override FrameEvent[] AcceptedEvents()
        {
            return new[]
            {
                FrameEvent.OnBeginFrame
            };
        }
    }

    class AudioEmitterProperty : Property
    {
        public bool autoplay;
        public bool loop;
        public float volume;
        public float pitch;

        public override void OnActivate()
        {
            base.OnActivate();

            // Do on activate stuff
        }

        public override void OnMessage(in Msg msg)
        {

        }

        public override void OnBeginFrame()
        {
            base.OnBeginFrame();

            Mathf.Matrix t = this.Entity.GetTransform();
            AudioEmitterProperty prop = this.Entity.GetProperty<AudioEmitterProperty>();

            // Do on frame stuff
        }

        public override System.Type[] AcceptedMessages()
        {
            return new[]
            {
                 typeof(PlayAudioMessage)
            };
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

        [UnmanagedCallersOnly]
        static public void PerformTests()
        {
            World world = Nebula.Game.World.Get(World.DEFAULT_WORLD);

            TestProperty testProp0 = new TestProperty();
            Entity entity = world.CreateEntity("Empty");
            entity.AddProperty(testProp0);

            Verify(entity.IsValid());

            Entity entity2 = world.CreateEntity("Empty");
            Verify(entity2.IsValid());
            Verify(entity2.Id != entity.Id);

            Entity.Destroy(entity);

            Verify(!entity.IsValid());
            Verify(entity2.IsValid());

            Entity.Destroy(entity2);

            Verify(!entity2.IsValid());

            TestProperty p1 = new TestProperty();
            Entity entity3 = world.CreateEntity("Empty");
            entity3.AddProperty(p1);

            Nebula.Game.Component.Owner owner = entity3.GetComponent<Nebula.Game.Component.Owner>();
            Verify(owner.entity == entity3.Id);

            Matrix transform = entity3.GetTransform();
            Verify(transform == Matrix.Identity);

            transform.M11 = 11;
            transform.M12 = 12;
            transform.M32 = 32;
            transform.M41 = 41;
            transform.M42 = 42;
            transform.M43 = 43;

            entity3.SetTransform(transform);
            Matrix newTransform = entity3.GetTransform();
            Verify(transform == newTransform);

            Verify(transform.Right == new Vector3(11, 12, 0));
            Verify(transform.Up == new Vector3(0, 1, 0));
            // TODO: Make sure our matrices in cpp and c# use the same coordinate system by default
            // Verify(transform.Forward == new Vector3(0, 32, 1));
            Verify(transform.Translation == new Vector3(41, 42, 43));

            Verify(1 == 1);
        }

        public class VariablePassing
        {
            [DllImport("__Internal", EntryPoint = "PassVec2"), SuppressUnmanagedCodeSecurity]
            public static extern void TestPassVec2(in Mathf.Vector2 vec);
            [DllImport("__Internal", EntryPoint = "PassVec3"), SuppressUnmanagedCodeSecurity]
            public static extern void TestPassVec3(in Mathf.Vector3 vec);
            [DllImport("__Internal", EntryPoint = "PassVec4"), SuppressUnmanagedCodeSecurity]
            public static extern void TestPassVec4(in Mathf.Vector4 vec);

            [UnmanagedCallersOnly]
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
            [DllImport("__Internal", EntryPoint = "PassString")]
            public static extern void PassString([MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Util.StringMarshaler))] string val);

            [DllImport("__Internal", EntryPoint = "TestArrayOfInt")]
            [SuppressGCTransition]
            public static extern void TestArrayOfInt(int[] arr, int size);

            [DllImport("__Internal", EntryPoint = "TestArrayOfVec3")]
            [SuppressGCTransition]
            public static extern void TestArrayOfVec3(Mathf.Vector3[] arr, int size);

            [DllImport("__Internal", EntryPoint = "TestArrayOfVec4")]
            [SuppressGCTransition] // This attribute can be used to avoid transitioning the GC, saving on performance. There are rules however: https://learn.microsoft.com/en-us/dotnet/api/system.runtime.interopservices.suppressgctransitionattribute?view=net-7.0
            public static extern void TestArrayOfVec4(Mathf.Vector4[] arr, int size);

            [UnmanagedCallersOnly]
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

                Mathf.Vector3[] vec3Arr = new Mathf.Vector3[10];
                for (int i = 0; i < vec3Arr.Length; i++)
                {
                    vec3Arr[i] = new Vector3(1, 2, 3);
                }
                TestArrayOfVec3(vec3Arr, vec3Arr.Length);


                Mathf.Vector4[] vec4Arr = new Mathf.Vector4[10];
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
            Nebula.Debug.Log("Nebula.Debug.Log works!\n");
        }
    }
}

public class AppEntry
{
    [UnmanagedCallersOnly]
    static public void Main()
    {
        Nebula.Runtime.OverrideNebulaApp(new TestApp());
        Nebula.Runtime.Setup(Assembly.GetExecutingAssembly());
        Nebula.Runtime.Start();
    }

    [UnmanagedCallersOnly]
    static public void OnBeginFrame()
    {
        Nebula.Runtime.OnBeginFrame();
    }

    [UnmanagedCallersOnly]
    static public void OnFrame()
    {
        Nebula.Runtime.OnFrame();
    }

    [UnmanagedCallersOnly]
    static public void OnEndFrame()
    {
        Nebula.Runtime.OnEndFrame();
    }

    [UnmanagedCallersOnly]
    static public void Exit()
    {
        Nebula.Runtime.Close();
    }
}
