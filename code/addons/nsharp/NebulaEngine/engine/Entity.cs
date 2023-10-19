using System;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;
using System.Collections;
using System.Collections.Generic;
using Mathf;

using Api = Nebula.Game.NebulaApiV1;

namespace Nebula
{
    namespace Game
    {
        [NativeCppClass]
        [StructLayout(LayoutKind.Sequential)]
        public struct EntityId
        {
            public uint id;
        }

        /// <summary>
        /// Represents a game object that resides in a game world.
        /// Contains properties and native components that make up the behaviour and logic of the entity.
        /// Entities can receive messages which are propagated to all properties that listen for the message.
        /// </summary>
        public class Entity
        {
            private World world = null;
            private uint id = 0xFFFFFFFF;
            private List<Property> properties;
            private MsgDispatcher dispatcher = new MsgDispatcher();

            /// <summary>
            /// The world that this entity belongs to
            /// </summary>
            public World World { get { return this.world; } }
            /// <summary>
            /// Unique identifier for the entity
            /// </summary>
            public uint Id { get { return this.id; } }

            /// <remarks>
            /// Do not create new entities using this constructor, instead create them via World.CreateEntity.
            /// </remarks>
            internal Entity(World world, uint entityId)
            {
                this.world = world;
                this.id = entityId;
                this.properties = new List<Property>();
            }

            /// <summary>
            /// Check if entity is valid (not destroyed)
            /// </summary>
            public bool IsValid() { return Api.IsValid(this.world.Id, this.id); }

            /// <summary>
            /// Check if entity has some unmanaged component
            /// </summary>
            public bool HasComponent<T>() where T : struct, NativeComponent
            {
                uint componentId = ComponentManager.Instance.GetComponentId<T>();
                return Api.HasComponent(this.world.Id, this.id, componentId);
            }

            /// <summary>
            /// Set the value of a native, unmanaged component.
            /// </summary>
            /// <remarks>
            /// This method does allocate and copy memory, thus should be used sparringly in time-critical code.
            /// </remarks>
            public void SetComponent<T>(in T component) where T : struct, NativeComponent
            {
                uint componentId = ComponentManager.Instance.GetComponentId<T>();
                int size = Marshal.SizeOf<T>();
                // HACK: there might be more efficient ways to avoid GC problems, if there are any...
                IntPtr ptr = Marshal.AllocHGlobal(size);
                Marshal.StructureToPtr(component, ptr, false);
                Api.SetComponentData(this.world.Id, this.id, componentId, ptr, size);
                Marshal.FreeHGlobal(ptr);
            }

            /// <summary>
            /// Gets a value copy of the native, unmanaged component.
            /// </summary>
            /// <remarks>
            /// This method does allocate and copy memory, thus should be used sparringly in time-critical code.
            /// </remarks>
            public T GetComponent<T>() where T : struct, NativeComponent
            {
                uint componentId = ComponentManager.Instance.GetComponentId<T>();
                int size = Marshal.SizeOf<T>();
                IntPtr ptr = Marshal.AllocHGlobal(size);
                Api.GetComponentData(this.world.Id, this.id, componentId, ptr, size);
                T component = Marshal.PtrToStructure<T>(ptr);
                Marshal.FreeHGlobal(ptr);
                return component;
            }

            /// <summary>
            /// Checks if the entity has a certain property
            /// </summary>
            /// <remarks>
            /// This is a linear search (O(N)) through all properties attached to this entity.
            /// </remarks>
            public bool HasProperty<T>() where T : Property
            {
                for (int i = 0; i < this.properties.Count; i++)
                {
                    if (this.properties[i] is T)
                    {
                        return true;
                    }
                }
                return false;
            }

            /// <summary>
            /// Adds a property to the entity
            /// </summary>
            public void AddProperty(Property property)
            {
                Debug.Assert(this.id != 0xFFFFFFFF);
                property.Entity = this;
                this.properties.Add(property);
                PropertyManager.Instance.RegisterProperty(property);
                property.Active = true;

                this.dispatcher.AttachHandler(property.OnMessage, property.AcceptedMessages());
            }

            /// <summary>
            /// Gets the property if it exists in the entity.
            /// </summary>
            /// <remarks>
            /// This is a linear search (O(N)) through all properties attached to this entity.
            /// </remarks>
            /// <returns>
            /// Null, or the property if entity has it.
            /// </returns>
            public T GetProperty<T>() where T : Property
            {
                for (int i = 0; i < this.properties.Count; i++)
                {
                    if (this.properties[i] is T)
                    {
                        return this.properties[i] as T;
                    }
                }

                return null;
            }

            /// <summary>
            /// Gets the transform from the entity.
            /// </summary>
            public Matrix GetTransform()
            {
                return Api.GetTransform(this.world.Id, this.id);
            }

            /// <summary>
            /// Sets the transform of the entity.
            /// </summary>
            public void SetTransform(Matrix matrix)
            {
                Api.SetTransform(this.world.Id, this.id, matrix);
            }

            /// <summary>
            /// Sends this entity a message.
            /// The message will be propagated into all Properties that this entity has, that accepts the message.
            /// </summary>
            public void Send<T>(in T msg) where T : struct, Msg
            {
                this.dispatcher.Dispatch(msg);
            }

            /// <summary>
            /// Schedule the entity for destruction.
            /// Note that this does not immediately remove the entity from the native backend.
            /// </summary>
            public static void Destroy(Entity entity)
            {
                if (entity.id == 0xFFFFFFFF)
                    return; // Already destroyed

                Api.DeleteEntity(entity.world.Id, entity.id);
                entity.id = 0xFFFFFFFF;

                for (int i = 0; i < entity.properties.Count; i++)
                {
                    Property property = entity.properties[i];
                    property.Destroy();
                }

                entity.properties.Clear();
            }

            /// <summary>
            /// Constructs a string representation of this entity
            /// </summary>
            public override string ToString()
            {
                return id.ToString();
            }
        }
    }
}
