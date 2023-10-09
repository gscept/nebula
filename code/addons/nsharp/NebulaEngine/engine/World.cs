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
        public class World
        {
            public static readonly uint DEFAULT_WORLD = Api.GetDefaultWorldId();

            private uint id;
            public uint Id { get { return id; } }

            private List<Entity> entities;
            
            public Entity CreateEntity(string template)
            {
                uint id = Api.CreateEntity(this.id, template);
                Entity entity = new Entity(this, id);
                this.RegisterEntity(entity);

                // TODO: Register a special flag ScriptComponent that is managed so that we can react to this entity being deleted from native code.

                // TODO: Check what properties this entity should have, and add them to the entity.
                return entity;
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
