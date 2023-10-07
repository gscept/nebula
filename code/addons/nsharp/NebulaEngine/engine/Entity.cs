using System;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;
using System.Collections;
using System.Collections.Generic;
using Mathf;

namespace Nebula
{
    namespace Game
    {
        public class Entity
        {
            public Entity()
            {
                this.properties = new List<Property>();
            }

            private uint id;
            public uint Id { get { return id; } }

            private List<Property> properties;

            //bool IsAlive() { return Nebula.Game.IsAlive(this.id); }

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

            static void Destroy(Entity entity)
            {

            }

            public override string ToString()
            {
                return id.ToString();
            }
        }
    }
}
