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

    /// <summary>
    /// A property with only plain fields, which is what the level format writes and reads back. The
    /// fields are written in name order, so the file the format produces is the same for the same
    /// values no matter where the fields are declared in the class.
    /// </summary>
    [ExportType]
    public class FormatTestProperty : Property
    {
        public int id;
        public string name;
        public Vector3 position;
        public float scale;
    }

    /// <summary>
    /// A property with a field named the same as one on FormatTestProperty. A change says which property it is
    /// about, and a field name on its own does not, so a change to one of them has to leave the other alone.
    /// </summary>
    [ExportType]
    public class SameFieldProperty : Property
    {
        public int id;
    }

    /// <summary>
    /// A property whose field is a reference to another entity, which is what the level format writes as the
    /// guid the file names the target by.
    /// </summary>
    public class RefTestProperty : Property
    {
        public Entity target;
    }

    /// <summary>
    /// A property that records what the runtime calls on it, which is what the sim world test checks: a
    /// property only runs in a world that is simulating, and its activation happens when the world starts
    /// simulating rather than when it is attached.
    /// </summary>
    [ExportType]
    public class SimTestProperty : Property, IMessageHandler<TestMsg>
    {
        public int activateCalls;
        public int deactivateCalls;
        public int frameCalls;
        public int messageCalls;

        public override void OnActivate()
        {
            this.activateCalls++;
        }

        public override void OnDeactivate()
        {
            this.deactivateCalls++;
        }

        public override void OnFrame()
        {
            this.frameCalls++;
        }

        public void OnMessage(in TestMsg msg)
        {
            this.messageCalls++;
        }

        protected override void RegisterMessages()
        {
            this.RegisterMessage<TestMsg>();
        }

        public override Nebula.Game.FrameEvent[] AcceptedEvents()
        {
            return new[]
            {
                Nebula.Game.FrameEvent.OnFrame
            };
        }
    }

    /// <summary>
    /// A property that counts its fixed calls. It runs on the test runner's fixed time source rather than the
    /// game logic one, so the number of steps is something the test decides rather than something the frame
    /// time gives.
    /// </summary>
    [ExportType]
    public class FixedTestProperty : Property
    {
        public int fixedCalls;

        public override void OnFixedFrame()
        {
            this.fixedCalls++;
        }

        public override uint FixedTimeSource()
        {
            return Tests.TestTimeSourceHash;
        }

        public override Nebula.Game.FrameEvent[] AcceptedEvents()
        {
            return new[]
            {
                Nebula.Game.FrameEvent.OnFixedFrame
            };
        }
    }

    /// <summary>
    /// A property that answers the activation gate. The gate and the frame count are what the test sets, and the
    /// counters are what it reads at the beginning of the frame, before that frame's events have run.
    /// </summary>
    [ExportType]
    public class GateTestProperty : Property
    {
        public bool gate;
        public int framesUntilDone;
        public int activateCalls;
        public int deactivateCalls;
        public int frameCalls;

        public override bool ShouldActivate()
        {
            return this.gate;
        }

        public override bool ShouldDeactivate()
        {
            return this.framesUntilDone > 0 && this.frameCalls >= this.framesUntilDone;
        }

        public override void OnActivate()
        {
            this.activateCalls++;
        }

        public override void OnDeactivate()
        {
            this.deactivateCalls++;
        }

        public override void OnFrame()
        {
            this.frameCalls++;
        }

        public override FrameEvent[] AcceptedEvents()
        {
            return [FrameEvent.OnFrame];
        }
    }

    /// <summary>
    /// An ActionProperty that counts what its queue does to it. It says its action is done after a number of
    /// frames, which is what lets the queue move on to the next action.
    /// </summary>
    [ExportType]
    public class QueueTestProperty : ActionProperty
    {
        public int activateCalls;
        public int deactivateCalls;
        public int frontCalls;
        public int removedCalls;
        public int frameCalls;
        public int eventCalls;
        public int loopCalls;
        public int framesUntilDone;

        public override bool ShouldDeactivate()
        {
            return this.framesUntilDone > 0 && this.frameCalls >= this.framesUntilDone;
        }

        public override void OnActivate()
        {
            this.activateCalls++;
        }

        public override void OnDeactivate()
        {
            this.deactivateCalls++;
        }

        public override void OnFrame()
        {
            this.frameCalls++;
        }

        public override void OnBecomeFrontOfQueue()
        {
            this.frontCalls++;
        }

        public override void OnRemovedFromQueue()
        {
            this.removedCalls++;
        }

        public void OnEventAction(float delta)
        {
            this.eventCalls++;
        }

        public bool OnLoopAction(float delta)
        {
            this.loopCalls++;
            return this.loopCalls < 2;
        }

        public override FrameEvent[] AcceptedEvents()
        {
            return [FrameEvent.OnFrame];
        }
    }

    /// <summary>
    /// A property that reads a sibling from OnActivate. The add does not activate, so the entity has all the
    /// properties it was given before any of them runs OnActivate. The test adds this one before its sibling
    /// and checks that OnActivate still saw it
    /// </summary>
    [ExportType]
    public class OrderTestProperty : Property
    {
        public int activateCalls;
        public bool sawSibling;

        public override void OnActivate()
        {
            this.activateCalls++;
            this.sawSibling = this.Entity.HasProperty<SiblingTestProperty>();
        }
    }

    /// <summary>
    /// The sibling OrderTestProperty reads. It only has to be on the entity
    /// </summary>
    [ExportType]
    public class SiblingTestProperty : Property
    {
        public int activateCalls;

        public override void OnActivate()
        {
            this.activateCalls++;
        }
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

        [DllImport("__Internal", EntryPoint = "SetTestWorldSimulating")]
        public static extern void SetTestWorldSimulating(uint worldId, [MarshalAs(UnmanagedType.I1)] bool simulating);

        [DllImport("__Internal", EntryPoint = "CreateTestTimeSource")]
        public static extern uint CreateTestTimeSource(double fixedFrameTime, int maxStepsPerFrame);

        [DllImport("__Internal", EntryPoint = "AdvanceTestTimeSource")]
        public static extern void AdvanceTestTimeSource(double delta);

        // The hash the test runner's fixed time source is created with
        public static readonly uint TestTimeSourceHash = TimeSourceHash.FourCC("TST1");

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

        [DllImport("__Internal", EntryPoint = "GetPropertySourceEntityId")]
        public static extern UInt64 GetPropertySourceEntityId();

        [DllImport("__Internal", EntryPoint = "GetPropertyTargetEntityId")]
        public static extern UInt64 GetPropertyTargetEntityId();

        [DllImport("__Internal", EntryPoint = "GetInstancelessEntityId")]
        public static extern UInt64 GetInstancelessEntityId();

        [DllImport("__Internal", EntryPoint = "CaptureEntityJson")]
        public static extern void CaptureEntityJson(UInt64 entityId);

        [DllImport("__Internal", EntryPoint = "ApplyPropertyJson")]
        public static extern void ApplyPropertyJson(UInt64 entityId);

        [DllImport("__Internal", EntryPoint = "ApplyEntityText")]
        public static extern void ApplyEntityText(UInt64 entityId, string text);

        // The same text read through the extension's hooks, which is the path the editor's revert takes. This is
        // what checks that the entry points the extension caches actually resolve
        [DllImport("__Internal", EntryPoint = "ResolveThroughExtension")]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool ResolveThroughExtension(string baseText, string keys, string expected);

        // The reference hooks, read through the extension the same way the document reads them. The references
        // come back as the block of "path\tguid" lines the extension hands over, because this side cannot read
        // the container C++ holds them in
        [DllImport("__Internal", EntryPoint = "TextRefsMatch")]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool TextRefsMatch(string text, string expectedPairs);

        [DllImport("__Internal", EntryPoint = "RemappedTextMatches")]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool RemappedTextMatches(string text, string pairs, string expected);

        // The override marker, read through the callback the test installed. A row asks for one key at a time,
        // and this is the only way this side can see which key it asked about
        [DllImport("__Internal", EntryPoint = "SetAuthoredAnswer")]
        public static extern void SetAuthoredAnswer(string path, [MarshalAs(UnmanagedType.I1)] bool answer);

        [DllImport("__Internal", EntryPoint = "AuthoredPathMatches")]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool AuthoredPathMatches(string expected);

        // The managed side cannot read what the native side recorded, and a string returned from C++ is one
        // the marshaler would free, so the test hands the text it expects here and the comparison is done
        // on the native side
        [DllImport("__Internal", EntryPoint = "CapturedJsonMatches")]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool CapturedJsonMatches(string expected);

        [DllImport("__Internal", EntryPoint = "RecordedChangeMatches")]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool RecordedChangeMatches(string path, string oldText, string newText);

        [DllImport("__Internal", EntryPoint = "RecordedPathMatches")]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool RecordedPathMatches(string expected);

        // The input mirror. There is no window in the test runner, so the test drives the frame and the events
        // through these and reads the result through the mirror
        [DllImport("__Internal", EntryPoint = "InputTestSnapshotSize")]
        public static extern int InputTestSnapshotSize();

        [DllImport("__Internal", EntryPoint = "InputTestKeyCount")]
        public static extern int InputTestKeyCount();

        [DllImport("__Internal", EntryPoint = "InputTestMouseButtonCount")]
        public static extern int InputTestMouseButtonCount();

        [DllImport("__Internal", EntryPoint = "InputTestKeyCode")]
        public static extern int InputTestKeyCode(string name);

        [DllImport("__Internal", EntryPoint = "InputTestMouseButtonCode")]
        public static extern int InputTestMouseButtonCode(string name);

        [DllImport("__Internal", EntryPoint = "InputTestBeginFrame")]
        public static extern void InputTestBeginFrame();

        [DllImport("__Internal", EntryPoint = "InputTestEndFrame")]
        public static extern void InputTestEndFrame();

        [DllImport("__Internal", EntryPoint = "InputTestPutKey")]
        public static extern void InputTestPutKey(int code, [MarshalAs(UnmanagedType.I1)] bool down);

        [DllImport("__Internal", EntryPoint = "InputTestPutMouseButton")]
        public static extern void InputTestPutMouseButton(int button, [MarshalAs(UnmanagedType.I1)] bool down);

        [DllImport("__Internal", EntryPoint = "InputTestPutMouseMove")]
        public static extern void InputTestPutMouseMove(float x, float y);

        [DllImport("__Internal", EntryPoint = "InputTestWarpMouse")]
        public static extern void InputTestWarpMouse(float x, float y);

        [DllImport("__Internal", EntryPoint = "InputTestPutMouseWheel")]
        public static extern void InputTestPutMouseWheel(int clicks);

        [DllImport("__Internal", EntryPoint = "InputTestPutFocus")]
        public static extern void InputTestPutFocus([MarshalAs(UnmanagedType.I1)] bool focus);

        [DllImport("__Internal", EntryPoint = "InputTestPutChar")]
        public static extern void InputTestPutChar(int chr);

        [DllImport("__Internal", EntryPoint = "InputTestCaptureMouse")]
        public static extern void InputTestCaptureMouse([MarshalAs(UnmanagedType.I1)] bool capture);

        [DllImport("__Internal", EntryPoint = "InputTestCaptureKeyboard")]
        public static extern void InputTestCaptureKeyboard([MarshalAs(UnmanagedType.I1)] bool capture);

        // The layout the engine has for a component. The generator builds the managed struct from its
        // own size table, so the test compares that table against these numbers, which come from the
        // compiler. An id that was never registered has no layout to read, so the caller checks it first
        [DllImport("__Internal", EntryPoint = "ComponentTypeSizeForManaged")]
        public static extern uint ComponentTypeSize(uint componentId);

        [DllImport("__Internal", EntryPoint = "ComponentNumFieldsForManaged")]
        public static extern uint ComponentNumFields(uint componentId);

        [DllImport("__Internal", EntryPoint = "ComponentFieldOffsetForManaged")]
        public static extern uint ComponentFieldOffset(uint componentId, uint fieldIndex);

        // The name comes into a buffer this side owns, because a DllImport that returns a string makes
        // the marshaler free the pointer it was given, and that pointer is the engine's string
        [DllImport("__Internal", EntryPoint = "ComponentFieldNameForManaged")]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool ComponentFieldName(uint componentId, uint fieldIndex, StringBuilder buffer, int bufferSize);

        // MemDb::AttributeId::Invalid(), which is what a name that was never registered answers with
        public const uint InvalidComponentId = 0xFFFF;


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
        /// <summary>
        /// The round trip through the level format for managed properties. The native side owns the
        /// buffer the json is written into and only ever sees text, which is the shape a level format
        /// has for a feature it cannot interpret. The property is put on one entity, written as text,
        /// and that text is applied to another entity that has nothing on it.
        /// </summary>
        public static void TestPropertyLevelFormat()
        {
            World world = World.Get(World.DEFAULT_WORLD);
            UInt64 sourceId = GetPropertySourceEntityId();
            UInt64 targetId = GetPropertyTargetEntityId();

            Entity source = world.GetEntity(sourceId);
            Entity target = world.GetEntity(targetId);
            Verify(source != null);
            Verify(target != null);

            // The name is longer than the buffer the native capture starts with, so the capture has to
            // answer with the length it needs and be made again
            FormatTestProperty property = new();
            property.id = 7;
            property.name = new string('x', 1200);
            property.position = new Vector3(1.5f, -2.0f, 3.0f);
            property.scale = 2.5f;
            source.AddProperty(property);

            CaptureEntityJson(sourceId);

            ApplyPropertyJson(targetId);

            FormatTestProperty applied = target.GetProperty<FormatTestProperty>();
            Verify(applied != null);
            if (applied != null)
            {
                Verify(applied.id == property.id);
                Verify(applied.name == property.name);
                Verify(applied.position == property.position);
                Verify(applied.scale == property.scale);
            }

            // The apply is exact: the properties the text names are the ones the entity has afterwards
            ApplyEntityText(targetId, "{\"FormatTestProperty\":{\"id\":3}}");

            FormatTestProperty kept = target.GetProperty<FormatTestProperty>();
            Verify(kept != null);
            if (kept != null)
            {
                Verify(kept.id == 3);
            }

            // A property the text names but the assembly does not have is skipped rather than guessed at, and the
            // ones it does name are put back
            ApplyEntityText(targetId, "{\"FormatTestProperty\":{\"id\":4},\"NoSuchProperty\":{\"x\":1}}");

            Verify(PropertyLevelFormat.HasProperty(targetId, "NoSuchProperty") == 0);
            Verify(PropertyLevelFormat.HasProperty(targetId, "FormatTestProperty") == 1);
        }

        /// <summary>
        /// The merge, the way the level format uses it: the keys one text carries, and what the base text
        /// gives when those keys are put over it. The scene cannot read either text, so this is the only place
        /// a merge can happen, and presence is the key being in the text rather than a bit in a mask.
        /// </summary>
        public static void TestPropertyMerge()
        {
            string baseText = "{\"FormatTestProperty\":{\"id\":1,\"name\":\"prefab\",\"position\":[1,2,3],\"scale\":2.5}}";
            string overText = "{\"FormatTestProperty\":{\"id\":7}}";

            List<string> keys = PropertyLevelFormat.CollectKeys(overText);
            Verify(keys.Count == 1);
            Verify(keys.Count > 0 && keys[0] == "FormatTestProperty.id");

            // Sorted, because every writer here produces the same order and the level format compares texts as
            // bytes. An order that differed between the writers would read as a change no value made
            keys = PropertyLevelFormat.CollectKeys("{\"B\":{\"y\":2},\"A\":{\"x\":1}}");
            Verify(keys.Count == 2);
            Verify(keys.Count > 1 && keys[0] == "A.x" && keys[1] == "B.y");

            // The key the instance authored takes its value and the rest still comes from the prefab. A diff
            // would not do this: a field the instance set to the same value as the prefab would start
            // following the prefab, which is not what an authored field means
            string merged = PropertyLevelFormat.MergeJson(baseText, overText);
            Verify(merged == "{\"FormatTestProperty\":{\"id\":7,\"name\":\"prefab\",\"position\":[1,2,3],\"scale\":2.5}}");

            // A key the base does not have joins it, which is what a property the instance added on its own
            // looks like. The names come out sorted, which is what makes a merge read as the same bytes the
            // text the entity would be written with when nothing changed
            merged = PropertyLevelFormat.MergeJson("{\"B\":{\"y\":2},\"A\":{\"x\":1}}", "{\"A\":{\"x\":3}}");
            Verify(merged == "{\"A\":{\"x\":3},\"B\":{\"y\":2}}");

            // A text with nothing in it changes nothing
            Verify(PropertyLevelFormat.MergeJson(baseText, "") == baseText);
            Verify(PropertyLevelFormat.MergeJson("", overText) == overText);
            Verify(PropertyLevelFormat.MergeJson("", "") == "");

            // The level format answers with null for a text it does not have, which is what a merge with
            // nothing on one side is. The marshaler throws on a null pointer, so the entry points read text
            // through this rather than directly
            Verify(PropertyLevelFormat.Utf8(IntPtr.Zero, 0) == "");

            // The partial write is what a linked entity writes, and it has to answer with the same text the
            // merge would put over the prefab. The source still holds the property the level format test put
            // on it, so this is the same value read through one key
            World world = World.Get(World.DEFAULT_WORLD);
            Entity source = world.GetEntity(GetPropertySourceEntityId());
            Verify(source != null);

            FormatTestProperty property = source == null ? null : source.GetProperty<FormatTestProperty>();
            Verify(property != null);
            if (property != null)
            {
                HashSet<string> only = new();
                only.Add("FormatTestProperty.id");

                string partial = PropertyLevelFormat.BuildJsonText(source, only);
                Verify(partial == "{\"FormatTestProperty\":{\"id\":7}}");

                // A property with none of its keys authored has nothing to say, which is what keeps an
                // instance that changed nothing from carrying the prefab's whole value
                only.Clear();
                Verify(PropertyLevelFormat.BuildJsonText(source, only) == "{}");
            }
        }

        /// <summary>
        /// What a revert to the prefab gives for a named key: the value the base text carries for it, or the
        /// field's default when the base has nothing. A field the chain never mentioned has no value to put
        /// back, and this is the only side that knows what its default is.
        /// </summary>
        public static void TestPropertyResolve()
        {
            string baseText = "{\"FormatTestProperty\":{\"id\":1,\"name\":\"prefab\",\"position\":[1,2,3],\"scale\":2.5}}";

            // The key the base carries is read from it
            Verify(PropertyLevelFormat.Resolve(baseText, "FormatTestProperty.id") == "{\"FormatTestProperty\":{\"id\":1}}");

            // A key the base does not carry falls back to the field's default, which is what a revert to a
            // prefab that never mentioned the field gives back
            Verify(PropertyLevelFormat.Resolve("", "FormatTestProperty.id") == "{\"FormatTestProperty\":{\"id\":0}}");

            // The fields come out in the order a property is written with, not the order the keys were given in
            Verify(PropertyLevelFormat.Resolve(baseText, "FormatTestProperty.scale\nFormatTestProperty.id")
                == "{\"FormatTestProperty\":{\"id\":1,\"scale\":2.5}}");

            // A property the assembly does not have cannot be answered for, so it is left out of the object the
            // keys are written into, and keys that name nothing answer with nothing
            Verify(PropertyLevelFormat.Resolve(baseText, "NoSuchProperty.x") == "{}");
            Verify(PropertyLevelFormat.Resolve(baseText, "") == "");

            // The same through the extension's hook, which is the path the editor's revert takes. A name that
            // does not match an entry point answers with nothing, so this is what checks the cache
            Verify(ResolveThroughExtension(baseText, "FormatTestProperty.id", "{\"FormatTestProperty\":{\"id\":1}}"));

            // A revert of one field is the chain's value for that key put over what the entity still has for the
            // rest of itself. The source still holds the property the level format test put on it, so the capture
            // side is the same value read through one key
            World world = World.Get(World.DEFAULT_WORLD);
            Entity source = world.GetEntity(GetPropertySourceEntityId());
            Verify(source != null);

            if (source != null)
            {
                // Every key the entity carries, not only the ones the marker has, because an exact apply takes
                // off whatever the composed text does not name
                HashSet<string> remaining = new();
                remaining.Add("FormatTestProperty.id");

                string merged = PropertyLevelFormat.MergeJson(
                    PropertyLevelFormat.Resolve(baseText, "FormatTestProperty.name"),
                    PropertyLevelFormat.BuildJsonText(source, remaining));
                Verify(merged == "{\"FormatTestProperty\":{\"id\":7,\"name\":\"prefab\"}}");

                // Taking the last key back leaves nothing to write, which is what the command reads as the
                // property being gone
                Verify(PropertyLevelFormat.MergeJson(PropertyLevelFormat.Resolve(baseText, "FormatTestProperty.id"),
                    PropertyLevelFormat.BuildJsonText(source, new HashSet<string>()))
                    == "{\"FormatTestProperty\":{\"id\":1}}");
            }

            // The override marker is the scene's own, so a row asks for the key it is drawing rather than being
            // handed the whole set. The test installs its own answer in C++, because the scene is not here, and
            // this is the only way to see that the path reaches the callback and the answer comes back
            UInt64 sourceId = GetPropertySourceEntityId();

            SetAuthoredAnswer("FormatTestProperty.id", true);
            Verify(PropertyLevelFormat.IsAuthored(sourceId, "FormatTestProperty.id"));
            Verify(AuthoredPathMatches("FormatTestProperty.id"));

            // A path with no field asks whether any field of the property is authored, which is what a section
            // row asks with
            SetAuthoredAnswer("FormatTestProperty", true);
            Verify(PropertyLevelFormat.IsAuthored(sourceId, "FormatTestProperty"));
            Verify(AuthoredPathMatches("FormatTestProperty"));

            SetAuthoredAnswer("FormatTestProperty.scale", false);
            Verify(!PropertyLevelFormat.IsAuthored(sourceId, "FormatTestProperty.scale"));
            Verify(AuthoredPathMatches("FormatTestProperty.scale"));
        }

        /// <summary>
        /// An entity reference inside a property, as the level format writes it. The format cannot resolve it
        /// on its own, because it does not read this text, so it hands the file's guid map over as the pairs
        /// this side can read.
        /// </summary>
        public static void TestPropertyRefs()
        {
            World world = World.Get(World.DEFAULT_WORLD);
            Entity source = world.CreateEntity();
            Entity target = world.CreateEntity();
            Verify(source != null);
            Verify(target != null);
            if (source == null || target == null)
            {
                return;
            }

            // The guids are the file's records. They are made up here, because these entities were created by
            // the test runner and not by a file
            const string sourceGuid = "11111111-1111-1111-1111-111111111111";
            const string targetGuid = "22222222-2222-2222-2222-222222222222";

            PropertyLevelFormat.SetRefContext(sourceGuid + ":" + source.Id + ";" + targetGuid + ":" + target.Id);

            RefTestProperty property = new();
            property.target = source;
            source.AddProperty(property);

            string text = PropertyLevelFormat.BuildJsonText(source, null);
            Verify(text == "{\"RefTestProperty\":{\"target\":\"" + sourceGuid + "\"}}");

            // The same reference, put onto the entity the file's other record was made as. The apply is exact and
            // works on the entity's shape, and the reference follows the map rather than the entity it was
            // written from, which is what a merged text needs when it names guids from several files
            PropertyLevelFormat.SetRefContext(targetGuid + ":" + target.Id);

            ApplyEntityText(target.Id, "{\"RefTestProperty\":{\"target\":\"" + targetGuid + "\"}}");

            RefTestProperty applied = target.GetProperty<RefTestProperty>();
            Verify(applied != null);
            Verify(applied != null && Object.ReferenceEquals(applied.target, target));

            // A reference the map has nothing for is nothing the file can name, and it reads as no entity
            PropertyLevelFormat.SetRefContext("");
            Verify(PropertyLevelFormat.BuildJsonText(source, null) == "{\"RefTestProperty\":{\"target\":null}}");

            // The references the text names, as this side reports them. This is the only way the level format
            // can know which part of a text it cannot read names an entity, and the paths it answers with are
            // the paths it can put guids back into
            Verify(TextRefsMatch(text, "RefTestProperty.target\t" + sourceGuid));

            // The same text against the other record's guid. This is what lets a text that names one file's
            // records be written into a file that names them by its own
            Verify(RemappedTextMatches(text, "RefTestProperty.target\t" + targetGuid,
                "{\"RefTestProperty\":{\"target\":\"" + targetGuid + "\"}}"));

            // A path given with no guid is a target the file has no record for, and that is what a file can
            // carry: the reference written as nothing
            Verify(RemappedTextMatches(text, "RefTestProperty.target\t", "{\"RefTestProperty\":{\"target\":null}}"));

            // A path the text does not name is left as the text had it, and a call with nothing to say comes
            // back as the text it was given
            Verify(RemappedTextMatches(text, "RefTestProperty.other\t" + targetGuid, text));
            Verify(RemappedTextMatches(text, "", text));
        }

        /// <summary>
        /// The mutation contract, on the entity shape the level format writes. The apply is exact, so the
        /// properties the text names are the ones the entity has afterwards, and a text that names nothing is an
        /// entity with nothing on it.
        ///
        /// The write path does not write. It works out the text before and after and hands both to the
        /// recorder, which is what runs the change. The test installs its own recorder in C++, and that
        /// recorder applies the text it is given the way a command does.
        /// </summary>
        public static void TestPropertyMutation()
        {
            World world = World.Get(World.DEFAULT_WORLD);
            UInt64 sourceId = GetPropertySourceEntityId();
            UInt64 targetId = GetPropertyTargetEntityId();

            Entity source = world.GetEntity(sourceId);
            Entity target = world.GetEntity(targetId);
            Verify(source != null);
            Verify(target != null);

            // The entity shape is the only shape, and it is the shape a command holds. The source still has the
            // long name the level format test put on it
            CaptureEntityJson(sourceId);
            Verify(CapturedJsonMatches("{\"FormatTestProperty\":{\"id\":7,\"name\":\"" + new string('x', 1200) + "\",\"position\":[1.5,-2,3],\"scale\":2.5}}"));

            // The apply is exact, so the entity's properties afterwards are the ones this text names
            ApplyEntityText(targetId, "{\"FormatTestProperty\":{\"id\":7,\"name\":\"mutated\",\"position\":[1,2,3],\"scale\":4.5}}");

            FormatTestProperty applied = target.GetProperty<FormatTestProperty>();
            Verify(applied != null);
            if (applied == null)
            {
                return;
            }

            Verify(applied.id == 7);
            Verify(applied.name == "mutated");
            Verify(applied.position == new Vector3(1, 2, 3));
            Verify(applied.scale == 4.5f);

            // The write path. The recorder is what applies the change, so the field only moves because the
            // recorder ran it
            PropertyLevelFormat.RecordChange(targetId, typeof(FormatTestProperty), "id", "9");

            Verify(RecordedChangeMatches("FormatTestProperty.id",
                "{\"FormatTestProperty\":{\"id\":7,\"name\":\"mutated\",\"position\":[1,2,3],\"scale\":4.5}}",
                "{\"FormatTestProperty\":{\"id\":9,\"name\":\"mutated\",\"position\":[1,2,3],\"scale\":4.5}}"));
            Verify(applied.id == 9);

            // The path is the key inside the text the change is about, and it is the only thing the editor's
            // override marker is built from
            Verify(RecordedPathMatches("FormatTestProperty.id"));

            // The removal is the same shape with the property left out of the after text. An entity with nothing
            // left has nothing to say, which is the same as what a capture answers with
            PropertyLevelFormat.RecordChange(targetId, typeof(FormatTestProperty), null, null);

            Verify(RecordedChangeMatches("FormatTestProperty",
                "{\"FormatTestProperty\":{\"id\":9,\"name\":\"mutated\",\"position\":[1,2,3],\"scale\":4.5}}",
                ""));
            Verify(target.GetProperty<FormatTestProperty>() == null);

            // A path with no field is the whole property, which is what a removal reports
            Verify(RecordedPathMatches("FormatTestProperty"));

            // The add. The entity has nothing, so the after text is built from a throwaway instance with one
            // field overridden, and the recorder is what creates the property because the write path never
            // creates one itself
            PropertyLevelFormat.RecordChange(targetId, typeof(FormatTestProperty), "id", "1");

            Verify(RecordedChangeMatches("FormatTestProperty", "",
                "{\"FormatTestProperty\":{\"id\":1,\"name\":null,\"position\":[0,0,0],\"scale\":0}}"));

            FormatTestProperty added = target.GetProperty<FormatTestProperty>();
            Verify(added != null);
            if (added != null)
            {
                Verify(added.id == 1);
                Verify(added.name == null);
            }

            // Undo is the same apply with the before text, which is what the command's Unexecute does
            ApplyEntityText(targetId, "{\"FormatTestProperty\":{\"id\":7,\"name\":\"mutated\",\"position\":[1,2,3],\"scale\":4.5}}");
            Verify(added != null && added.id == 7);
            Verify(added != null && added.name == "mutated");

            // Two properties with a field of the same name. The path is what says which property the change is
            // about, and the field name on its own does not, so the other property has to keep its value
            SameFieldProperty other = new() { id = 5 };
            target.AddProperty(other);

            PropertyLevelFormat.RecordChange(targetId, typeof(FormatTestProperty), "id", "11");

            Verify(RecordedChangeMatches("FormatTestProperty.id",
                "{\"FormatTestProperty\":{\"id\":7,\"name\":\"mutated\",\"position\":[1,2,3],\"scale\":4.5},\"SameFieldProperty\":{\"id\":5}}",
                "{\"FormatTestProperty\":{\"id\":11,\"name\":\"mutated\",\"position\":[1,2,3],\"scale\":4.5},\"SameFieldProperty\":{\"id\":5}}"));

            Verify(added != null && added.id == 11);
            Verify(other.id == 5);
        }

        /// <summary>
        /// An entity can be valid without having an instance, because an id can be held by something that
        /// has taken the instance out and is waiting to put it back. This is the state an editor command is
        /// in while it is on the undo stack, and it is why a property that reads the world has to check the
        /// instance rather than only the id.
        /// </summary>
        public static void TestInstancelessEntity()
        {
            UInt64 id = GetInstancelessEntityId();

            Verify(Api.IsValid(id));
            Verify(!Api.HasInstance(id));
        }

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

        /// <summary>
        /// Every component type in the engine assembly and in the plugin assembly, checked against the
        /// layout the engine reports. The managed struct is built from the generator's own size table,
        /// so a type that is not blittable, or a field whose size or alignment the table gets wrong,
        /// comes out as a struct that is the wrong size or that reads a field at the wrong offset, and
        /// nothing notices until a component is copied through Marshal. The numbers here come from the
        /// compiler, so this is the check that the two sides still agree.
        /// </summary>
        public static void TestComponentLayout()
        {
            Assembly[] assemblies = { typeof(NativeComponent).Assembly, Assembly.GetExecutingAssembly() };

            foreach (Assembly assembly in assemblies)
            {
                foreach (Type type in assembly.GetTypes())
                {
                    // The interface itself answers to IsAssignableFrom, and it is not a component
                    if (!type.IsValueType || !typeof(NativeComponent).IsAssignableFrom(type))
                        continue;

                    uint id = Api.GetComponentId(type.FullName);
                    Verify(id != InvalidComponentId);
                    if (id == InvalidComponentId)
                        continue;

                    Verify(Marshal.SizeOf(type) == ComponentTypeSize(id));

                    // The generator adds a padding field when its own fields do not reach the native
                    // size, and that field has no counterpart on the native side
                    List<FieldInfo> fields = new List<FieldInfo>();
                    foreach (FieldInfo field in type.GetFields(BindingFlags.Public | BindingFlags.Instance))
                    {
                        if (field.Name != "__pad")
                            fields.Add(field);
                    }

                    Verify((uint)fields.Count == ComponentNumFields(id));

                    // The names are compared too, because the level format and the inspector look a
                    // field up by name rather than by position, so a field that moved is a different
                    // field even when the offset happens to be right
                    StringBuilder name = new StringBuilder(64);
                    for (int i = 0; i < fields.Count; i++)
                    {
                        Verify(ComponentFieldName(id, (uint)i, name, name.Capacity));
                        Verify(name.ToString() == fields[i].Name);
                        Verify(Marshal.OffsetOf(type, fields[i].Name) == ComponentFieldOffset(id, (uint)i));
                    }
                }
            }
        }

        /// <summary>
        /// The order the matrix product applies its operands. It has to be the engine's order, because a
        /// Matrix here is the same 64 bytes a Math::mat4 is, and a composition that runs the other way gives
        /// a different transform from the same two parts. This is the same case as
        /// tests/mathtest/mat4test.cc, where trans123 * rotOneX is the transform that rotates and then
        /// translates.
        /// </summary>
        public static void TestMatrixOrder()
        {
            Matrix trans = Matrix.CreateTranslation(1.0f, 2.0f, 3.0f);
            Matrix rot = Matrix.CreateRotationX(Mathf.PiOver2);

            // The right operand is the one applied first, so a point through trans * rot is rotated and
            // then moved
            Vector3 rotated = Vector3.Transform(Vector3.Zero, rot);
            Vector3 throughProduct = Vector3.Transform(Vector3.Zero, trans * rot);
            Verify(Near(throughProduct, rotated + new Vector3(1.0f, 2.0f, 3.0f)));

            // The quaternion product has the same order, which is what makes a composed rotation agree with
            // the quaternion it came from. XNA's product order did not agree, so this is the check that the
            // two sides of this file still say the same thing
            Quaternion q1 = Quaternion.CreateFromAxisAngle(Vector3.Up, 0.3f);
            Quaternion q2 = Quaternion.CreateFromAxisAngle(Vector3.Right, 0.2f);
            Vector3 byQuaternion = Vector3.Transform(Vector3.Zero, Quaternion.Multiply(q1, q2));
            Vector3 byMatrix = Vector3.Transform(Vector3.Zero, Matrix.CreateFromQuaternion(q1) * Matrix.CreateFromQuaternion(q2));
            Verify(Near(byQuaternion, byMatrix));
        }

        private static bool Near(Vector3 a, Vector3 b)
        {
            return Math.Abs(a.X - b.X) < 1e-4f && Math.Abs(a.Y - b.Y) < 1e-4f && Math.Abs(a.Z - b.Z) < 1e-4f;
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

        // The queue membership is checked across a reload, and an instance field of the app is not something the
        // reload carries. These are the parts of the test that have to be the same objects on both sides of it
        public static QueueTestProperty StaticQueueProperty;
        public static Entity StaticQueueEntity;

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

        // Sim world test, same shape. A property only runs in a world that is simulating, so the mode is
        // what decides it, and the test runner flips the mode while the managed side reads it at the
        // beginning of the frame
        private uint simWorld;
        private SimTestProperty simProperty;

        // Time source test. The mirror is refreshed at the beginning of the frame, so a check made here sees
        // the values the engine produced for this frame
        private double inputTimeBeforePause;
        private FixedTestProperty fixedProperty;

        // The activation gate test and the action queue test. Both are read at the beginning of the frame, which
        // is where the entity's pass has already run and where this frame's events have not
        private GateTestProperty gateProperty;
        private QueueTestProperty queueProperty;
        private Entity queueEntity;
        private ActionQueue actionQueue;

        // The activation order test. Both properties are attached before the pass runs, so the one added
        // first sees the one added second from its OnActivate
        private OrderTestProperty orderProperty;
        private SiblingTestProperty siblingProperty;

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

                // A property that is at the front of its entity's queue when the reload happens. The queue is an
                // engine object held by the entity, so nothing walks it, and the property detach that runs before
                // the reattach empties it. VerifyReload is where the result of that is checked.
                Entity queueEntity = World.Get(propertyWorld).CreateEntity();
                StaticQueueProperty = new QueueTestProperty { framesUntilDone = 0 };
                StaticQueueEntity = queueEntity;
                queueEntity.AddProperty(StaticQueueProperty);
                Tests.Verify(!StaticQueueProperty.Active);
                queueEntity.GetActionQueue().Property(queueEntity, StaticQueueProperty);
                queueEntity.GetActionQueue().BeginExecution();
                Tests.Verify(StaticQueueProperty.IsFrontOfQueue);
                Tests.Verify(!StaticQueueProperty.Active);

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
            else if (this.syncPhase == 12)
            {
                // The property types of the reloaded assembly are registered again, so this is also
                // where the format is checked after a reload
                Tests.TestPropertyLevelFormat();
                this.syncPhase = 13;
            }
            else if (this.syncPhase == 13)
            {
                Tests.TestPropertyMutation();
                this.syncPhase = 14;
            }
            else if (this.syncPhase == 14)
            {
                Tests.TestInstancelessEntity();
                this.syncPhase = 15;
            }
            else if (this.syncPhase == 15)
            {
                // A world the plugin did not create is simulating by default, which is the plain game case.
                // The add does not activate by itself, it records the intent, and the entity's activation
                // pass on the next frame is what activates it
                Tests.DestroyTestWorld();
                this.simWorld = Tests.CreateTestWorld();
                World world = World.Get(this.simWorld);
                Tests.Verify(world.IsSimulating());

                Entity entity = world.CreateEntity();
                this.simProperty = new SimTestProperty();
                entity.AddProperty(this.simProperty);
                Tests.Verify(!this.simProperty.Active);
                Tests.Verify(this.simProperty.activateCalls == 0);

                this.syncPhase = 16;
            }
            else if (this.syncPhase == 16)
            {
                // The pass ran at the beginning of this frame and activated it. The event for this frame has
                // not run yet, because these checks are before the property events
                Tests.Verify(this.simProperty.Active);
                Tests.Verify(this.simProperty.activateCalls == 1);
                Tests.Verify(this.simProperty.frameCalls == 0);

                // The mode is read at the beginning of the frame, so the change is seen on the next one
                Tests.SetTestWorldSimulating(this.simWorld, false);
                this.syncPhase = 17;
            }
            else if (this.syncPhase == 17)
            {
                World world = World.Get(this.simWorld);
                Tests.Verify(!world.IsSimulating());

                // The property was active, so the mode change deactivated it. Nothing runs against a property
                // in a world that is not simulating, neither the update method nor a message. The count here is
                // the frame the pass activated it in, which was a simulating one
                Tests.Verify(!this.simProperty.Active);
                Tests.Verify(this.simProperty.deactivateCalls == 1);
                Tests.Verify(this.simProperty.frameCalls == 1);

                this.simProperty.Entity.Send(new TestMsg { f = 1 });
                Tests.Verify(this.simProperty.messageCalls == 0);

                Tests.SetTestWorldSimulating(this.simWorld, true);
                this.syncPhase = 18;
            }
            else if (this.syncPhase == 18)
            {
                World world = World.Get(this.simWorld);
                Tests.Verify(world.IsSimulating());

                // Activation runs again when the world starts simulating. The frame event for this frame has
                // not happened yet, because the checks here run before the property events, and the frame
                // before did not call it while the world was not simulating
                Tests.Verify(this.simProperty.Active);
                Tests.Verify(this.simProperty.activateCalls == 2);
                Tests.Verify(this.simProperty.frameCalls == 1);

                this.simProperty.Entity.Send(new TestMsg { f = 1 });
                Tests.Verify(this.simProperty.messageCalls == 1);

                this.syncPhase = 19;
            }
            else if (this.syncPhase == 19)
            {
                // The update method runs again in the frame after the world started simulating
                Tests.Verify(this.simProperty.frameCalls == 2);

                Tests.DestroyTestWorld();
                this.syncPhase = 20;
            }
            else if (this.syncPhase == 20)
            {
                // The mirror is refreshed at the beginning of the frame, so these are this frame's values
                TimeSource gameplay = TimeManager.Instance.Gameplay;
                TimeSource system = TimeManager.Instance.System;
                TimeSource input = TimeManager.Instance.Input;
                Tests.Verify(gameplay != null);
                Tests.Verify(system != null);
                Tests.Verify(input != null);
                Tests.Verify(gameplay.FrameTime > 0.0);
                Tests.Verify(gameplay.Ticks > 0);

                // A hash the mirror has not seen is null rather than an assert
                Tests.Verify(TimeManager.Instance.Get(Tests.TestTimeSourceHash) == null);

                // The write goes through the api and the mirror keeps the same value, so a read in the same
                // frame agrees with what was just written
                gameplay.TimeFactor = 0.5f;
                system.TimeFactor = 1.0f;
                Tests.Verify(gameplay.TimeFactor == 0.5f);

                input.Pause();
                Tests.Verify(input.IsPaused);
                this.inputTimeBeforePause = input.Time;

                this.fixedProperty = new FixedTestProperty();
                World.Get(World.DEFAULT_WORLD).CreateEntity().AddProperty(this.fixedProperty);

                // Three steps for the next frame. The source is capped at three, so the frame time the engine
                // adds on top cannot make the count larger
                Tests.CreateTestTimeSource(0.01, 3);
                Tests.AdvanceTestTimeSource(0.03);

                this.syncPhase = 21;
            }
            else if (this.syncPhase == 21)
            {
                TimeSource gameplay = TimeManager.Instance.Gameplay;
                TimeSource system = TimeManager.Instance.System;
                TimeSource input = TimeManager.Instance.Input;

                // Both sources follow the frame time, so the factor is what makes them differ
                Tests.Verify(Math.Abs(gameplay.FrameTime - system.FrameTime * 0.5) < 1e-9);

                // A paused source does not advance
                Tests.Verify(input.IsPaused);
                Tests.Verify(input.FrameTime == 0.0);
                Tests.Verify(input.Time == this.inputTimeBeforePause);

                TimeSource test = TimeManager.Instance.Get(Tests.TestTimeSourceHash);
                Tests.Verify(test != null);
                Tests.Verify(test.FixedFrameTime == 0.01);
                Tests.Verify(test.MaxStepsPerFrame == 3);

                // The fixed event has not run for this frame yet, and the mirror had no steps when it was
                // synced, because the advance happened after the sync
                Tests.Verify(this.fixedProperty.fixedCalls == 0);

                input.Resume();
                this.syncPhase = 22;
            }
            else if (this.syncPhase == 22)
            {
                // The three steps the test runner put on the source are what the fixed event ran on
                Tests.Verify(this.fixedProperty.fixedCalls == 3);
                this.syncPhase = 23;
            }
            else if (this.syncPhase == 23)
            {
                Tests.TestPropertyMerge();
                this.syncPhase = 24;
            }
            else if (this.syncPhase == 24)
            {
                Tests.TestPropertyRefs();
                this.syncPhase = 25;
            }
            else if (this.syncPhase == 25)
            {
                Tests.TestPropertyResolve();
                this.syncPhase = 26;
            }
            else if (this.syncPhase == 26)
            {
                // The contract between the two sides is the field order and the code values, and both sides
                // state them. This is where the test compares the two statements
                Tests.Verify(Input.SnapshotSize == Tests.InputTestSnapshotSize());
                Tests.Verify((int)Key.NumKeyCodes == Tests.InputTestKeyCount());
                Tests.Verify((int)MouseButton.NumMouseButtons == Tests.InputTestMouseButtonCount());
                Tests.Verify((int)Key.W == Tests.InputTestKeyCode("W"));
                Tests.Verify((int)Key.F11 == Tests.InputTestKeyCode("F11"));
                Tests.Verify((int)MouseButton.LeftButton == Tests.InputTestMouseButtonCode("LeftButton"));

                // A key pressed during the frame. The frame is what an edge belongs to
                Tests.InputTestBeginFrame();
                Tests.InputTestPutKey((int)Key.W, true);
                Input.Sync();
                Tests.Verify(Input.IsKeyPressed(Key.W));
                Tests.Verify(Input.IsKeyHeld(Key.W));
                Tests.Verify(!Input.IsKeyReleased(Key.W));
                Tests.InputTestEndFrame();

                // A modifier key on its own. The mirror's bits are built from the handler's key states, so
                // the side that was pressed is the bit that comes back, and the group answers for either side.
                // The code is taken from the engine rather than from this side's enum, so the test does not
                // depend on the two enums agreeing
                Tests.InputTestBeginFrame();
                Tests.InputTestPutKey(Tests.InputTestKeyCode("LeftControl"), true);
                Input.Sync();
                Tests.Verify(Input.Modifiers == Modifier.LeftControlModifier);
                Tests.Verify(Input.HasModifier(Modifier.ControlModifier));
                Tests.Verify(!Input.HasModifier(Modifier.ShiftModifier));
                Tests.InputTestEndFrame();

                this.syncPhase = 27;
            }
            else if (this.syncPhase == 27)
            {
                // A frame with nothing in it. The held state is the handler's state, so it survives the frame,
                // and the edge is cleared at the start of it
                Tests.InputTestBeginFrame();
                Input.Sync();
                Tests.Verify(Input.IsKeyHeld(Key.W));
                Tests.Verify(!Input.IsKeyPressed(Key.W));
                Tests.Verify(!Input.IsKeyReleased(Key.W));
                Tests.InputTestEndFrame();

                this.syncPhase = 28;
            }
            else if (this.syncPhase == 28)
            {
                Tests.InputTestBeginFrame();
                Tests.InputTestPutKey((int)Key.W, false);
                Input.Sync();

                // The release is the edge for this frame, and the held state is still true in it: the engine
                // keeps the pressed state for the frame a key was tapped in, so a press and a release inside
                // one frame is not missed
                Tests.Verify(Input.IsKeyReleased(Key.W));
                Tests.Verify(Input.IsKeyHeld(Key.W));

                // A code the mirror has no word for reads as no bit rather than as a read outside the buffer
                Tests.Verify(!Input.IsKeyHeld(Key.InvalidKey));

                Tests.InputTestEndFrame();
                this.syncPhase = 29;
            }
            else if (this.syncPhase == 29)
            {
                // The frame after a release is where the held state goes
                Tests.InputTestBeginFrame();
                Input.Sync();
                Tests.Verify(!Input.IsKeyHeld(Key.W));
                Tests.Verify(!Input.IsKeyReleased(Key.W));
                Tests.InputTestEndFrame();

                this.syncPhase = 30;
            }
            else if (this.syncPhase == 30)
            {
                Tests.InputTestBeginFrame();
                Tests.InputTestPutMouseButton((int)MouseButton.LeftButton, true);
                Tests.InputTestPutMouseMove(100.0f, 50.0f);
                Tests.InputTestPutMouseWheel(-1);
                Input.Sync();

                Tests.Verify(Input.IsMousePressed(MouseButton.LeftButton));
                Tests.Verify(Input.IsMouseHeld(MouseButton.LeftButton));
                Tests.Verify(!Input.IsMouseReleased(MouseButton.LeftButton));
                Tests.Verify(Input.MousePixelPosition == new Vector2(100.0f, 50.0f));
                Tests.Verify(Input.MouseScreenPosition == new Vector2(100.0f, 50.0f));
                Tests.Verify(Input.Wheel == -1);

                // The engine builds the movement as the position the frame started at minus the position the
                // event carried, which is the negative of the delta. See ISSUES.md
                Tests.Verify(Input.MouseMovement == new Vector2(-100.0f, -50.0f));

                // A warp the application makes does not come as an event, so the handler has to be re-based by
                // the caller that warps. The movement the frame already has is not changed by it, only the
                // origin the next event is measured from, which is what a script that re-centers the cursor
                // every frame depends on. Without the re-base this second frame would read the jump as its
                // movement
                Tests.InputTestWarpMouse(40.0f, 20.0f);
                Input.Sync();
                Tests.Verify(Input.MouseMovement == new Vector2(-100.0f, -50.0f));
                Tests.Verify(Input.MousePixelPosition == new Vector2(40.0f, 20.0f));
                Tests.InputTestEndFrame();

                Tests.InputTestBeginFrame();
                Tests.InputTestPutMouseMove(50.0f, 25.0f);
                Input.Sync();
                Tests.Verify(Input.MouseMovement == new Vector2(-10.0f, -5.0f));
                Tests.InputTestEndFrame();

                this.syncPhase = 31;
            }
            else if (this.syncPhase == 31)
            {
                // Focus and the two capture flags are the server's state rather than the handler's, and they
                // are what tell a caller that a key is missing because the UI took it
                Tests.InputTestBeginFrame();
                Tests.InputTestPutFocus(false);
                Input.Sync();
                Tests.Verify(!Input.HasFocus);
                Tests.Verify(!Input.KeyCaptured);
                Tests.Verify(!Input.MouseCaptured);
                Tests.InputTestEndFrame();

                Tests.InputTestCaptureKeyboard(true);
                Input.Sync();
                Tests.Verify(Input.KeyCaptured);

                Tests.InputTestCaptureMouse(true);
                Input.Sync();
                Tests.Verify(Input.MouseCaptured);

                Tests.InputTestCaptureKeyboard(false);
                Tests.InputTestCaptureMouse(false);
                Input.Sync();
                Tests.Verify(!Input.KeyCaptured);
                Tests.Verify(!Input.MouseCaptured);

                this.syncPhase = 32;
            }
            else if (this.syncPhase == 32)
            {
                // Text is the one part of the keyboard state that cannot come through the mirror, so it comes
                // through a buffer this side owns
                Tests.InputTestBeginFrame();
                Tests.InputTestPutChar((int)'h');
                Tests.InputTestPutChar((int)'i');
                Input.Sync();
                Tests.Verify(Input.CharInput() == "hi");
                Tests.InputTestEndFrame();

                this.syncPhase = 33;
            }
            else if (this.syncPhase == 33)
            {
                // The component mirror is a layout contract between the two sides, and nothing in the
                // build says the generator's table still matches the C++ type. This is the check
                Tests.TestComponentLayout();

                // The matrix product is the one place the managed math library does not follow XNA, so the
                // order it applies its operands in is worth a check of its own
                Tests.TestMatrixOrder();

                this.syncPhase = 34;
            }
            else if (this.syncPhase == 34)
            {
                // A property that was asked to be active but answers the activation gate with no. Attaching it
                // records the intent without activating it, which is what the entity's pass checks on the next
                // frame
                World world = World.Get(World.DEFAULT_WORLD);
                Entity entity = world.CreateEntity();
                this.gateProperty = new GateTestProperty { gate = false, framesUntilDone = 3 };
                entity.AddProperty(this.gateProperty);
                Tests.Verify(!this.gateProperty.Active);
                Tests.Verify(this.gateProperty.activateCalls == 0);

                this.gateProperty.gate = true;
                this.syncPhase = 35;
            }
            else if (this.syncPhase == 35)
            {
                // The pass ran at the beginning of this frame and activated it. The event for this frame has not
                // run yet, because these checks are before the property events
                Tests.Verify(this.gateProperty.Active);
                Tests.Verify(this.gateProperty.activateCalls == 1);
                Tests.Verify(this.gateProperty.deactivateCalls == 0);
                Tests.Verify(this.gateProperty.frameCalls == 0);

                this.syncPhase = 36;
            }
            else if (this.syncPhase == 36)
            {
                Tests.Verify(this.gateProperty.frameCalls == 1);
                Tests.Verify(this.gateProperty.Active);
                this.syncPhase = 37;
            }
            else if (this.syncPhase == 37)
            {
                Tests.Verify(this.gateProperty.frameCalls == 2);
                Tests.Verify(this.gateProperty.Active);
                this.syncPhase = 38;
            }
            else if (this.syncPhase == 38)
            {
                // The frame count reached the number it was told to stop at, so the pass deactivated it at the
                // beginning of this frame. The frame that ran the last event was the one that reached the count
                Tests.Verify(this.gateProperty.frameCalls == 3);
                Tests.Verify(!this.gateProperty.Active);
                Tests.Verify(this.gateProperty.deactivateCalls == 1);

                // Asking for it to be active again works after it deactivated itself, because the setter's guard
                // is on what the property actually is rather than on what it was asked to be. With the gate closed
                // that ask records the intent without activating it, and the pass on the next frame is what
                // activates it when the gate opens again
                this.gateProperty.gate = false;
                this.gateProperty.Active = true;
                Tests.Verify(!this.gateProperty.Active);
                Tests.Verify(this.gateProperty.activateCalls == 1);

                this.gateProperty.gate = true;
                this.syncPhase = 39;
            }
            else if (this.syncPhase == 39)
            {
                Tests.Verify(this.gateProperty.Active);
                Tests.Verify(this.gateProperty.activateCalls == 2);
                this.syncPhase = 40;
            }
            else if (this.syncPhase == 40)
            {
                // Taking the gate property out of the running state so the queue test has an entity of its own
                this.gateProperty.Active = false;
                Tests.Verify(!this.gateProperty.Active);
                Tests.Verify(this.gateProperty.deactivateCalls == 2);

                // A property in a queue does not run until the queue says it is at the front of it
                World world = World.Get(World.DEFAULT_WORLD);
                this.queueEntity = world.CreateEntity();
                this.queueProperty = new QueueTestProperty { framesUntilDone = 2 };
                this.queueEntity.AddProperty(this.queueProperty);
                Tests.Verify(!this.queueProperty.Active);

                this.actionQueue = this.queueEntity.GetActionQueue();
                this.actionQueue.Property(this.queueEntity, this.queueProperty);
                this.actionQueue.Event(this.queueEntity, this.queueProperty.OnEventAction);
                this.actionQueue.BeginExecution();

                Tests.Verify(this.queueProperty.IsFrontOfQueue);
                Tests.Verify(this.queueProperty.frontCalls == 1);
                Tests.Verify(!this.queueProperty.Active);
                this.syncPhase = 41;
            }
            else if (this.syncPhase == 41)
            {
                // The pass activated it because the queue said it was at the front, and the queue did not move on
                // because the action is still running
                Tests.Verify(this.queueProperty.Active);
                Tests.Verify(this.queueProperty.activateCalls == 1);
                Tests.Verify(this.actionQueue.CurrentType == ActionQueue.ActionType.Property);
                Tests.Verify(Object.ReferenceEquals(this.actionQueue.CurrentProperty, this.queueProperty));
                Tests.Verify(this.queueProperty.frameCalls == 0);
                this.syncPhase = 42;
            }
            else if (this.syncPhase == 42)
            {
                Tests.Verify(this.queueProperty.frameCalls == 1);
                Tests.Verify(this.queueProperty.Active);
                this.syncPhase = 43;
            }
            else if (this.syncPhase == 43)
            {
                // It said it was done at the beginning of this frame, which is where the pass deactivated it and
                // the queue moved on to the event. Execute loads the next action, it does not run it in the same
                // call, so the event has not run yet
                Tests.Verify(this.queueProperty.frameCalls == 2);
                Tests.Verify(!this.queueProperty.Active);
                Tests.Verify(this.queueProperty.deactivateCalls == 1);
                Tests.Verify(!this.queueProperty.IsFrontOfQueue);
                Tests.Verify(this.queueProperty.removedCalls == 1);
                Tests.Verify(this.actionQueue.CurrentType == ActionQueue.ActionType.Event);
                Tests.Verify(this.queueProperty.eventCalls == 0);
                Tests.Verify(this.actionQueue.IsEmpty());
                this.syncPhase = 44;
            }
            else if (this.syncPhase == 44)
            {
                // The event ran on this frame's Execute and the queue has nothing left
                Tests.Verify(this.queueProperty.eventCalls == 1);
                Tests.Verify(this.actionQueue.CurrentType == ActionQueue.ActionType.None);
                Tests.Verify(this.actionQueue.IsEmpty());

                // A property that is at the front of the queue when the queue is cleared is deactivated and told
                // it is no longer in it. The action that was still waiting behind it is taken out as well
                this.actionQueue.Property(this.queueEntity, this.queueProperty);
                this.actionQueue.Loop(this.queueEntity, this.queueProperty.OnLoopAction);
                this.actionQueue.BeginExecution();

                Tests.Verify(this.queueProperty.IsFrontOfQueue);
                Tests.Verify(!this.queueProperty.Active);
                this.syncPhase = 45;
            }
            else if (this.syncPhase == 45)
            {
                // The pass activated it first, so Clear has a running property to take out
                Tests.Verify(this.queueProperty.Active);
                Tests.Verify(this.queueProperty.activateCalls == 2);

                this.actionQueue.Clear();

                Tests.Verify(!this.queueProperty.Active);
                Tests.Verify(this.queueProperty.deactivateCalls == 2);
                Tests.Verify(this.queueProperty.removedCalls == 2);
                Tests.Verify(!this.queueProperty.IsFrontOfQueue);
                Tests.Verify(this.actionQueue.IsEmpty());
                Tests.Verify(this.actionQueue.CurrentType == ActionQueue.ActionType.None);
                this.syncPhase = 46;
            }
            else if (this.syncPhase == 46)
            {
                // An Idle with no duration is done in the frame it runs in, and the Loop after it is loaded by the
                // same Execute but run by the next one
                this.actionQueue.Idle(this.queueEntity, 0.0f);
                this.actionQueue.Loop(this.queueEntity, this.queueProperty.OnLoopAction);
                this.actionQueue.BeginExecution();

                Tests.Verify(this.actionQueue.CurrentType == ActionQueue.ActionType.Idle);
                this.syncPhase = 47;
            }
            else if (this.syncPhase == 47)
            {
                Tests.Verify(this.actionQueue.CurrentType == ActionQueue.ActionType.Loop);
                Tests.Verify(this.queueProperty.loopCalls == 0);
                this.syncPhase = 48;
            }
            else if (this.syncPhase == 48)
            {
                // The loop ran once and answered true
                Tests.Verify(this.queueProperty.loopCalls == 1);
                Tests.Verify(this.actionQueue.CurrentType == ActionQueue.ActionType.Loop);
                this.syncPhase = 49;
            }
            else if (this.syncPhase == 49)
            {
                // The second call answered false, which ended the action and left the queue empty
                Tests.Verify(this.queueProperty.loopCalls == 2);
                Tests.Verify(this.actionQueue.CurrentType == ActionQueue.ActionType.None);
                Tests.Verify(this.actionQueue.IsEmpty());
                this.syncPhase = 50;
            }
            else if (this.syncPhase == 50)
            {
                // The activation order guarantee. The dependent property is added first, so with the add
                // recording the intent rather than activating, the entity has both properties before the pass
                // runs OnActivate on either of them
                World world = World.Get(World.DEFAULT_WORLD);
                Entity entity = world.CreateEntity();
                this.orderProperty = new OrderTestProperty();
                this.siblingProperty = new SiblingTestProperty();
                entity.AddProperty(this.orderProperty);
                entity.AddProperty(this.siblingProperty);
                Tests.Verify(!this.orderProperty.Active);
                Tests.Verify(!this.siblingProperty.Active);

                this.syncPhase = 51;
            }
            else if (this.syncPhase == 51)
            {
                // The pass activated both. The one added first read its sibling from OnActivate and found it,
                // even though it was added after, which is what the deferred activation is for
                Tests.Verify(this.orderProperty.Active);
                Tests.Verify(this.orderProperty.activateCalls == 1);
                Tests.Verify(this.orderProperty.sawSibling);
                Tests.Verify(Object.ReferenceEquals(this.orderProperty.Entity.GetProperty<SiblingTestProperty>(), this.siblingProperty));
                Tests.Verify(this.siblingProperty.Active);
                Tests.Verify(this.siblingProperty.activateCalls == 1);

                this.syncPhase = 52;
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

            // A queue does not survive the reload. Nothing walks it, because it is an engine object held by the
            // entity, and the unload detaches every property from every entity before the reattach, which empties
            // it. This is the documented behaviour, and this is where it is stated: the property comes back on its
            // entity, but it is no longer an action the queue runs
            Tests.Verify(StaticQueueProperty != null);
            Tests.Verify(!StaticQueueProperty.IsFrontOfQueue);
            Tests.Verify(StaticQueueEntity.GetActionQueue().IsEmpty());
            Tests.Verify(StaticQueueEntity.GetActionQueue().CurrentType == ActionQueue.ActionType.None);
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
