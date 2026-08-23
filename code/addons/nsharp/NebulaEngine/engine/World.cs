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
            private Dictionary<UInt64, Entity> entityMap;
            
            public Entity CreateEntity()
            {
                UInt64 id = Api.CreateEntity(this.id);
                Entity entity = new Entity(this, id);
                this.RegisterEntity(entity);
                return entity;
            }

            public Entity GetEntity(UInt64 id)
            {
                if (this.entityMap.TryGetValue(id, out Entity entity))
                {
                    return entity;
                }

                // Lazily create entity if id is valid.
                if (Api.IsValid(id))
                {
                    Entity newEntity = new Entity(this, id);
                    this.RegisterEntity(newEntity);
                    return newEntity;
                }

                return null;
            }
            
            private void RegisterEntity(Entity entity)
            {
                this.entities.Add(entity);
            }

            public void DeregisterEntity(Entity entity)
            {
                this.entities.EraseSwap(entity);
                this.entityMap.Remove(entity.Id);
            }

            static World() { }
            private World(uint id)
            {
                this.id = id;
                this.entities = new List<Entity>();
                this.entityMap = new Dictionary<ulong, Entity>();
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
                        this.entities.EraseSwap(i);
                        this.entityMap.Remove(entity.Id);
                        Entity.Destroy(entity);
                        i--; // rerun same index, since we've swapped and erased.
                        continue;
                    }
                }
            }
        }
    }
}
