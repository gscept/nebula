using System;
using System.Collections;
using System.Collections.Generic;
using System.Reflection;
using System.Runtime.CompilerServices;

namespace Nebula
{
    internal sealed partial class HotReloadState
    {
        private bool TryMigrateCollectionValue(object oldValue, Type newType, out object result)
        {
            if (oldValue is Array array)
            {
                result = this.MigrateArray(array, newType);
                return true;
            }
            if (oldValue is IDictionary dictionary)
            {
                result = this.MigrateDictionary(dictionary, newType);
                return true;
            }
            if (this.TryMigrateGenericDictionary(oldValue, newType, out result))
            {
                return true;
            }

            if (oldValue is IList list)
            {
                result = this.MigrateList(list, newType);
                return true;
            }
            return this.TryMigrateEnumerableCollection(oldValue, newType, out result);
        }

        private object MigrateArray(Array oldArray, Type newType)
        {
            Type elementType = newType.GetElementType();
            int rank = oldArray.Rank;
            int[] lengths = new int[rank];
            int[] lowerBounds = new int[rank];
            for (int i = 0; i < rank; i++)
            {
                lengths[i] = oldArray.GetLength(i);
                lowerBounds[i] = oldArray.GetLowerBound(i);
            }

            Array result = Array.CreateInstance(elementType, lengths, lowerBounds);
            this.replacements.Add(oldArray, result);
            int[] indices = (int[])lowerBounds.Clone();
            this.work.Enqueue(() => this.PopulateArray(oldArray, result, elementType, lengths, lowerBounds, indices));
            return result;
        }

        private void PopulateArray(Array source, Array destination, Type elementType, int[] lengths, int[] lowerBounds, int[] indices)
        {
            if (source.Length == 0)
            {
                return;
            }

            while (true)
            {
                destination.SetValue(this.MigrateValue(source.GetValue(indices), elementType), indices);
                int dimension = source.Rank - 1;
                while (dimension >= 0)
                {
                    indices[dimension]++;
                    if (indices[dimension] < lowerBounds[dimension] + lengths[dimension])
                    {
                        break;
                    }

                    indices[dimension] = lowerBounds[dimension];
                    dimension--;
                }
                if (dimension < 0)
                {
                    return;
                }
            }
        }

        private object MigrateList(IList oldList, Type newType)
        {
            IList result = (IList)this.ConstructCollection(newType, null);
            this.replacements.Add(oldList, result);
            this.QueueListContents(oldList, result);
            return result;
        }

        private void QueueListContents(IList source, IList destination)
        {
            Type elementType = GetCollectionArguments(destination.GetType(), typeof(IList<>))[0];
            this.work.Enqueue(() => {
                for (int i = 0; i < source.Count; i++)
                {
                    _ = destination.Add(this.MigrateValue(source[i], elementType));
                }
            });
        }

        private object MigrateDictionary(IDictionary oldDictionary, Type newType)
        {
            object comparer = this.MigrateComparer(oldDictionary);
            IDictionary result = (IDictionary)this.ConstructCollection(newType, comparer);
            this.replacements.Add(oldDictionary, result);

            this.QueueDictionaryContents(oldDictionary, result);
            return result;
        }

        private void QueueDictionaryContents(IDictionary source, IDictionary destination)
        {
            Type[] arguments = GetCollectionArguments(destination.GetType(), typeof(IDictionary<,>));
            List<KeyValuePair<object, object>> entries = [];
            foreach (DictionaryEntry entry in source)
            {
                entries.Add(new KeyValuePair<object, object>(
                    this.MigrateValue(entry.Key, arguments[0]),
                    this.MigrateValue(entry.Value, arguments[1])));
            }

            this.lateWork.Enqueue(() => {
                for (int i = 0; i < entries.Count; i++)
                {
                    destination.Add(entries[i].Key, entries[i].Value);
                }
            });
        }

        private bool TryMigrateGenericDictionary(object oldValue, Type newType, out object result)
        {
            result = null;
            Type dictionaryInterface = FindGenericInterface(oldValue.GetType(), typeof(IDictionary<,>));
            bool isConditionalWeakTable = oldValue.GetType().IsGenericType
                && oldValue.GetType().GetGenericTypeDefinition() == typeof(ConditionalWeakTable<,>);
            if ((dictionaryInterface == null && !isConditionalWeakTable) || oldValue is IDictionary)
            {
                return false;
            }

            object comparer = this.MigrateComparer(oldValue);
            result = this.ConstructCollection(newType, comparer);
            this.replacements.Add(oldValue, result);

            Type[] arguments = isConditionalWeakTable
                ? newType.GetGenericArguments()
                : GetCollectionArguments(newType, typeof(IDictionary<,>));
            MethodInfo add = (newType.GetMethod("TryAdd", BindingFlags.Instance | BindingFlags.Public, null, arguments, null)
                ?? newType.GetMethod("Add", BindingFlags.Instance | BindingFlags.Public, null, arguments, null)) ?? throw new InvalidOperationException("Hot reload does not know how to populate '" + newType + "'.");

            List<KeyValuePair<object, object>> entries = [];
            foreach (object entry in (IEnumerable)oldValue)
            {
                Type entryType = entry.GetType();
                object key = entryType.GetProperty("Key").GetValue(entry);
                object value = entryType.GetProperty("Value").GetValue(entry);
                entries.Add(new KeyValuePair<object, object>(
                    this.MigrateValue(key, arguments[0]),
                    this.MigrateValue(value, arguments[1])));
            }

            object destination = result;
            this.lateWork.Enqueue(() => {
                for (int i = 0; i < entries.Count; i++)
                {
                    _ = add.Invoke(destination, [entries[i].Key, entries[i].Value]);
                }
            });
            return true;
        }

        private bool TryMigrateEnumerableCollection(object oldValue, Type newType, out object result)
        {
            result = null;
            if (oldValue is not IEnumerable)
            {
                return false;
            }

            MethodInfo addMethod = FindCollectionMethod(newType, "Add")
                ?? FindCollectionMethod(newType, "Enqueue")
                ?? FindCollectionMethod(newType, "Push")
                ?? FindCollectionMethod(newType, "AddLast");
            if (addMethod == null)
            {
                return false;
            }

            object comparer = this.MigrateComparer(oldValue);
            result = this.ConstructCollection(newType, comparer);
            this.replacements.Add(oldValue, result);
            object destination = result;
            Type elementType = addMethod.GetParameters()[0].ParameterType;
            List<object> values = [];
            foreach (object item in (IEnumerable)oldValue)
            {
                values.Add(this.MigrateValue(item, elementType));
            }

            void populate()
            {
                if (addMethod.Name == "Push")
                {
                    values.Reverse();
                }

                for (int i = 0; i < values.Count; i++)
                {
                    _ = addMethod.Invoke(destination, [values[i]]);
                }
            }

            if (IsSetType(oldValue.GetType()))
            {
                this.lateWork.Enqueue(populate);
            }
            else
            {
                this.work.Enqueue(populate);
            }

            return true;
        }

        private object MigrateComparer(object collection)
        {
            object comparer = GetComparer(collection);
            return this.MigrateValue(comparer, comparer == null ? null : this.GetNewType(comparer.GetType()));
        }

        private bool MigrateIntoReadonlyContainer(object source, object destination)
        {
            if (source == null || destination == null)
            {
                return source == null && destination == null;
            }

            if (IsImmutable(source.GetType()))
            {
                return Object.Equals(source, destination);
            }

            if (destination is IList destinationList && source is IList sourceList)
            {
                destinationList.Clear();
                this.replacements[source] = destination;
                this.QueueListContents(sourceList, destinationList);
                return true;
            }
            if (destination is IDictionary destinationDictionary && source is IDictionary sourceDictionary)
            {
                destinationDictionary.Clear();
                this.replacements[source] = destination;
                this.QueueDictionaryContents(sourceDictionary, destinationDictionary);
                return true;
            }

            if (this.IsSwappedAssembly(source.GetType().Assembly) && destination.GetType() == this.GetNewType(source.GetType()))
            {
                this.RegisterReplacement(source, destination);
                this.QueueObjectFields(source, destination);
                return true;
            }
            return false;
        }

        private object ConstructCollection(Type type, object comparer)
        {
            if (comparer != null)
            {
                ConstructorInfo[] constructors = type.GetConstructors(BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance);
                for (int i = 0; i < constructors.Length; i++)
                {
                    ParameterInfo[] parameters = constructors[i].GetParameters();
                    if (parameters.Length == 1 && parameters[0].ParameterType.IsInstanceOfType(comparer))
                    {
                        return constructors[i].Invoke([comparer]);
                    }
                }
            }
            return this.Construct(type);
        }

        private static object GetComparer(object collection)
        {
            PropertyInfo comparer = collection.GetType().GetProperty("Comparer", BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic)
                ?? collection.GetType().GetProperty("EqualityComparer", BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic);
            return comparer == null ? null : comparer.GetValue(collection);
        }

        private static Type[] GetCollectionArguments(Type type, Type contract)
        {
            // The concrete type may be non-generic, or have unrelated type parameters.
            Type collectionInterface = FindGenericInterface(type, contract);
            return collectionInterface != null ? collectionInterface.GetGenericArguments() : [typeof(object), typeof(object)];
        }

        private static MethodInfo FindCollectionMethod(Type type, string name)
        {
            MethodInfo[] methods = type.GetMethods(BindingFlags.Instance | BindingFlags.Public);
            for (int i = 0; i < methods.Length; i++)
            {
                if (methods[i].Name == name && methods[i].GetParameters().Length == 1)
                {
                    return methods[i];
                }
            }
            return null;
        }

        private static Type FindGenericInterface(Type type, Type genericDefinition)
        {
            if (type.IsGenericType && type.GetGenericTypeDefinition() == genericDefinition)
            {
                return type;
            }

            Type[] interfaces = type.GetInterfaces();
            for (int i = 0; i < interfaces.Length; i++)
            {
                if (interfaces[i].IsGenericType && interfaces[i].GetGenericTypeDefinition() == genericDefinition)
                {
                    return interfaces[i];
                }
            }
            return null;
        }

        private static bool IsSetType(Type type)
        {
            return FindGenericInterface(type, typeof(ISet<>)) != null;
        }
    }
}
