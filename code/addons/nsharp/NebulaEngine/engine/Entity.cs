using System;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;
using System.Collections.Generic;
using System.Diagnostics.CodeAnalysis;

using Api = Nebula.Game.NebulaApiV1;

namespace Nebula
{
    namespace Game
    {
        [NativeCppClass]
        [StructLayout(LayoutKind.Sequential)]
        public struct EntityId
        {
            public UInt64 id;
        }

        /// <summary>
        /// Represents a game object that resides in a game world.
        /// Contains properties and native components that make up the behaviour and logic of the entity.
        /// Entities can receive messages which are propagated to all properties that listen for the message.
        /// </summary>
        public class Entity
        {
            private readonly World world;
            private UInt64 id = 0xFFFFFFFFFFFFFFFF;
            private List<Property> properties;
            private MsgDispatcher dispatcher;

            /// <summary>
            /// The world that this entity belongs to
            /// </summary>
            public World World => this.world;
            /// <summary>
            /// Unique identifier for the entity
            /// </summary>
            public UInt64 Id => this.id;

            /// <remarks>
            /// Do not create new entities using this constructor, instead create them via World.CreateEntity.
            /// </remarks>
            internal Entity(World world, UInt64 entityId)
            {
                this.world = world;
                this.id = entityId;
            }

            /// <summary>
            /// Check if entity has some unmanaged component
            /// </summary>
            public bool HasComponent<T>() where T : struct, NativeComponent
            {
                uint componentId = ComponentManager.Instance.GetComponentId<T>();
                return Api.HasComponent(this.id, componentId);
            }

            /// <summary>
            /// Set the value of a native, unmanaged component.
            /// </summary>
            /// <remarks>
            /// This method copies the component through a stack-backed buffer.
            /// </remarks>
            public unsafe void SetComponent<T>(in T component) where T : struct, NativeComponent
            {
                uint componentId = ComponentManager.Instance.GetComponentId<T>();
                int size = Marshal.SizeOf<T>();
                Span<byte> buffer = stackalloc byte[size];
                T value = component;
                MemoryMarshal.Write(buffer, in value);
                fixed (byte* ptr = &buffer[0])
                {
                    Api.SetComponentData(this.id, componentId, (IntPtr)ptr, size);
                }
            }

            /// <summary>
            /// Gets a value copy of the native, unmanaged component.
            /// </summary>
            /// <remarks>
            /// This method copies the component through a stack-backed buffer.
            /// </remarks>
            public unsafe T GetComponent<T>() where T : struct, NativeComponent
            {
                uint componentId = ComponentManager.Instance.GetComponentId<T>();
                int size = Marshal.SizeOf<T>();
                Span<byte> buffer = stackalloc byte[size];
                fixed (byte* ptr = &buffer[0])
                {
                    Api.GetComponentData(this.id, componentId, (IntPtr)ptr, size);
                }
                return MemoryMarshal.Read<T>(buffer);
            }

            /// <summary>
            /// Checks if the entity has a certain property
            /// </summary>
            /// <remarks>
            /// This is a linear search (O(N)) through all properties attached to this entity.
            /// </remarks>
            public bool HasProperty<T>() where T : Property
            {
                if (this.properties == null)
                {
                    return false;
                }

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
            public void AddProperty(Property property, bool active = true)
            {
                Debug.Assert(this.id != 0xFFFFFFFF);
                this.properties ??= [];

                property.Entity = this;
                this.properties.Add(property);
                PropertyManager.Instance.RegisterProperty(property);
                property.AttachMessages();
                property.Active = active;
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
                if (this.properties == null)
                {
                    return null;
                }

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
            /// Gets the position of the entity.
            /// </summary>
            public Vector3 GetPosition()
            {
                return Api.GetPosition(this.id);
            }

            /// <summary>
            /// Sets the position of the entity.
            /// </summary>
            public void SetPosition(Vector3 position)
            {
                Api.SetPosition(this.id, position);
            }

            /// <summary>
            /// Gets the orientation of the entity.
            /// </summary>
            public Quaternion GetOrientation()
            {
                return Api.GetOrientation(this.id);
            }

            /// <summary>
            /// Sets the orientation of the entity.
            /// </summary>
            public void SetOrientation(Quaternion orientation)
            {
                Api.SetOrientation(this.id, orientation);
            }

            /// <summary>
            /// Gets the scale of the entity.
            /// </summary>
            public Vector3 GetScale()
            {
                return Api.GetScale(this.id);
            }

            /// <summary>
            /// Sets the scale of the entity.
            /// </summary>
            public void SetScale(Vector3 scale)
            {
                Api.SetScale(this.id, scale);
            }

            /// <summary>
            /// Sends this entity a message.
            /// The message will be propagated into all Properties that this entity has, that accepts the message.
            /// </summary>
            public void Send<T>(in T msg) where T : Msg
            {
                if (this.dispatcher != null)
                {
                    this.dispatcher.Dispatch(in msg);
                }
            }

            internal void RegisterMessage<T>(IMessageHandler<T> handler) where T : Msg
            {
                this.dispatcher ??= new MsgDispatcher();
                this.dispatcher.Register(handler);
            }

            internal void UnregisterMessages(Property property)
            {
                if (this.dispatcher != null)
                {
                    this.dispatcher.Unregister(property);
                }
            }

            internal List<Property> GetProperties()
            {
                return this.properties;
            }

            internal void ClearProperties()
            {
                if (this.properties == null)
                {
                    return;
                }

                for (int i = 0; i < this.properties.Count; i++)
                {
                    Property property = this.properties[i];
                    PropertyManager.Instance.UnregisterProperty(property);
                    property.DetachMessages();
                    property.Destroy();
                }
                this.properties.Clear();
                this.dispatcher = null;
            }

            /// <summary>
            /// Schedule the entity for destruction.
            /// Note that this does not immediately remove the entity from the native backend.
            /// </summary>
            public static void Destroy(Entity entity)
            {
                entity.world.DeregisterEntity(entity);

                if (entity.id == 0xFFFFFFFFFFFFFFFF)
                {
                    return; // Already destroyed
                }

                Api.DeleteEntity(entity.id);
                entity.Detach();
            }

            /// <summary>
            /// Release the managed side of the entity without touching the native one. Used when the
            /// world the entity belongs to goes away, because the native entities are gone with it and
            /// there is nothing left to delete through the api.
            /// </summary>
            internal void Detach()
            {
                this.id = 0xFFFFFFFFFFFFFFFF;
                this.ClearProperties();
            }

            /// <summary>
            /// Constructs a string representation of this entity
            /// </summary>
            public override string ToString()
            {
                return this.id.ToString();
            }
        }

        //--------------------------------------------------------------------------
        /**
        */
        public static class EntityExtensions
        {
            //--------------------------------------------------------------------------
            /** <summary>
                Returns true if entity is valid (not destroyed).

                This is safe to call even if the entity == null.
                </summary>
            */
#nullable enable
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            public static bool IsValid([NotNullWhen(true)] this Entity? entity)
            {
                return entity != null && Api.IsValid(entity.Id);
            }
        }
    }
}
