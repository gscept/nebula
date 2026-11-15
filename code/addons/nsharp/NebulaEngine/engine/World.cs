using System;
using System.Collections.Generic;

using Api = Nebula.Game.NebulaApiV1;

namespace Nebula
{
    namespace Game
    {
        /// <summary>
        /// Worlds hold entities. 
        /// </summary>
        /// <remark>
        /// Note that the managed side of Nebula Worlds (C# worlds) is basically a mirror of the native side, which has authority.
        /// </remark>
        public class World
        {
            public static readonly uint DEFAULT_WORLD = Api.GetDefaultWorldId();

            private readonly uint id;
            public uint Id => this.id;

            private readonly List<Entity> entities;
            private readonly Dictionary<UInt64, Entity> entityMap;

            // Native world ids to World objects
            private static readonly Dictionary<uint, World> worlds = [];

            static World()
            {
                worlds[DEFAULT_WORLD] = new World(DEFAULT_WORLD);
            }

            /// <summary>
            /// The entities in the world.
            /// </summary>
            public IReadOnlyList<Entity> Entities => this.entities;

            public Entity CreateEntity()
            {
                UInt64 id = Api.CreateEntity(this.id);
                Entity entity = new Entity(this, id);
                this.RegisterEntity(entity);
                return entity;
            }

            /// <summary>
            /// Get an entity by id, or null if it doesn't exist. 
            /// </summary>
            /// <remark>
            /// Entities created in native code become available once the entity queue has been flushed at the beginning of
            /// the frame, so an entity created during the frame is not in the mirror until the next one.
            /// </remark>
            public Entity GetEntity(UInt64 id)
            {
                if (this.entityMap.TryGetValue(id, out Entity entity))
                {
                    return entity;
                }

                return null;
            }

            /// <summary>
            /// Add an entity reported by the native entity callback. Entities that are already mirrored
            /// are skipped, which is what keeps entities created from C# from being added twice.
            /// </summary>
            internal Entity RegisterEntityFromNative(UInt64 id)
            {
                if (this.entityMap.ContainsKey(id))
                {
                    return null;
                }

                Entity entity = new Entity(this, id);
                this.RegisterEntity(entity);
                return entity;
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

            /// <summary>
            /// Check if a world id exists in the native game server.
            /// </summary>
            public static bool HasWorld(uint id)
            {
                return Api.HasWorld(id);
            }

            private World(uint id)
            {
                this.id = id;
                this.entities = [];
                this.entityMap = [];
            }

            /// <summary>
            /// Get a world by its native world id. Returns null if the world does not exist.
            /// </summary>
            public static World Get(uint id)
            {
                if (worlds.TryGetValue(id, out World world))
                {
                    return world;
                }

                if (!Api.HasWorld(id))
                {
                    return null;
                }

                world = new World(id);
                worlds[id] = world;
                return world;
            }

            /// <summary>
            /// Removes entities that no longer exist in the native world.
            /// </summary>
            public void CollectGarbage()
            {
                for (int i = 0; i < this.entities.Count; i++)
                {
                    Entity entity = this.entities[i];
                    if (!entity.IsValid())
                    {
                        // Destroy deregisters from the mirror, so the element at i is replaced by the
                        // last one and has to be revisited.
                        Entity.Destroy(entity);
                        i--;
                    }
                }
            }
        }
    }
}
