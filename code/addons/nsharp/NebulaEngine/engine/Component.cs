using System;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;
using System.Collections.Generic;

namespace Nebula
{
    namespace Game
    {
        public class ComponentManager
        {
            private readonly Dictionary<Type, uint> componentRegistry = [];

            private static readonly ComponentManager instance = new ComponentManager();
            // Explicit static constructor to tell C# compiler
            // not to mark type as beforefieldinit
            static ComponentManager() { }
            private ComponentManager()
            {
                // empty
            }

            /// <summary>
            /// Gets the manager instance
            /// </summary>
            public static ComponentManager Instance => instance;

            /// <summary>
            /// Retrieve the component id for a given NativeComponent
            /// </summary>
            public uint GetComponentId<T>() where T : NativeComponent
            {
                bool res = this.componentRegistry.TryGetValue(typeof(T), out uint id);
                Debug.Assert(res, "Could not find component id of provided component type!");
                return id;
            }

            /// <summary>
            /// Registers a type to an id
            /// </summary>
            public void RegisterComponent(Type type, uint id)
            {
                this.componentRegistry[type] = id;
            }


            /// <summary>
            /// Clears all registered components that belong to a specific assembly.
            /// </summary>
            internal void ClearPluginComponents(System.Reflection.Assembly assembly)
            {
                List<Type> remove = [];

                foreach (KeyValuePair<Type, uint> entry in this.componentRegistry)
                {
                    if (entry.Key.Assembly == assembly)
                    {
                        remove.Add(entry.Key);
                    }
                }

                for (int i = 0; i < remove.Count; i++)
                {
                    _ = this.componentRegistry.Remove(remove[i]);
                }
            }
        }

        /// <summary>
        /// All structs that inherit from this class will automatically be registered and searchable as native components
        /// </summary>
        public interface NativeComponent
        {
            // empty
        }

        /// <summary>
        /// Represents a native descriptor for a resource name. 
        /// You should only construct these by loading a descriptor from native/unmanaged api.
        /// </summary>
        [NativeCppClass]
        [StructLayout(LayoutKind.Sequential)]
        public readonly struct ResourceDescriptor
        {
            // represented in native code as Util::StringAtom
            public readonly IntPtr descriptor;
        }
    }
}
