using System;
using System.Collections.Generic;
using System.Runtime.Loader;

namespace Nebula
{
    internal sealed partial class HotReloadState
    {
        internal sealed class PropertyReplacement
        {
            internal Nebula.Game.Entity Entity;
            internal Nebula.Game.Property OldProperty;
            internal Nebula.Game.Property NewProperty;
            // What the old property was asked to be, rather than whether it was actually running. A
            // property in a world that is not simulating is not running, and the reattached one has to
            // still activate when that world starts simulating
            internal bool RequestedActive;
        }

        internal void PrepareProperties()
        {
            // A property can only be attached from C#, so the worlds the runtime has seen are the
            // ones that can hold one. The plugin is not limited to the default world, since the
            // native side mirrors every world it creates into the managed one.
            foreach (Nebula.Game.World world in Nebula.Game.World.AllWorlds())
            {
                List<Nebula.Game.Entity> entities = world.GetEntities();
                for (int entityIndex = 0; entityIndex < entities.Count; entityIndex++)
                {
                    Nebula.Game.Entity entity = entities[entityIndex];
                    List<Nebula.Game.Property> oldProperties = entity.GetProperties();
                    if (oldProperties == null)
                    {
                        continue;
                    }

                    for (int propertyIndex = 0; propertyIndex < oldProperties.Count; propertyIndex++)
                    {
                        this.PrepareProperty(entity, oldProperties[propertyIndex]);
                    }
                }
            }
        }

        private void PrepareProperty(Nebula.Game.Entity entity, Nebula.Game.Property oldProperty)
        {
            if (AssemblyLoadContext.GetLoadContext(oldProperty.GetType().Assembly) == this.newLoadContext)
            {
                this.properties.Add(new PropertyReplacement {
                    Entity = entity,
                    NewProperty = oldProperty,
                    RequestedActive = oldProperty.RequestedActive
                });
                return;
            }
            if (!this.IsSwappedAssembly(oldProperty.GetType().Assembly))
            {
                return;
            }

            Type newType = this.GetNewType(oldProperty.GetType());
            Nebula.Game.Property newProperty = (Nebula.Game.Property)this.Construct(newType);
            this.RegisterReplacement(oldProperty, newProperty);
            this.properties.Add(new PropertyReplacement {
                Entity = entity,
                OldProperty = oldProperty,
                NewProperty = newProperty,
                RequestedActive = oldProperty.RequestedActive
            });
        }
    }
}
