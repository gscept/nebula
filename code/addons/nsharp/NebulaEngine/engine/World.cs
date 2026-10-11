using System;
using System.Collections.Generic;

using Api = Nebula.Game.NebulaApiV1;

namespace Nebula
{
    namespace Game
    {
        public class World
        {
            public static readonly uint DEFAULT_WORLD = Api.GetDefaultWorldId();

            private readonly uint id;
            public uint Id => this.id;

            private readonly List<Entity> entities;
            private readonly Dictionary<UInt64, Entity> entityMap;

            static World() { }

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
                this.entityMap[entity.Id] = entity;
            }

            internal List<Entity> GetEntities()
            {
                return this.entities;
            }

            internal void ClearProperties()
            {
                for (int i = 0; i < this.entities.Count; i++)
                {
                    this.entities[i].ClearProperties();
                }
            }

            public void DeregisterEntity(Entity entity)
            {
                this.entities.EraseSwap(entity);
                _ = this.entityMap.Remove(entity.Id);
            }

            private World(uint id)
            {
                this.id = id;
                this.entities = [];
                this.entityMap = [];
            }

            private static readonly World tempDefaultWorld = new World(DEFAULT_WORLD);

            public static World Get(uint id)
            {
                // TODO: Get from native
                return tempDefaultWorld.id == id ? tempDefaultWorld : null;
            }

            public void CollectGarbage()
            {
                for (int i = 0; i < this.entities.Count; i++)
                {
                    Entity entity = this.entities[i];
                    if (!entity.IsValid())
                    {
                        this.entities.EraseSwap(i);
                        _ = this.entityMap.Remove(entity.Id);
                        Entity.Destroy(entity);
                        i--; // rerun same index, since we've swapped and erased.
                        continue;
                    }
                }
            }
        }
    }
}
