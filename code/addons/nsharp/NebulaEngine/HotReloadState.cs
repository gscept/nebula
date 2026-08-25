using System;
using System.Collections.Generic;
using System.Globalization;
using System.Reflection;
using Nebula.Game;

namespace Nebula
{
    /// <summary>Stable, scalar-only state captured across a full plugin reload.</summary>
    internal sealed class HotReloadState
    {
        internal sealed class PropertySnapshot
        {
            internal ulong EntityId;
            internal string TypeName;
            internal bool Active;
            internal HotReloadState State;
        }

        private sealed class Value
        {
            public Type Type;
            public string Text;
        }

        private readonly Dictionary<string, Value> values = new Dictionary<string, Value>();
        private readonly List<PropertySnapshot> properties = new List<PropertySnapshot>();

        internal List<PropertySnapshot> Properties { get { return this.properties; } }

        private const BindingFlags CAPTURE_FIELD_FLAGS = BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic;

        public void Capture(string root, object instance)
        {
            if (instance == null)
            {
                return;
            }
            this.CaptureFields(root, instance, new HashSet<object>());
        }

        public void Restore(string root, object instance)
        {
            if (instance == null)
                return;
            this.RestoreFields(root, instance, new HashSet<object>());
        }

        internal void CaptureProperties(World world)
        {
            if (world == null)
                return;

            List<Entity> entities = world.GetEntities();
            for (int i = 0; i < entities.Count; i++)
            {
                List<Property> entityProperties = entities[i].GetProperties();
                if (entityProperties == null)
                    continue;

                for (int j = 0; j < entityProperties.Count; j++)
                {
                    Property property = entityProperties[j];
                    HotReloadState propertyState = new HotReloadState();
                    propertyState.Capture("property", property);
                    this.properties.Add(new PropertySnapshot
                    {
                        EntityId = entities[i].Id,
                        TypeName = property.GetType().FullName,
                        Active = property.Active,
                        State = propertyState
                    });
                }
            }
        }

        internal void RestoreProperties(Assembly assembly, World world)
        {
            if (assembly == null || world == null)
                return;

            for (int i = 0; i < this.properties.Count; i++)
            {
                PropertySnapshot snapshot = this.properties[i];
                Entity entity = world.GetEntity(snapshot.EntityId);
                Type propertyType = assembly.GetType(snapshot.TypeName, false);

                if (entity == null || propertyType == null || !typeof(Property).IsAssignableFrom(propertyType))
                {
                    Console.Error.WriteLine("[NSharp] Could not restore property '" + snapshot.TypeName + "'.");
                    continue;
                }

                Property property = (Property)Activator.CreateInstance(propertyType);
                snapshot.State.Restore("property", property);
                entity.AddProperty(property, snapshot.Active);
            }
        }

        private void CaptureFields(string path, object instance, HashSet<object> visited)
        {
            if (instance == null || !visited.Add(instance))
                return;

            FieldInfo[] fields = instance.GetType().GetFields(CAPTURE_FIELD_FLAGS);
            for (int i = 0; i < fields.Length; i++)
            {
                FieldInfo field = fields[i];
                if (field.IsInitOnly
                    || field.IsLiteral
                    || field.GetCustomAttribute<NonSerializedAttribute>() != null
                    || field.IsStatic
                    || field.GetCustomAttribute<NoHotReload>() != null)
                {
                    continue;
                }

                object value = field.GetValue(instance);
                string fieldPath = path + "." + field.Name;

                if (value == null)
                {
                    continue;
                }

                Type type = field.FieldType;
                if (type == typeof(string)
                    || type.IsPrimitive
                    || type.IsEnum
                    || type == typeof(decimal))
                {
                    this.values[fieldPath] = new Value
                    {
                        Type = type,
                        Text = Convert.ToString(value, CultureInfo.InvariantCulture)
                    };
                }
                else if (!type.IsValueType && type.Assembly != typeof(HotReloadState).Assembly)
                {
                    this.CaptureFields(fieldPath, value, visited);
                }
            }
        }

        private void RestoreFields(string path, object instance, HashSet<object> visited)
        {
            if (instance == null || !visited.Add(instance))
                return;

            FieldInfo[] fields = instance.GetType().GetFields(CAPTURE_FIELD_FLAGS);
            for (int i = 0; i < fields.Length; i++)
            {
                FieldInfo field = fields[i];
                if (field.IsInitOnly
                    || field.IsLiteral
                    || field.GetCustomAttribute<NonSerializedAttribute>() != null
                    || field.IsStatic
                    || field.GetCustomAttribute<NoHotReload>() != null)
                {
                    continue;
                }

                string fieldPath = path + "." + field.Name;
                Value value;
                if (this.values.TryGetValue(fieldPath, out value))
                {
                    try
                    {
                        object converted = value.Text;
                        if (field.FieldType != typeof(string))
                        {
                            if (field.FieldType.IsEnum)
                            {
                                converted = Enum.Parse(field.FieldType, value.Text);
                            }
                            else
                            {
                                converted = Convert.ChangeType(value.Text, field.FieldType, CultureInfo.InvariantCulture);
                            }
                        }
                        field.SetValue(instance, converted);
                    }
                    catch (Exception)
                    {
                        // Schema changes leave the new field at its default value.
                    }
                    continue;
                }

                object child = field.GetValue(instance);

                if (child != null
                    && !field.FieldType.IsValueType
                    && field.FieldType.Assembly != typeof(HotReloadState).Assembly)
                {
                    this.RestoreFields(fieldPath, child, visited);
                }
            }
        }
    }

    //--------------------------------------------------------------------------
    /** <summary>
        Add to fields that should not persist between hot reloads.

        The field will be reverted to its default value after a reload.
        </summary>
    */
    [AttributeUsage(AttributeTargets.Field)]
    public sealed class NoHotReload : Attribute {}
}
