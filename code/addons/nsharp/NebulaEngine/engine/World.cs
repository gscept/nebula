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

            // The mode as of the last frame, kept only to know when it changed. The mode itself is read from
            // the native world rather than cached, because the editor changes it between frames and then
            // applies the properties it copied in the same call, and an activation decided there has to see
            // the mode that is actually set
            private bool lastSimulating;

            /// <summary>
            /// Whether this world is simulating
            /// </summary>
            public bool IsSimulating()
            {
                return Api.IsSimulating(this.id);
            }

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

            /// <summary>
            /// The worlds the managed runtime has seen. A native world the plugin has not looked at
            /// is not in here, and cannot hold a managed property.
            /// </summary>
            internal static IReadOnlyCollection<World> AllWorlds()
            {
                return worlds.Values;
            }

            /// <summary>
            /// Detaches properties from every world the managed runtime knows about. A property can
            /// only be attached from C#, so the worlds the runtime has seen are the only ones that
            /// can hold one.
            /// </summary>
            internal static void ClearAllProperties()
            {
                foreach (World world in AllWorlds())
                {
                    world.ClearProperties();
                }
            }

            /// <summary>
            /// Reads the mode from the native world and runs the activation pass when it changed. This is
            /// where a property is activated when its world starts simulating and deactivated when it stops,
            /// because a property that was asked to be active only actually runs in a simulating world.
            /// </summary>
            public void RefreshMode()
            {
                bool simulating = this.IsSimulating();
                if (simulating == this.lastSimulating)
                {
                    return;
                }

                this.lastSimulating = simulating;

                for (int i = 0; i < this.entities.Count; i++)
                {
                    Entity entity = this.entities[i];
                    List<Property> properties = entity.GetProperties();
                    if (properties == null)
                    {
                        continue;
                    }

                    // Activation can read a component, so it does not run against an entity that has no
                    // instance, which is an id something is holding rather than an entity in the world
                    if (!entity.HasInstance())
                    {
                        continue;
                    }

                    for (int j = 0; j < properties.Count; j++)
                    {
                        Property property = properties[j];
                        if (simulating)
                        {
                            if (property.RequestedActive)
                            {
                                property.Activate();
                            }
                        }
                        else
                        {
                            property.Deactivate();
                        }
                    }
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

                // Read the mode at construction, so a property attached before the first frame is activated
                // in a world that is already simulating
                this.lastSimulating = Api.IsSimulating(id);
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

            /// <summary>
            /// Drops the world on the managed side. Called when the native world is destroyed
            /// </summary>
            public void Clear()
            {
                // the slot the world lived in can be taken by the next world, and a new world starting from index 0 with
                // generation 0 produces the same entity ids. Nothing is deleted through the api here,
                // since the native entities are gone with the world.

                for (int i = 0; i < this.entities.Count; i++)
                {
                    this.entities[i].Detach();
                }

                this.entities.Clear();
                this.entityMap.Clear();
                worlds.Remove(this.id);
            }
        }
    }
}
