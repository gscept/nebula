using System;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;
using System.Collections;
using System.Collections.Generic;
using Mathf;
using System.Reflection;
using System.Linq;

namespace Nebula
{
    namespace Game
    {
        public sealed class PropertyManager
        {
            private static readonly PropertyManager instance = new PropertyManager();
            // Explicit static constructor to tell C# compiler
            // not to mark type as beforefieldinit
            static PropertyManager()
            {

            }
            private PropertyManager() { }

            public static PropertyManager Instance
            {
                get
                {
                    return instance;
                }
            }

            public void RegisterProperty(Property property)
            {
                List<Property> properties;
                bool exists = this.registry.TryGetValue(property.GetType(), out properties);
                if (!exists)
                {
                    properties = new List<Property>();
                    this.registry.Add(property.GetType(), properties);
                }
                properties.Add(property);                
            }

            public void PrintAllProperties()
            {
                foreach (var item in registry)
                {
                    Console.Write(item.Key.ToString());
                    Console.Write(":\n");
                    foreach (var prop in item.Value)
                    {
                        Console.Write("\t");
                        Console.Write(prop.ToString());
                        Console.Write("\n");
                    }
                    
                }
            }

            private Dictionary<System.Type, List<Property>> registry = new Dictionary<System.Type, List<Property>>();
        }
    }
}

