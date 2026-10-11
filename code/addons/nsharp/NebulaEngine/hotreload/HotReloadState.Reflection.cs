using System;
using System.Reflection;
using System.Runtime.Loader;

namespace Nebula
{
    internal sealed partial class HotReloadState
    {
        private bool TryMigrateReflectionValue(object oldValue, Type newType, out object result)
        {
            if (oldValue is Assembly assembly)
            {
                result = this.GetNewAssembly(assembly);
            }
            else if (oldValue is Module module)
            {
                result = this.MigrateModule(module);
            }
            else if (oldValue is Type type)
            {
                result = this.GetNewType(type);
            }
            else if (oldValue is MemberInfo info)
            {
                result = this.MigrateMember(info);
            }
            else if (oldValue is ParameterInfo paramInfo)
            {
                result = this.MigrateParameter(paramInfo);
            }
            else if (oldValue is Delegate deleg)
            {
                result = this.MigrateDelegate(deleg, newType);
            }
            else
            {
                result = null;
                return false;
            }
            return true;
        }

        private object MigrateModule(Module module)
        {
            Assembly assembly = this.GetNewAssembly(module.Assembly);
            return assembly == module.Assembly ? module : assembly.GetModule(module.Name);
        }

        private object MigrateDelegate(Delegate oldDelegate, Type newType)
        {
            if (this.replacements.TryGetValue(oldDelegate, out object cached))
            {
                return cached;
            }

            Delegate result = null;
            Delegate[] invocationList = oldDelegate.GetInvocationList();
            for (int i = 0; i < invocationList.Length; i++)
            {
                Delegate part = invocationList[i];
                object target = this.MigrateValue(part.Target, part.Target == null ? null : this.GetNewType(part.Target.GetType()));
                MethodInfo method = (MethodInfo)this.MigrateMember(part.Method);
                Delegate migrated = target == null
                    ? Delegate.CreateDelegate(newType, method, true)
                    : Delegate.CreateDelegate(newType, target, method, true);
                result = Delegate.Combine(result, migrated);
            }
            this.replacements.Add(oldDelegate, result);
            return result;
        }

        private object MigrateMember(MemberInfo member)
        {
            Type declaringType = member.DeclaringType == null ? null : this.GetNewType(member.DeclaringType);
            if (declaringType == null || declaringType == member.DeclaringType)
            {
                return member;
            }

            BindingFlags flags = BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance | BindingFlags.Static;
            return member switch
            {
                FieldInfo => declaringType.GetField(member.Name, flags),
                PropertyInfo => declaringType.GetProperty(member.Name, flags),
                EventInfo => declaringType.GetEvent(member.Name, flags),
                ConstructorInfo constructor => this.FindConstructor(declaringType, constructor),
                MethodInfo method => this.FindMethod(declaringType, method),
                _ => member
            };
        }

        private object MigrateParameter(ParameterInfo parameter)
        {
            MethodBase member = (MethodBase)this.MigrateMember(parameter.Member);
            if (parameter.Position < 0)
            {
                return ((MethodInfo)member).ReturnParameter;
            }

            ParameterInfo[] parameters = member.GetParameters();
            return parameter.Position < parameters.Length ? parameters[parameter.Position] : null;
        }

        private MethodInfo FindMethod(Type declaringType, MethodInfo oldMethod)
        {
            BindingFlags flags = BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance | BindingFlags.Static;
            MethodInfo[] methods = declaringType.GetMethods(flags);
            ParameterInfo[] oldParameters = oldMethod.GetParameters();
            for (int i = 0; i < methods.Length; i++)
            {
                MethodInfo candidate = methods[i];
                ParameterInfo[] candidateParameters = candidate.GetParameters();
                if (candidate.Name != oldMethod.Name || candidateParameters.Length != oldParameters.Length || candidate.GetGenericArguments().Length != oldMethod.GetGenericArguments().Length)
                {
                    continue;
                }

                if (!this.ParametersMatch(oldParameters, candidateParameters))
                {
                    continue;
                }

                if (oldMethod.IsGenericMethod && !oldMethod.IsGenericMethodDefinition)
                {
                    Type[] arguments = oldMethod.GetGenericArguments();
                    for (int argumentIndex = 0; argumentIndex < arguments.Length; argumentIndex++)
                    {
                        arguments[argumentIndex] = this.GetNewType(arguments[argumentIndex]);
                    }

                    return candidate.MakeGenericMethod(arguments);
                }
                return candidate;
            }
            throw new MissingMethodException(declaringType.FullName, oldMethod.Name);
        }

        private bool ParametersMatch(ParameterInfo[] oldParameters, ParameterInfo[] newParameters)
        {
            for (int i = 0; i < oldParameters.Length; i++)
            {
                if (!this.AreEquivalentTypes(oldParameters[i].ParameterType, newParameters[i].ParameterType))
                {
                    return false;
                }
            }
            return true;
        }

        private bool AreEquivalentTypes(Type oldType, Type newType)
        {
            return oldType.IsGenericParameter || newType.IsGenericParameter
                ? oldType.IsGenericParameter
                    && newType.IsGenericParameter
                    && oldType.GenericParameterPosition == newType.GenericParameterPosition
                    && (oldType.DeclaringMethod == null) == (newType.DeclaringMethod == null)
                : this.GetNewType(oldType) == newType;
        }

        private ConstructorInfo FindConstructor(Type declaringType, ConstructorInfo oldConstructor)
        {
            ParameterInfo[] parameters = oldConstructor.GetParameters();
            Type[] types = new Type[parameters.Length];
            for (int i = 0; i < parameters.Length; i++)
            {
                types[i] = this.GetNewType(parameters[i].ParameterType);
            }

            return declaringType.GetConstructor(BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance, null, types, null);
        }

        private Type GetNewType(Type oldType)
        {
            if (oldType == null)
            {
                return null;
            }

            if (oldType.IsArray)
            {
                Type element = this.GetNewType(oldType.GetElementType());
                return oldType == oldType.GetElementType().MakeArrayType()
                    ? element.MakeArrayType()
                    : element.MakeArrayType(oldType.GetArrayRank());
            }
            if (oldType.IsByRef)
            {
                return this.GetNewType(oldType.GetElementType()).MakeByRefType();
            }

            if (oldType.IsPointer)
            {
                return this.GetNewType(oldType.GetElementType()).MakePointerType();
            }

            if (oldType.IsGenericParameter)
            {
                return oldType;
            }

            if (oldType.IsConstructedGenericType)
            {
                Type definition = this.GetNewType(oldType.GetGenericTypeDefinition());
                Type[] arguments = oldType.GetGenericArguments();
                for (int i = 0; i < arguments.Length; i++)
                {
                    arguments[i] = this.GetNewType(arguments[i]);
                }

                return definition.MakeGenericType(arguments);
            }
            if (!this.IsSwappedAssembly(oldType.Assembly))
            {
                return oldType;
            }

            Assembly replacementAssembly = this.GetNewAssembly(oldType.Assembly);
            Type result = replacementAssembly.GetType(oldType.FullName, false);
            return result ?? throw new TypeLoadException("Hot reload type '" + oldType.FullName + "' no longer exists.");
        }

        private object Construct(Type type)
        {
            if (type.IsValueType)
            {
                return Activator.CreateInstance(type);
            }

            if (type.IsAbstract || type.IsInterface)
            {
                throw new InvalidOperationException("Hot reload cannot construct '" + type + "'.");
            }

            HotReloadFactoryAttribute factoryAttribute = type.GetCustomAttribute<HotReloadFactoryAttribute>();
            if (factoryAttribute != null)
            {
                return ConstructWithFactory(type, factoryAttribute);
            }

            ConstructorInfo constructor = type.GetConstructor(BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic, null, Type.EmptyTypes, null);
            return constructor == null
                ? throw new MissingMethodException("Hot reload type '" + type + "' requires a parameterless constructor or a specialized upgrader.")
                : constructor.Invoke(null);
        }

        private static object ConstructWithFactory(Type type, HotReloadFactoryAttribute attribute)
        {
            MethodInfo factory = type.GetMethod(attribute.MethodName, BindingFlags.Static | BindingFlags.Public | BindingFlags.NonPublic, null, Type.EmptyTypes, null);
            if (factory == null || !type.IsAssignableFrom(factory.ReturnType))
            {
                throw new MissingMethodException("Hot reload factory '" + type.FullName + "." + attribute.MethodName + "' is missing or has an incompatible return type.");
            }

            object value = factory.Invoke(null, null);
            return value ?? throw new InvalidOperationException("Hot reload factory '" + type.FullName + "." + attribute.MethodName + "' returned null.");
        }

        private bool IsSwappedAssembly(Assembly assembly)
        {
            return AssemblyLoadContext.GetLoadContext(assembly) == this.oldLoadContext;
        }

        private Assembly GetNewAssembly(Assembly assembly)
        {
            if (!this.IsSwappedAssembly(assembly))
            {
                return assembly;
            }

            if (assembly == this.oldAssembly)
            {
                return this.newAssembly;
            }

            AssemblyName name = assembly.GetName();
            foreach (Assembly candidate in this.newLoadContext.Assemblies)
            {
                if (candidate.GetName().Name == name.Name)
                {
                    return candidate;
                }
            }
            return this.newLoadContext.LoadFromAssemblyName(name);
        }
    }
}
