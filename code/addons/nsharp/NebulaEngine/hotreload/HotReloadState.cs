using System;
using System.Collections.Generic;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.Loader;

namespace Nebula
{
    /// <summary>Migrates a live object graph between two versions of a plugin assembly.</summary>
    internal sealed partial class HotReloadState
    {
        private sealed class ReferenceComparer : IEqualityComparer<object>
        {
            internal static readonly ReferenceComparer Instance = new ReferenceComparer();

            public new bool Equals(object left, object right)
            {
                return Object.ReferenceEquals(left, right);
            }

            public int GetHashCode(object value)
            {
                return RuntimeHelpers.GetHashCode(value);
            }
        }

        private sealed class ManagedCallback
        {
            internal IHotReloadManaged OldValue;
            internal IHotReloadManaged NewValue;
            internal Dictionary<string, object> State;
        }

        private readonly Assembly oldAssembly;
        private readonly Assembly newAssembly;
        private readonly AssemblyLoadContext oldLoadContext;
        private readonly AssemblyLoadContext newLoadContext;
        private readonly Dictionary<object, object> replacements = new Dictionary<object, object>(ReferenceComparer.Instance);
        private readonly Queue<Action> work = new Queue<Action>();
        private readonly Queue<Action> lateWork = new Queue<Action>();
        private readonly List<ManagedCallback> callbacks = [];
        private readonly HashSet<object> queuedObjects = new HashSet<object>(ReferenceComparer.Instance);
        private readonly HashSet<object> persistedObjects = new HashSet<object>(ReferenceComparer.Instance);
        private readonly List<PropertyReplacement> properties = [];

        internal HotReloadState(Assembly oldAssembly, Assembly newAssembly)
        {
            this.oldAssembly = oldAssembly ?? throw new ArgumentNullException(nameof(oldAssembly));
            this.newAssembly = newAssembly ?? throw new ArgumentNullException(nameof(newAssembly));
            this.oldLoadContext = AssemblyLoadContext.GetLoadContext(oldAssembly);
            this.newLoadContext = AssemblyLoadContext.GetLoadContext(newAssembly);
        }

        internal IReadOnlyList<PropertyReplacement> Properties => this.properties;

        internal void RegisterReplacement(object oldValue, object newValue)
        {
            if (oldValue == null || newValue == null)
            {
                throw new ArgumentNullException(oldValue == null ? nameof(oldValue) : nameof(newValue));
            }

            this.replacements[oldValue] = newValue;
        }

        internal void Migrate(object oldPlugin, object newPlugin, object oldApplication, object newApplication)
        {
            try
            {
                this.RegisterReplacement(oldPlugin, newPlugin);
                this.RegisterReplacement(oldApplication, newApplication);

                this.QueueObjectFields(oldPlugin, newPlugin);
                this.QueueObjectFields(oldApplication, newApplication);
                for (int i = 0; i < this.properties.Count; i++)
                {
                    if (this.properties[i].OldProperty != null)
                    {
                        this.QueueObjectFields(this.properties[i].OldProperty, this.properties[i].NewProperty);
                    }
                }

                this.MigrateStaticFields();
                int callbackIndex = 0;
                while (true)
                {
                    this.DrainWork();
                    if (callbackIndex == this.callbacks.Count)
                    {
                        break;
                    }

                    // Migrating callback state can discover objects with callbacks of their own.
                    ManagedCallback callback = this.callbacks[callbackIndex++];
                    string[] keys = new string[callback.State.Count];
                    callback.State.Keys.CopyTo(keys, 0);
                    for (int keyIndex = 0; keyIndex < keys.Length; keyIndex++)
                    {
                        callback.State[keys[keyIndex]] = this.MigrateValue(callback.State[keys[keyIndex]], null);
                    }
                }
            }
            catch
            {
                this.NotifyFailure();
                throw;
            }
        }

        internal void Complete()
        {
            for (int i = 0; i < this.callbacks.Count; i++)
            {
                this.callbacks[i].NewValue.Created(this.callbacks[i].State);
            }

            this.callbacks.Clear();
        }

        private void DrainWork()
        {
            while (this.work.Count > 0 || this.lateWork.Count > 0)
            {
                while (this.work.Count > 0)
                {
                    this.work.Dequeue()();
                }

                // Hash-based containers must see populated keys. A late task may
                // discover more ordinary work; finish it before the next late task.
                if (this.lateWork.Count > 0)
                {
                    this.lateWork.Dequeue()();
                }
            }
        }

        private void NotifyFailure()
        {
            for (int i = 0; i < this.callbacks.Count; i++)
            {
                try
                {
                    this.callbacks[i].OldValue.Failed();
                }
                catch (Exception exception)
                {
                    Console.Error.WriteLine("[NSharp] Hot reload failure callback failed: " + exception);
                }
            }
        }

        private object MigrateValue(object oldValue, Type destinationType)
        {
            if (oldValue == null)
            {
                return null;
            }

            Type oldType = oldValue.GetType();
            if (!oldType.IsValueType && this.replacements.TryGetValue(oldValue, out object replacement))
            {
                ValidateDestinationType(replacement.GetType(), destinationType);
                return replacement;
            }

            Type newType;
            try
            {
                newType = this.GetNewType(oldType);
            }
            catch (TypeLoadException)
            {
                if (oldValue is IHotReloadManaged failed)
                {
                    failed.Failed();
                }

                throw;
            }

            ValidateDestinationType(newType, destinationType);
            if (oldType.GetCustomAttribute<NoHotReloadAttribute>() != null)
            {
                return this.MigrateResetValue(oldValue, newType);
            }

            if (oldType.IsEnum)
            {
                return MigrateEnum(oldValue, oldType, newType);
            }

            if (IsImmutable(oldType))
            {
                return oldValue;
            }

            if (this.TryMigrateReflectionValue(oldValue, newType, out replacement))
            {
                return replacement;
            }

            if (this.TryMigrateRuntimeValue(oldValue, newType, out replacement))
            {
                return replacement;
            }

            return this.TryMigrateCollectionValue(oldValue, newType, out replacement)
                ? replacement
                : this.MigrateObject(oldValue, oldType, newType);
        }

    }
}
