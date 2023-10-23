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
        public class EntityTemplate
        {
            public string name;
            
        }

        public sealed class TemplateManager
        {
            private delegate Property DeserializeProperty(string json);

            private class Template
            {
                private class PropertyData
                {
                    // TODO: We're currently deserializing from json, which is very slow. We should replace it with MemoryPack instead
                    public string json;
                    public DeserializeProperty Deserialize;
                }
                List<PropertyData> data;
            }

            // maps from property name to deserialize function
            private Dictionary<string, DeserializeProperty> deserializeFuncs = new Dictionary<string, DeserializeProperty>();
            // maps from template name to Template
            private Dictionary<string, Template> templates = new Dictionary<string, Template>();

            private static readonly TemplateManager instance = new TemplateManager();
            // Explicit static constructor to tell C# compiler
            // not to mark type as beforefieldinit
            static TemplateManager() { }
            private TemplateManager()
            {
            }

            /// <summary>
            /// Returns the template manager singleton instance.
            /// </summary>
            public static TemplateManager Instance
            {
                get
                {
                    return instance;
                }
            }

            /// <summary>
            /// Read all templates from path, and its subfolders, and load them
            /// </summary>
            /// <param name="path"></param>
            public void SetupTemplates(string path)
            {
                string[] files = System.IO.Directory.GetFiles(path, "*.json", System.IO.SearchOption.AllDirectories);
                foreach (string file in files) 
                {
                    // TODO: Load all templates so that we can instantiate them later.
                }
            }

            // register a property type
            public void RegisterPropertyType<T>() where T : Property
            {
                string name = typeof(T).Name;
                // local function specific for the type of property
                Property DeserializePropertyFunc<TYPE>(string json) => System.Text.Json.JsonSerializer.Deserialize<TYPE>(json) as Property;
                DeserializeProperty deserialize = DeserializePropertyFunc<T>;

                this.deserializeFuncs.Add(name, deserialize);
            }
        }
    }
}

