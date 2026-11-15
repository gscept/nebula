using System;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Security;
using System.Text;
using System.Collections.Generic;
using System.Reflection;
using ConsoleHook;
using Nebula;
using Nebula.Game;
using NST;
using Test;

using Api = Nebula.Game.NebulaApiV1;

[assembly: NebulaGamePlugin(typeof(NST.TestPlugin))]

namespace NST
{
    [ExportType]
    public enum ReloadMode
    {
        First,
        Second
    }

    [ExportType]
    public struct ExportedStruct
    {
        public ExportedState State;
        public int Count;
    }

    [ExportType]
    public sealed class TestPlugin : IGamePlugin
    {
        public void OnLoad(GamePluginContext context)
        {
            Tests.RecordLoadAttempt();

            // The fourth attempt is the one the test drives on purpose. It has to fail once, and the
            // manager must not keep retrying the same build on the frames after it.
            if (Tests.GetLoadAttempts() == 4)
            {
                throw new InvalidOperationException("Simulated broken build");
            }

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

    [ExportType]
    public class TestProperty : Property, IMessageHandler<TestMsg>
    {
        public int i = 0;
        public float f;
        public string s;
        public Vector3 v;
        public ExportedState state = new ExportedState();
        public List<Vector3> vectors = new List<Vector3>();
        public Dictionary<string, ExportedState> states = new Dictionary<string, ExportedState>();
        public ExportedState[] stateArray;
        public ExportedState[,] stateGrid;
        public HashSet<ExportedState> stateSet = new HashSet<ExportedState>();
        public Queue<ExportedState> stateQueue = new Queue<ExportedState>();
        public WeakReference<ExportedState> weakState;
        public WeakReference weakCycle = new WeakReference(null);
        public WeakReference<object> genericWeakCycle = new WeakReference<object>(null);
        public ConditionalWeakTable<ExportedState, ExportedState> weakStates = new ConditionalWeakTable<ExportedState, ExportedState>();
        public Func<int> readPrivateValue;
        public Type exportedType = typeof(ExportedState);
        public MethodInfo exportedMethod = typeof(TestProperty).GetMethod(nameof(PrepareReloadState));
        public Entity entityReference;
        public ReloadMode mode = ReloadMode.Second;
        public ValueTuple<ExportedState, int> stateTuple;
        public ExportedStruct? optionalState;
        private int privateValue = 42;
        [NoHotReload]
        public int resetValue = 42;
        private int messageCallsAtPrepare;

        public TestProperty()
        {
            this.state.Position = new Vector3(1, 2, 3);
            this.state.Shared = this.state;
            this.vectors.Add(this.state.Position);
            this.states.Add("main", this.state);
            this.stateArray = new[] { this.state, this.state };
            this.stateGrid = new ExportedState[1, 1];
            this.stateGrid[0, 0] = this.state;
            this.stateSet.Add(this.state);
            this.stateQueue.Enqueue(this.state);
            this.weakState = new WeakReference<ExportedState>(this.state);
            this.weakCycle.Target = this.weakCycle;
            this.genericWeakCycle.SetTarget(this.genericWeakCycle);
            this.weakStates.Add(this.state, this.state);
            this.readPrivateValue = () => this.privateValue;
            this.stateTuple = new ValueTuple<ExportedState, int>(this.state, 12);
            this.optionalState = new ExportedStruct { State = this.state, Count = 13 };
        }

        public void PrepareReloadState()
        {
            this.privateValue = 99;
            this.resetValue = 99;
            this.entityReference = this.Entity;
            this.messageCallsAtPrepare = Tests.GetPropertyMessageCalls();
        }

        public void VerifyReloadState()
        {
            Tests.Verify(this.i > 0);
            Tests.Verify(this.privateValue == 99);
            Tests.Verify(this.resetValue == 42);
            Tests.Verify(Object.ReferenceEquals(this.state, this.state.Shared));
            Tests.Verify(this.state.CreatedCalled);
            Tests.Verify(Object.ReferenceEquals(this.state, this.states["main"]));
            Tests.Verify(Object.ReferenceEquals(this.state, this.stateArray[0]));
            Tests.Verify(Object.ReferenceEquals(this.state, this.stateArray[1]));
            Tests.Verify(Object.ReferenceEquals(this.state, this.stateGrid[0, 0]));
            Tests.Verify(this.stateSet.Contains(this.state));
            Tests.Verify(Object.ReferenceEquals(this.state, this.stateQueue.Peek()));
            ExportedState weakTarget;
            Tests.Verify(this.weakState.TryGetTarget(out weakTarget) && Object.ReferenceEquals(this.state, weakTarget));
            Tests.Verify(Object.ReferenceEquals(this.weakCycle, this.weakCycle.Target));
            Tests.Verify(this.genericWeakCycle.TryGetTarget(out object cycleTarget) && Object.ReferenceEquals(this.genericWeakCycle, cycleTarget));
            ExportedState weakTableValue;
            Tests.Verify(this.weakStates.TryGetValue(this.state, out weakTableValue) && Object.ReferenceEquals(this.state, weakTableValue));
            Tests.Verify(this.readPrivateValue() == 99);
            Tests.Verify(this.exportedType == typeof(ExportedState));
            Tests.Verify(this.exportedMethod.DeclaringType == typeof(TestProperty));
            Tests.Verify(this.mode == ReloadMode.Second);
            Tests.Verify(Object.ReferenceEquals(this.state, this.stateTuple.Item1) && this.stateTuple.Item2 == 12);
            Tests.Verify(this.optionalState.HasValue && Object.ReferenceEquals(this.state, this.optionalState.Value.State) && this.optionalState.Value.Count == 13);
            Tests.Verify(this.vectors.Count == 1 && this.vectors[0].X == 1);
            Tests.Verify(this.Entity != null && this.Entity.IsValid() && Object.ReferenceEquals(this.Entity, this.entityReference));

            // PropertyManager and the message dispatcher live in the engine assembly, so a property
            // from a retired plugin assembly keeps being called if the reload does not detach it
            Tests.Verify(Tests.GetPropertyFrameCalls() == this.i);
            this.Entity.Send(new TestMsg { f = 1 });
            Tests.Verify(Tests.GetPropertyMessageCalls() == this.messageCallsAtPrepare + 1);
        }

        public override void OnBeginFrame()
        {
            i++;
            Tests.RecordPropertyFrameCall();
            Debug.Log(String.Format("TestProperty OnBeginFrame() called {0} times\n", i));

            Console.WriteLine(this.state.Position);
            Console.WriteLine(this.state.Shared);
            Console.WriteLine(this.vectors[0]);
            Console.WriteLine(this.states["main"].Position);
        }

        public void OnMessage(in TestMsg msg)
        {
            Tests.RecordPropertyMessageCall();
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

    /// <summary>
    /// A property attached to an entity in a world the plugin did not create. Reload has to look at
    /// every world the managed runtime knows about, not only the default one.
    /// </summary>
    [ExportType]
    public class WorldProperty : Property
    {
        public int value;
    }

    [ExportType]
    public class ExportedState : IHotReloadManaged
    {
        public Vector3 Position;
        public ExportedState Shared;
        public bool CreatedCalled;

        public void Destroyed(IDictionary<string, object> state)
        {
            state["position"] = this.Position;
            // These callbacks are discovered while draining a callback-owned collection.
            state["nested"] = new List<CallbackState> { new CallbackState() };
        }

        public void Created(IReadOnlyDictionary<string, object> state)
        {
            Vector3 position = (Vector3)state["position"];
            if (position != this.Position)
                throw new InvalidOperationException("Hot reload callback state was not migrated.");
            this.CreatedCalled = true;
        }
    }

    [ExportType]
    public class CallbackState : IHotReloadManaged
    {
        public void Destroyed(IDictionary<string, object> state)
        {
            state["mode"] = ReloadMode.Second;
        }

        public void Created(IReadOnlyDictionary<string, object> state)
        {
            if (state["mode"] is not ReloadMode mode || mode != ReloadMode.Second)
                throw new InvalidOperationException("Nested callback state retained its old assembly type.");
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

        [DllImport("__Internal", EntryPoint = "GetNativeEntityIdForManaged")]
        private static extern UInt64 GetNativeEntityId();

        [DllImport("__Internal", EntryPoint = "GetComponentEntityIdForManaged")]
        private static extern UInt64 GetComponentEntityId();

        [DllImport("__Internal", EntryPoint = "CreateTestWorld")]
        public static extern uint CreateTestWorld();

        [DllImport("__Internal", EntryPoint = "DestroyTestWorld")]
        public static extern void DestroyTestWorld();

        [DllImport("__Internal", EntryPoint = "RecordPropertyFrameCall")]
        public static extern void RecordPropertyFrameCall();

        [DllImport("__Internal", EntryPoint = "RecordLoadAttempt")]
        public static extern void RecordLoadAttempt();

        [DllImport("__Internal", EntryPoint = "GetLoadAttempts")]
        public static extern int GetLoadAttempts();

        [DllImport("__Internal", EntryPoint = "RecordPropertyMessageCall")]
        public static extern void RecordPropertyMessageCall();

        [DllImport("__Internal", EntryPoint = "GetPropertyFrameCalls")]
        public static extern int GetPropertyFrameCalls();

        [DllImport("__Internal", EntryPoint = "GetPropertyMessageCalls")]
        public static extern int GetPropertyMessageCalls();

        public static void Verify(bool test,
        [CallerFilePath] string filePath = "",
        [CallerLineNumber] int lineNumber = 0)
        {
            VERIFY(test, filePath, lineNumber);
        }

        /// <summary>
        /// The test runner creates an entity in the default world from C++ before the managed
        /// tests run. Native code is the source of truth, and the script feature unit mirrors the
        /// entities of a world into the managed one at the beginning of the frame, which is what
        /// makes an entity authored in C++ visible to the plugin.
        /// </summary>
        public static void TestNativeEntityDiscovery()
        {
            World world = World.Get(World.DEFAULT_WORLD);
            Verify(world != null);
            Verify(World.HasWorld(World.DEFAULT_WORLD));
            Verify(World.Get(63) == null);

            UInt64 nativeId = GetNativeEntityId();
            Verify(nativeId != 0xFFFFFFFFFFFFFFFF);

            Entity entity = world.GetEntity(nativeId);
            Verify(entity != null);
            Verify(entity.Id == nativeId);
            Verify(Object.ReferenceEquals(entity.World, world));

            // The mirror must resolve to the same object every time
            Verify(Object.ReferenceEquals(world.GetEntity(nativeId), entity));

            int count = 0;
            bool foundNative = false;
            for (int i = 0; i < world.Entities.Count; i++)
            {
                Entity mirrored = world.Entities[i];
                count++;
                if (mirrored.Id == nativeId)
                {
                    foundNative = true;
                    Verify(Object.ReferenceEquals(mirrored, entity));
                }
            }

            Verify(foundNative);
            Verify(count > 0);
        }

        /// <summary>
        /// Verify that the IDL generator maps component types to blittable mirrors of the native
        /// layout, and that a component round-trips through native with that layout.
        /// </summary>
        public static void TestTypeMappings()
        {
            Verify(typeof(TestFlags).GetField("flags32").FieldType == typeof(uint));
            Verify(typeof(TestFlags).GetField("flags64").FieldType == typeof(UInt64));
            Verify(typeof(TestAtom).GetField("atom").FieldType == typeof(ResourceDescriptor));

            World world = World.Get(World.DEFAULT_WORLD);
            Entity entity = world.GetEntity(GetComponentEntityId());
            Verify(entity != null);
            Verify(entity.HasComponent<TestFlags>());
            Verify(entity.HasComponent<TestAtom>());

            TestFlags flags = new TestFlags { flags32 = 0x80000000, flags64 = 0x8000000000000000 };
            entity.SetComponent(flags);

            TestFlags read = entity.GetComponent<TestFlags>();
            Verify(read.flags32 == 0x80000000);
            Verify(read.flags64 == 0x8000000000000000);
        }

        static public TestProperty PerformTests()
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
            p1.PrepareReloadState();

            TestMsg2 testmsg2 = new TestMsg2
            {
                f = 200.0f
            };
            entity3.Send(testmsg2);

            Verify(1 == 1);
            return p1;
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

    [ExportType]
    public class TestApp : NebulaApp
    {
        public static ExportedState StaticState = new ExportedState();
        public static readonly List<ExportedState> StaticStates = new List<ExportedState>();
        private bool testsRun;
        private int reloadCount;
        private bool reloadVerified;
        private TestProperty property;

        // Entity sync test, driven across frames. Native is the source of truth, and the managed
        // mirror only changes when the script feature unit flushes or sweeps it.
        private int syncPhase;
        private UInt64 syncEntity;
        private UInt64 transientEntity;

        // World sync test, same frame-by-frame shape. The world is created and destroyed by the
        // test runner, which is what the editor does with its authoring worlds.
        private uint testWorld;
        private UInt64 testWorldEntity;
        private World staleWorld;
        private Entity staleEntity;
        private WorldProperty worldProperty;
        private Entity worldPropertyEntity;

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
                Tests.TestNativeEntityDiscovery();
                Tests.TestTypeMappings();
                this.property = Tests.PerformTests();
                StaticState.Position = new Vector3(9, 8, 7);
                StaticStates.Clear();
                StaticStates.Add(this.property.state);

                // Create an entity purely in native code. It is not in the managed mirror until the
                // entity queue has been flushed at the beginning of the next frame.
                World world = World.Get(World.DEFAULT_WORLD);
                this.syncEntity = Api.CreateEntity(world.Id);
                Tests.Verify(world.GetEntity(this.syncEntity) == null);
                this.syncPhase = 1;
            }
            else if (this.syncPhase == 1)
            {
                // The entity queue was flushed at the beginning of this frame
                World world = World.Get(World.DEFAULT_WORLD);
                Entity entity = world.GetEntity(this.syncEntity);
                Tests.Verify(entity != null);
                Tests.Verify(Object.ReferenceEquals(world.GetEntity(this.syncEntity), entity));

                // Deletion is deferred, so the mirror still holds the entity until the sweep at the
                // end of the frame
                Api.DeleteEntity(this.syncEntity);
                Tests.Verify(Object.ReferenceEquals(world.GetEntity(this.syncEntity), entity));
                this.syncPhase = 2;
            }
            else if (this.syncPhase == 2)
            {
                // ManageEntities ran at the end of the previous frame, which made the entity id
                // invalid, and the script feature unit swept the mirror in OnDecay
                Tests.Verify(World.Get(World.DEFAULT_WORLD).GetEntity(this.syncEntity) == null);
                this.syncPhase = 3;
            }
            else if (this.syncPhase == 3)
            {
                // Create and destroy in the same frame. Deletion is deferred, so the entity is no
                // longer valid by the time the queue is flushed, and the entity count is the same as
                // at the last sweep. It must not end up in the mirror.
                World world = World.Get(World.DEFAULT_WORLD);
                this.transientEntity = Api.CreateEntity(world.Id);
                Api.DeleteEntity(this.transientEntity);
                this.syncPhase = 4;
            }
            else if (this.syncPhase == 4)
            {
                Tests.Verify(World.Get(World.DEFAULT_WORLD).GetEntity(this.transientEntity) == null);
                this.syncPhase = 5;
            }
            else if (this.syncPhase == 5)
            {
                // A world the script did not create still gets a mirror, which is the editor case
                this.testWorld = Tests.CreateTestWorld();
                World world = World.Get(this.testWorld);
                Tests.Verify(world != null);
                Tests.Verify(Object.ReferenceEquals(world, World.Get(this.testWorld)));
                Tests.Verify(world.Entities.Count == 0);

                this.testWorldEntity = Api.CreateEntity(this.testWorld);
                Tests.Verify(world.GetEntity(this.testWorldEntity) == null);
                this.staleWorld = world;
                this.syncPhase = 6;
            }
            else if (this.syncPhase == 6)
            {
                World world = World.Get(this.testWorld);
                Entity entity = world.GetEntity(this.testWorldEntity);
                Tests.Verify(entity != null);
                Tests.Verify(world.Entities.Count == 1);
                this.staleEntity = entity;

                Tests.DestroyTestWorld();

                // The mirror goes with the world, and the world object goes with the mirror so the
                // recycled slot gets a fresh one
                Tests.Verify(World.Get(this.testWorld) == null);
                Tests.Verify(this.staleWorld.Entities.Count == 0);
                Tests.Verify(this.staleWorld.GetEntity(this.testWorldEntity) == null);
                Tests.Verify(!this.staleEntity.IsValid());
                this.syncPhase = 7;
            }
            else if (this.syncPhase == 7)
            {
                // The slot the destroyed world used is free, so the next world takes it and starts
                // at index 0 generation 0, which is the same entity id the old mirror still had
                uint recycled = Tests.CreateTestWorld();
                Tests.Verify(recycled == this.testWorld);

                World world = World.Get(recycled);
                Tests.Verify(world != null);
                Tests.Verify(!Object.ReferenceEquals(world, this.staleWorld));
                Tests.Verify(world.Entities.Count == 0);
                Tests.Verify(!Object.ReferenceEquals(world.GetEntity(this.testWorldEntity), this.staleEntity));

                UInt64 sameId = Api.CreateEntity(recycled);
                Tests.Verify(sameId == this.testWorldEntity);
                this.syncPhase = 8;
            }
            else if (this.syncPhase == 8)
            {
                World world = World.Get(this.testWorld);
                Entity entity = world.GetEntity(this.testWorldEntity);
                Tests.Verify(entity != null);
                Tests.Verify(!Object.ReferenceEquals(entity, this.staleEntity));
                Tests.Verify(entity.IsValid());
                Tests.Verify(Object.ReferenceEquals(entity.World, world));
                Tests.Verify(world.Entities.Count == 1);

                Tests.DestroyTestWorld();

                // A property on an entity in a world the plugin did not create. Reload has to look
                // at every world the runtime knows about, not only the default one.
                uint propertyWorld = Tests.CreateTestWorld();
                Tests.Verify(propertyWorld == this.testWorld);
                Entity propertyEntity = World.Get(propertyWorld).CreateEntity();
                this.worldPropertyEntity = propertyEntity;
                this.worldProperty = new WorldProperty { value = 7 };
                propertyEntity.AddProperty(this.worldProperty);
                this.syncPhase = 9;
            }
            else if (this.reloadCount < 2)
            {
                this.reloadCount++;
                HotReload.Request();
            }
            else if (!this.reloadVerified)
            {
                this.VerifyReload();
                this.reloadVerified = true;
            }
            else if (this.syncPhase == 9)
            {
                // TestPlugin.OnLoad throws on the fourth attempt
                HotReload.Request();
                this.syncPhase = 10;
            }
            else if (this.syncPhase == 10)
            {
                // Nothing is requested here. The frame after the failure is where a manager that
                // re-requests on failure would attempt the same build again.
                this.syncPhase = 11;
            }
            else if (this.syncPhase == 11)
            {
                Tests.Verify(Tests.GetLoadAttempts() == 4);
                this.syncPhase = 12;
            }
            base.OnFrame();
        }

        private void VerifyReload()
        {
            Tests.Verify(this.property != null);
            this.property.VerifyReloadState();
            Tests.Verify(StaticState.Position == new Vector3(9, 8, 7));
            Tests.Verify(StaticStates.Count == 1 && Object.ReferenceEquals(StaticStates[0], this.property.state));

            // The property is on an entity in a world the plugin did not create. If the reload only
            // looks at the default world, the new instance is never attached to the entity.
            Tests.Verify(Object.ReferenceEquals(this.worldProperty.Entity, this.worldPropertyEntity));
            Tests.Verify(this.worldProperty.value == 7);
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
