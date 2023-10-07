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
        public class World
        {
            public const uint DEFAULT_WORLD = 0;

            private uint id;
            public uint Id { get { return id; } }

            private List<Entity> entities;

            public void RegisterEntity(Entity entity)
            {
                this.entities.Add(entity);
            }


            static World() { }

            private World()
            {
                this.entities = new List<Entity>();
            }

            private static readonly World tempDefaultWorld = new World();
            
            public static World Get(uint id)
            {
                // TODO: Get from native
                return tempDefaultWorld;
            }
        }
    }
}
