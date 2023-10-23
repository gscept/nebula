using System;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;
using System.Collections;
using System.Collections.Generic;
using Mathf;
using Nebula.Game;

namespace Nebula
{
    namespace Game
    {
        public class ComponentManager
        {
            private Dictionary<Type, uint> componentRegistry = new Dictionary<Type, uint>();

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
            public static ComponentManager Instance
            {
                get
                {
                    return instance;
                }
            }

            /// <summary>
            /// Retrieve the component id for a given NativeComponent
            /// </summary>
            public uint GetComponentId<T>() where T : NativeComponent
            {
                uint id = 0xFFFFFFFF;
                bool res = componentRegistry.TryGetValue(typeof(T), out id);
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
        public struct ResourceDescriptor
        {
            // represented in native code as Util::StringAtom
            public readonly IntPtr descriptor;
        }
    }
}
