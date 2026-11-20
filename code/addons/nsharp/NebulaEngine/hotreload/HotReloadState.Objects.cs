using System;
using System.Collections.Generic;
using System.Reflection;
using System.Runtime.CompilerServices;

namespace Nebula
{
    internal sealed partial class HotReloadState
    {
        private static void ValidateDestinationType(Type newType, Type destinationType)
        {
            if (destinationType != null
                && !destinationType.IsAssignableFrom(newType)
                && Nullable.GetUnderlyingType(destinationType) != newType)
            {
                throw new InvalidOperationException("Hot reload cannot assign '" + newType + "' to '" + destinationType + "'.");
            }
        }

        private object MigrateResetValue(object oldValue, Type newType)
        {
            object replacement = this.Construct(newType);
            if (!oldValue.GetType().IsValueType)
            {
                this.replacements.Add(oldValue, replacement);
                this.RecordCallback(oldValue, replacement);
            }
            return replacement;
        }

        private static object MigrateEnum(object oldValue, Type oldType, Type newType)
        {
            if (oldType == newType)
            {
                return oldValue;
            }

            object underlying = Convert.ChangeType(oldValue, Enum.GetUnderlyingType(oldType));
            return Enum.ToObject(newType, underlying);
        }

        private object MigrateObject(object oldValue, Type oldType, Type newType)
        {
            if (oldType.IsValueType)
            {
                if (oldType == newType)
                {
                    return oldValue;
                }

                object boxed = this.Construct(newType);
                this.CopyObjectFields(oldValue, boxed);
                return boxed;
            }

            if (oldType != newType && !this.IsSwappedAssembly(oldType.Assembly))
            {
                object replacement = this.Construct(newType);
                this.replacements.Add(oldValue, replacement);
                this.QueueObjectFields(oldValue, replacement);
                return replacement;
            }

            if (!this.IsSwappedAssembly(oldType.Assembly))
            {
                if (oldValue is IHotReloadManaged persisted && this.persistedObjects.Add(oldValue))
                {
                    persisted.Persisted();
                }

                return oldValue;
            }

            RequireExported(oldType);
            object result = this.Construct(newType);
            this.replacements.Add(oldValue, result);
            this.QueueObjectFields(oldValue, result);
            return result;
        }

        private void QueueObjectFields(object oldValue, object newValue)
        {
            if (!this.queuedObjects.Add(oldValue))
            {
                return;
            }

            this.RecordCallback(oldValue, newValue);
            this.work.Enqueue(() => this.CopyObjectFields(oldValue, newValue));
        }

        private void RecordCallback(object oldValue, object newValue)
        {
            if (oldValue is not IHotReloadManaged oldManaged || newValue is not IHotReloadManaged newManaged)
            {
                return;
            }

            Dictionary<string, object> state = [];
            this.callbacks.Add(new ManagedCallback { OldValue = oldManaged, NewValue = newManaged, State = state });
            oldManaged.Destroyed(state);
        }

        private void CopyObjectFields(object oldValue, object newValue)
        {
            Type oldLevel = oldValue.GetType();
            Type newLevel = newValue.GetType();
            bool copyEveryLevel = !this.IsSwappedAssembly(oldLevel.Assembly) && oldLevel != newLevel;

            while (oldLevel != null && newLevel != null)
            {
                if (this.IsSwappedAssembly(oldLevel.Assembly) || copyEveryLevel)
                {
                    this.CopyDeclaredFields(oldValue, newValue, oldLevel, newLevel);
                }

                oldLevel = oldLevel.BaseType;
                newLevel = newLevel.BaseType;
            }
        }

        private void CopyDeclaredFields(object oldValue, object newValue, Type oldLevel, Type newLevel)
        {
            FieldInfo[] oldFields = oldLevel.GetFields(BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.DeclaredOnly);
            for (int i = 0; i < oldFields.Length; i++)
            {
                FieldInfo oldField = oldFields[i];
                if (ShouldSkip(oldField))
                {
                    continue;
                }

                FieldInfo newField = newLevel.GetField(oldField.Name, BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.DeclaredOnly);
                if (newField == null || ShouldSkip(newField))
                {
                    continue;
                }

                try
                {
                    object migrated = this.MigrateValue(oldField.GetValue(oldValue), newField.FieldType);
                    newField.SetValue(newValue, migrated);
                }
                catch (Exception exception)
                {
                    throw new InvalidOperationException("Could not migrate field '" + oldLevel.FullName + "." + oldField.Name + "'.", exception);
                }
            }
        }

        private static bool ShouldSkip(FieldInfo field)
        {
            return field.IsLiteral || field.FieldType.IsPointer || field.GetCustomAttribute<NoHotReloadAttribute>() != null || BackingPropertyHasNoHotReload(field);
        }

        private static bool BackingPropertyHasNoHotReload(FieldInfo field)
        {
            if (!field.Name.StartsWith("<", StringComparison.Ordinal) || !field.Name.EndsWith(">k__BackingField", StringComparison.Ordinal))
            {
                return false;
            }

            int end = field.Name.IndexOf('>');
            PropertyInfo property = field.DeclaringType.GetProperty(field.Name.Substring(1, end - 1), BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public | BindingFlags.NonPublic);
            return property != null && property.GetCustomAttribute<NoHotReloadAttribute>() != null;
        }

        private static void RequireExported(Type type)
        {
            if (!IsExportedOrGenerated(type))
            {
                throw new InvalidOperationException("Plugin type '" + type.FullName + "' must have [ExportType] to survive hot reload.");
            }
        }

        private static bool IsExportedOrGenerated(Type type)
        {
            return type.GetCustomAttribute<ExportTypeAttribute>() != null
                || type.GetCustomAttribute<CompilerGeneratedAttribute>() != null;
        }

        private static bool IsStaticExportType(Type type)
        {
            return type.GetCustomAttribute<ExportTypeAttribute>() != null || (type.GetCustomAttribute<CompilerGeneratedAttribute>() != null
                && type.DeclaringType != null
                && IsStaticExportType(type.DeclaringType));
        }

        private static bool IsImmutable(Type type)
        {
            return type == typeof(string)
                || type.IsPrimitive
                || type == typeof(decimal)
                || type == typeof(DateTime)
                || type == typeof(DateTimeOffset)
                || type == typeof(TimeSpan)
                || type == typeof(Guid)
                || type == typeof(Uri);
        }
    }
}
