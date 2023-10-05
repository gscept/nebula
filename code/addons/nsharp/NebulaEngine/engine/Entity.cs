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
        public struct Entity
        {
            uint id;

            //bool IsAlive() { return Nebula.Game.IsAlive(this.id); }

            // This is for getting and setting unmanaged components
            public bool HasComponent<T>() { return false; }
            public void SetComponent<T>(T component) { return; }
            public T GetComponent<T>() { return default(T); }

            // These are for managed properties
            public bool HasProperty<T>() { return false; }
            public void AddProperty(Property property)
            {
                property.Entity = this;
            }
            public T GetProperty<T>() { return default(T); }

            public Matrix GetTransform() { return default(Matrix); }
            public void SetTransform(Matrix matrix) { return; }

            public void Send(in Msg msg)
            {
                // TODO: implement me!
            }

            static void Destroy(Entity entity) { }

            public override string ToString()
            {
                return id.ToString();
            }

            //private List<Property> properties;
        }
    }
}
