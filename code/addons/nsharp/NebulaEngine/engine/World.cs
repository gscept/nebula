using System;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;
using System.Collections;
using System.Collections.Generic;

using Api = Nebula.Game.NebulaApiV1;

namespace Nebula
{
    namespace Game
    {
        public class World
        {
            public static readonly uint DEFAULT_WORLD = Api.GetDefaultWorldId();

            private uint id;
            public uint Id { get { return id; } }

            private List<Entity> entities;
            
            public Entity CreateEntity()
            {
                UInt64 id = Api.CreateEntity(this.id);
                Entity entity = new Entity(this, id);
                this.RegisterEntity(entity);
                return entity;
            }

            public Entity CreateEntity(string name)
            {
                return this.CreateEntity();
            }
            
            private void RegisterEntity(Entity entity)
            {
                this.entities.Add(entity);
            }

            static World() { }
            private World(uint id)
            {
                this.id = id;
                this.entities = new List<Entity>();
            }
            private static readonly World tempDefaultWorld = new World(DEFAULT_WORLD);
            
            public static World Get(uint id)
            {
                // TODO: Get from native
                if (tempDefaultWorld.id == id)
                    return tempDefaultWorld;
                else
                    return null;
            }

            public void CollectGarbage()
            {
                for (int i = 0; i < this.entities.Count; i++)
                {
                    Entity entity = this.entities[i];
                    if (!entity.IsValid())
                    {
                        Entity.Destroy(entity);
                        this.entities.EraseSwap(i);
                        i--; // rerun same index, since we've swapped and erased.
                        continue;
                    }
                }
            }
        }
    }
}
