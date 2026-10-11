using System;
using System.Reflection;
using System.Runtime.CompilerServices;

namespace Nebula
{
    internal sealed partial class HotReloadState
    {
        private void MigrateStaticFields()
        {
            foreach (Assembly assembly in this.oldLoadContext.Assemblies)
            {
                Type[] oldTypes = assembly.GetTypes();
                for (int typeIndex = 0; typeIndex < oldTypes.Length; typeIndex++)
                {
                    Type oldType = oldTypes[typeIndex];
                    if (oldType.ContainsGenericParameters || !IsStaticExportType(oldType))
                    {
                        continue;
                    }

                    this.MigrateStaticFields(oldType, this.GetNewType(oldType));
                }
            }
        }

        private void MigrateStaticFields(Type oldType, Type newType)
        {
            RuntimeHelpers.RunClassConstructor(newType.TypeHandle);
            FieldInfo[] fields = oldType.GetFields(BindingFlags.Static | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.DeclaredOnly);
            for (int i = 0; i < fields.Length; i++)
            {
                FieldInfo oldField = fields[i];
                if (ShouldSkip(oldField))
                {
                    continue;
                }

                FieldInfo newField = newType.GetField(oldField.Name, BindingFlags.Static | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.DeclaredOnly);
                if (newField == null || ShouldSkip(newField))
                {
                    continue;
                }

                if (newField.IsInitOnly)
                {
                    if (!this.MigrateIntoReadonlyContainer(oldField.GetValue(null), newField.GetValue(null)))
                    {
                        throw new InvalidOperationException("Static readonly field '" + oldType.FullName + "." + oldField.Name + "' cannot be replaced during hot reload.");
                    }

                    continue;
                }
                newField.SetValue(null, this.MigrateValue(oldField.GetValue(null), newField.FieldType));
            }
        }
    }
}
