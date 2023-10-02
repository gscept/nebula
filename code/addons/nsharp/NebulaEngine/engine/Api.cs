using System;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;
using System.Collections;
using System.Collections.Generic;
using Mathf;


/*
Entities are only ids as per usual, pointing into their respective tables in unmanaged code
Components in managed code are objects that are allocated linearly per type by some object pool manager.
 */

namespace Psuedo
{
    struct Entity
    {
        uint id;

        //bool IsAlive() { return Nebula.Game.IsAlive(this.id); }

        // This is for getting and setting unmanaged components
        public bool HasComponent<T>() { return false; }
        public void SetComponent<T>(T component) { return; }
        public T GetComponent<T>() { return default(T); }

        // These are for managed properties
        public bool HasProperty<T>() { return false; }
        public void AddProperty<T>() { }
        public T GetProperty<T>() { return default(T); }

        public Matrix GetTransform() { return default(Matrix); }
        public void SetTransform(Matrix matrix) { return; }

        public void Send(in Msg msg)
        {
            for (int i = 0; i < properties.Count; i++)
            {
                properties[i].OnMessage(msg);
            }
        }

        static void Destroy(Entity entity) { }

        private List<Property> properties;
    }

    // Example of imported, "unmanaged" component
    [NativeCppClass]
    struct AudioEmitter
    {
        uint emitterId;
        bool autoplay;
        bool loop;
    }

    class Property
    {
        private Entity owner;

        public bool active;

        public Entity Owner
        {
            get
            {
                return owner;
            }
        }

        public virtual Nebula.Game.Events[] AcceptedEvents()
        {
            // Override in subclass to tell the property manager which functions to call for this class
            // This is only executed once when the property type is registered to the property manager
            return null;
        }

        public virtual void SetupAcceptedMessages()
        {
            // Override in subclass
            // This is only executed once when the property type is registered to the property manager
        }

        public virtual void OnMessage(in Msg msg)
        {
            // Override in subclass
        }

        public virtual void OnActivate() { }
        public virtual void OnDeactivate() { }
        public virtual void OnBeginFrame() { }
    }

    class Msg
    {
    }

    class PlayAudioMessage : Msg
    {
        public bool loop;
        public float volume;
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
            PlayAudioMessage audio = (PlayAudioMessage)msg;
            if (audio != null)
            {

            }
        }

        public override void OnBeginFrame()
        {
            base.OnBeginFrame();

            Mathf.Matrix t = this.Owner.GetTransform();
            AudioEmitterProperty prop = this.Owner.GetProperty<AudioEmitterProperty>();

            // Do on frame stuff
        }

        public override void SetupAcceptedMessages()
        {
        }

        public override Nebula.Game.Events[] AcceptedEvents()
        {
            return new[] {
                Nebula.Game.Events.OnActivate,
                Nebula.Game.Events.OnFrame
            };
        }
    }

    class NebulaApiV1
    {
        // ...
    }

    class NebulaApp
    {
        void OnStart(NebulaApiV1 api)
        {
            //PropertyManager.Register<AudioEmitterProperty>();

            // api.DoSomething();???
        }

        void OnShutdown()
        {
            // Might be able to automate deregistering, if we keep track of the registered proeprties per api object passed  to the dll.
            // PropertyManager.Deregister<AudioEmitterProperty>();
        }
    }

}

namespace Nebula
{
    namespace Game
    {
        public enum Events
        {
            OnActivate,
            OnDeactivate,
            // Update order
            //OnBeginFrame, // Called every frame before rendering (old vis results)
            //OnFixedFrame, // Called every couple of frames with a fixed delta frame time. These updates are distributed and might not be called on the same frame for all components (called after OnBeginFrame)
            OnFrame, // Called every frame while rendering (up to date vis results)
                     //OnEndFrame, // Called after rendering is done

            // Total number of events
            NumEvents
        }

        public delegate void EventDelegate();

        public struct ComponentData<T>
        {
            public void Allocate()
            {
                //this.buffer.Add(T());
            }

            public T this[int index]
            {
                get
                {
                    return buffer[index];
                }
                set
                {
                    buffer[index] = value;
                }
            }

            // TODO: should be a native array, with access methods via internal calls.
            private List<T> buffer;
        }

        public sealed class ComponentManager
        {
            private static readonly ComponentManager instance = new ComponentManager();
            // Explicit static constructor to tell C# compiler
            // not to mark type as beforefieldinit
            static ComponentManager()
            {
                // Initialize eventcallbacks list to a specific size.
                Instance.eventCallbacks = new List<List<EventDelegate>>();
                for (int i = 0; i < (int)Events.NumEvents; i++)
                {
                    Instance.eventCallbacks.Add(new List<EventDelegate>());
                }
            }
            private ComponentManager() { }

            public static ComponentManager Instance
            {
                get
                {
                    return instance;
                }
            }

            public static void RegisterComponent(IComponent component)
            {
                component.SetupEvents();
                Instance.registry.Add(component);
            }

            public static void SetupEventDelegate(Events e, EventDelegate func)
            {
                Instance.eventCallbacks[(int)e].Add(func);
            }

            public static void OnFrame()
            {
                int numCallbacks = Instance.eventCallbacks[(int)Events.OnFrame].Count;
                for (int i = 0; i < numCallbacks; ++i)
                {
                    Instance.eventCallbacks[(int)Events.OnFrame][i]();
                }
            }

            private List<IComponent> registry = new List<IComponent>();
            private List<List<EventDelegate>> eventCallbacks;
        }

        public interface IComponent
        {
            InstanceId Register(Game.Entity entity);
            void Deregister(Game.Entity entity);
            void OnActivate(InstanceId id);
            void OnDeactivate(InstanceId id);
            void SetupEvents();
        }

        public class Component<DATA> : IComponent
        {
            public InstanceId Register(Game.Entity entity)
            {
                InstanceId instance = new InstanceId((uint)size++);
                //foreach (PropertyInfo propertyInfo in this.GetProperties())
                //{
                //    if (propertyInfo.CanRead)
                //    {
                //        propertyInfo.GetValue().Allocate();
                //    }
                //}

                this.OnActivate(instance);
                return instance;
            }

            public void Deregister(Game.Entity entity)
            {
                InstanceId instance = entityMap[entity];
                this.OnDeactivate(instance);

                // TODO: Dealloc data.

            }

            protected void RegisterEvent(Events e, EventDelegate func)
            {
                events.Add(e);
                Game.ComponentManager.SetupEventDelegate(e, func);
            }

            public virtual void SetupEvents()
            {
                // override in subclass
            }

            public virtual void OnActivate(InstanceId id)
            {
                // override in subclass
            }

            public virtual void OnDeactivate(InstanceId id)
            {
                // override in subclass
            }

            public readonly List<Events> events = new List<Events>();

            private int size;
            protected ComponentData<DATA> data;
            private Dictionary<Entity, InstanceId> entityMap;
        }

        /*
         * Entity   
         */
        public struct Entity : IEquatable<Entity>
        {
            private UInt32 id;

            public Entity(uint id)
            {
                this.id = id;
            }

            public uint Id
            {
                get
                {
                    return id;
                }
            }

            /// <summary>
            /// This entitys transform
            /// </summary>
            public Matrix Transform
            {
                get
                {
                    return GetTransform();
                }

                set
                {
                    // TODO: Send set transform message.
                    // maybe check if this entity is registered first and register it if necessary?
                    SetTransform(value);
                }
            }

            public T GetComponent<T>()
            {
                Debug.Assert(false);
                return default(T);
            }

            /// <summary>
            /// Check whether this entity is valid (alive)
            /// </summary>
            [MethodImplAttribute(MethodImplOptions.InternalCall)]
            public static extern bool IsValid();

            /// <summary>
            /// Convert entity to string representation
            /// </summary>
            public override string ToString() { return this.id.ToString(); }

            /// <summary>
            /// Check if two entities are the same
            /// </summary>
            public bool Equals(Entity other) { return this.id == other.id; }

            /// <summary>
            /// Retrieve the transform of an entity if it is registered to the component
            /// </summary>
            [MethodImplAttribute(MethodImplOptions.InternalCall)]
            private extern Matrix GetTransform();

            [MethodImplAttribute(MethodImplOptions.InternalCall)]
            private extern void SetTransform(Matrix mat);


        }

        public struct InstanceId
        {
            private uint id;

            public InstanceId(uint id)
            {
                this.id = id;
            }

            public uint Id
            {
                get
                {
                    return id;
                }
            }
        }
    }

    public class EntityManager
    {
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public static extern Game.Entity CreateEntity();

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public static extern void DeleteEntity(Game.Entity entity);
    }

    public class Debug
    {
        [DllImport("__Internal", EntryPoint = "N_Print")]
        public static extern void Log(string val);

        [DllImport("__Internal", EntryPoint = "N_Assert")]
        public static extern void Assert(bool value);

        [DllImport("__Internal", EntryPoint = "N_Assert")]
        public static extern void Assert(bool value, string message);
    }

    // [DllImport ("__Internal", EntryPoint="Foobar", CharSet=CharSet.Ansi)]
    // static extern void Foobar(
    //     [MarshalAs (UnmanagedType.CustomMarshaler,
    //         MarshalTypeRef=typeof(StringMarshaler))]
    //     String val
    // );

}

