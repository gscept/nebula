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
        public class Entity
        {
            private World world;
            public World World { get { return this.world; } }
            private uint id;
            public uint Id { get { return this.id; } }

            private List<Property> properties;

            /// <summary>
            /// Do not create new entities using this constructor, instead create them via World.CreateEntity.
            /// </summary>
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

            // This is for getting and setting unmanaged components
            public bool HasComponent<T>() { return false; }
            public void SetComponent<T>(T component) { return; }
            public T GetComponent<T>() { return default(T); }

            // These are for managed properties
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
            public void AddProperty(Property property)
            {
                property.Entity = this;
                this.properties.Add(property);
            }
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

            public Matrix GetTransform()
            {
                return default(Matrix);
            }
            public void SetTransform(Matrix matrix)
            {
                return;
            }

            public void Send(in Msg msg)
            {
                // TODO: implement me!
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
                // TODO: Clean up anything related to the entity
            }

            public override string ToString()
            {
                return id.ToString();
            }
        }
    }
}
