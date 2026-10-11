using System;
using System.Reflection;
using System.Threading.Tasks;

namespace Nebula
{
    internal sealed partial class HotReloadState
    {
        private bool TryMigrateRuntimeValue(object oldValue, Type newType, out object result)
        {
            if (oldValue is WeakReference reference)
            {
                result = this.MigrateWeakReference(reference);
            }
            else if (IsGenericType(oldValue.GetType(), typeof(WeakReference<>)))
            {
                result = this.MigrateGenericWeakReference(oldValue, newType);
            }
            else if (oldValue is Task task)
            {
                result = this.MigrateTask(task, newType);
            }
            else
            {
                result = null;
                return false;
            }
            return true;
        }

        private object MigrateWeakReference(WeakReference oldReference)
        {
            object target = oldReference.Target;
            WeakReference result = new WeakReference(null, oldReference.TrackResurrection);
            this.replacements.Add(oldReference, result);
            this.work.Enqueue(() => result.Target = this.MigrateValue(target, null));
            return result;
        }

        private object MigrateGenericWeakReference(object oldReference, Type newType)
        {
            MethodInfo tryGetTarget = oldReference.GetType().GetMethod("TryGetTarget");
            object[] arguments = [null];
            bool hasTarget = (bool)tryGetTarget.Invoke(oldReference, arguments);
            object target = hasTarget ? arguments[0] : null;
            Type targetType = newType.GetGenericArguments()[0];
            object result = Activator.CreateInstance(newType, [null]);
            this.replacements.Add(oldReference, result);
            MethodInfo setTarget = newType.GetMethod("SetTarget");
            this.work.Enqueue(() => setTarget.Invoke(result, [this.MigrateValue(target, targetType)]));
            return result;
        }

        private object MigrateTask(Task oldTask, Type newType)
        {
            if (!newType.IsGenericType)
            {
                return !oldTask.IsCompletedSuccessfully
                    ? throw new InvalidOperationException("A task referenced by plugin state must complete successfully before hot reload.")
                    : (object)Task.CompletedTask;
            }
            if (!oldTask.IsCompleted || oldTask.IsFaulted || oldTask.IsCanceled)
            {
                throw new InvalidOperationException("A task containing plugin state must complete before hot reload.");
            }

            object value = oldTask.GetType().GetProperty("Result").GetValue(oldTask);
            Type resultType = newType.GetGenericArguments()[0];
            object migrated = this.MigrateValue(value, resultType);
            MethodInfo fromResult = typeof(Task).GetMethod("FromResult").MakeGenericMethod(resultType);
            object result = fromResult.Invoke(null, [migrated]);
            this.replacements.Add(oldTask, result);
            return result;
        }

        private static bool IsGenericType(Type type, Type definition)
        {
            return type.IsGenericType && type.GetGenericTypeDefinition() == definition;
        }
    }
}
