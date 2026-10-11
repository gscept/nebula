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
            internal bool Active;
        }

        internal void PrepareProperties(Nebula.Game.World world)
        {
            if (world == null)
            {
                return;
            }

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

        private void PrepareProperty(Nebula.Game.Entity entity, Nebula.Game.Property oldProperty)
        {
            if (AssemblyLoadContext.GetLoadContext(oldProperty.GetType().Assembly) == this.newLoadContext)
            {
                this.properties.Add(new PropertyReplacement {
                    Entity = entity,
                    NewProperty = oldProperty,
                    Active = oldProperty.Active
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
                Active = oldProperty.Active
            });
        }
    }
}
