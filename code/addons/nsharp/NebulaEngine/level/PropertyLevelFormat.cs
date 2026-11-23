using System;
using System.Buffers;
using System.Collections.Generic;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;
using System.Text.Json;
using System.Text.Json.Nodes;
using System.Text.Json.Serialization;
using Nebula.Game;

namespace Nebula
{
    /// <summary>
    /// The level format's contribution for managed properties.
    ///
    /// The native side treats what this produces as text it does not interpret: it writes the text as a
    /// node under the entity, and it hands the text it read back here. That is what lets a property hold
    /// anything the json writer can express, and what lets a level written with this extension still be
    /// saved by a build that does not have it.
    ///
    /// Entity references inside a property are written as the guid the file names the target by, and read
    /// back through the map the file gives. The level format cannot resolve them on its own, because it does
    /// not read this text, so it hands the map here before the call that needs it. Without a map a reference
    /// writes as null, which is what a build that has no file to resolve against gets.
    /// </summary>
    public static class PropertyLevelFormat
    {
        private static readonly Dictionary<string, Type> propertyTypes = [];

        // The file's guid map, both ways. Built from the pairs the level format hands over, because the
        // format's container is not something this side can read
        private static readonly Dictionary<string, UInt64> refByGuid = [];
        private static readonly Dictionary<UInt64, string> guidByRef = [];

        /// <summary>
        /// Text from a pointer the native side owns. The formats hand null for a text they do not have, which
        /// is what a merge with nothing on one side is, and the marshaler throws on a null pointer instead of
        /// answering with an empty string.
        /// </summary>
        public static string Utf8(IntPtr ptr, int length)
        {
            if (ptr == IntPtr.Zero)
            {
                return "";
            }

            return Marshal.PtrToStringUTF8(ptr, length);
        }

        private static readonly JsonSerializerOptions jsonOptions = new()
        {
            WriteIndented = false,
        };

        // The writer is given its own options, because Utf8JsonWriter takes JsonWriterOptions and the
        // converters live in JsonSerializerOptions
        private static readonly JsonWriterOptions writerOptions = new()
        {
            Indented = false,
        };

        // The editor's recorder. This is the only way the managed side reaches C++, and it is a no-op when
        // no editor is attached, which is what keeps a game build that has no editor working
        [DllImport("__Internal", EntryPoint = "RecordPropertyChange")]
        private static extern void RecordPropertyChange(ulong entityId, string path, string oldText, string newText);

        // The editor's recorder. A press on the override marker says which key it was pressed on, and what that
        // means is the editor's decision, so this only reports the key
        [DllImport("__Internal", EntryPoint = "RequestPropertyRevert")]
        private static extern void RequestPropertyRevert(ulong entityId, string path);

        // The editor's marker. This side cannot read it, so a row asks. C++ bool is one byte, which is not what
        // a P/Invoke bool marshals as by default
        [DllImport("__Internal", EntryPoint = "PropertyExtension_IsAuthored")]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool IsAuthoredCall(ulong entityId, string path);

        // The names the editor's dropdown lists. The panel reads names and text and has no way to read this
        // dictionary, so the registry is mirrored into C++ with these
        [DllImport("__Internal", EntryPoint = "PropertyExtension_AddName")]
        private static extern void AddNameCall(string name);

        [DllImport("__Internal", EntryPoint = "PropertyExtension_ClearNames")]
        private static extern void ClearNamesCall();

        static PropertyLevelFormat()
        {
            jsonOptions.Converters.Add(new Vector2Converter());
            jsonOptions.Converters.Add(new Vector3Converter());
            jsonOptions.Converters.Add(new Vector4Converter());
            jsonOptions.Converters.Add(new QuaternionConverter());
            jsonOptions.Converters.Add(new ColorConverter());
            jsonOptions.Converters.Add(new EntityConverter());
        }

        /// <summary>
        /// The file's guid map, as "guid:entityId" pairs. This is the only way the level format can give it
        /// here, because it does not read this text and cannot know which parts of it name entities. It is
        /// only valid for the duration of the call that sets it.
        /// </summary>
        public static void SetRefContext(string pairs)
        {
            refByGuid.Clear();
            guidByRef.Clear();
            if (string.IsNullOrEmpty(pairs))
            {
                return;
            }

            int pos = 0;
            while (pos < pairs.Length)
            {
                int end = pos;
                while (end < pairs.Length && pairs[end] != ';')
                {
                    end++;
                }

                int split = pairs.IndexOf(':', pos, end - pos);
                if (split > pos && UInt64.TryParse(pairs.Substring(split + 1, end - split - 1), out UInt64 id))
                {
                    string guid = pairs.Substring(pos, split - pos);
                    refByGuid[guid] = id;
                    guidByRef[id] = guid;
                }

                pos = end + 1;
            }
        }

        // The editor's marker. It lives in the scene and this side cannot read it, so a row asks for the key it
        // is drawing. A path with no field asks whether any field of the property is authored, which is what a
        // section row asks with
        public static bool IsAuthored(ulong entityId, string path)
        {
            return IsAuthoredCall(entityId, path);
        }

        // A key is a path: the property name, a dot, the field name. This is the only thing that crosses to the
        // editor for a field, so nothing on either side has to put the prefix back together
        private static string KeyPath(string propertyName, string fieldName)
        {
            return propertyName + "." + fieldName;
        }

        // Keys come across as one block of text, separated by a newline, because this side cannot read the
        // container C++ holds them in
        private static List<string> SplitKeys(string keys)
        {
            List<string> named = [];
            if (string.IsNullOrEmpty(keys))
            {
                return named;
            }

            int pos = 0;
            while (pos < keys.Length)
            {
                int end = pos;
                while (end < keys.Length && keys[end] != '\n')
                {
                    end++;
                }

                string key = keys.Substring(pos, end - pos);
                if (key.Length > 0)
                {
                    named.Add(key);
                }

                pos = end + 1;
            }

            return named;
        }

        /// <summary>
        /// The keys one text carries, as "property.field" paths. The level format cannot see inside the text,
        /// so this is how it knows which part of it an entity authored.
        /// </summary>
        public static List<string> CollectKeys(string text)
        {
            List<string> keys = [];
            JsonNode parsed = string.IsNullOrEmpty(text) ? null : JsonNode.Parse(text);
            JsonObject obj = parsed is JsonObject o ? o : null;
            if (obj == null)
            {
                return keys;
            }

            foreach (KeyValuePair<string, JsonNode> entry in obj)
            {
                // AsObject() throws when the node is not an object, so the type is checked rather than asked
                // for. A field whose value is an array is a leaf here, which is what the writer writes for a
                // vector
                JsonObject fields = entry.Value is JsonObject f ? f : null;
                if (fields == null)
                {
                    keys.Add(entry.Key);
                    continue;
                }

                foreach (KeyValuePair<string, JsonNode> field in fields)
                {
                    keys.Add(entry.Key + "." + field.Key);
                }
            }

            // Sorted, because every writer here produces the same order and the level format compares texts as
            // bytes. An order that differed between the writers would read as a change no value made
            keys.Sort((left, right) => string.CompareOrdinal(left, right));

            return keys;
        }

        public static int JsonKeys(string text, IntPtr outBuffer, int maxLen)
        {
            return WriteNames(CollectKeys(text), outBuffer, maxLen);
        }

        /// <summary>
        /// The entity references one text carries, as "path\tguid" lines. The level format cannot see inside
        /// the text, so this is how it knows which parts of it name entities, and it is what lets it put its
        /// own file's guids back into the text.
        ///
        /// Only the fields this side knows are entity references, so a property the text names but this build
        /// does not have cannot be answered for.
        /// </summary>
        public static List<string> CollectRefs(string text)
        {
            List<string> lines = [];
            JsonNode parsed = string.IsNullOrEmpty(text) ? null : JsonNode.Parse(text);
            JsonObject obj = parsed is JsonObject o ? o : null;
            if (obj == null)
            {
                return lines;
            }

            foreach (KeyValuePair<string, JsonNode> entry in obj)
            {
                // AsObject() throws when the node is not an object, so the type is checked rather than asked
                // for
                JsonObject fields = entry.Value is JsonObject f ? f : null;
                if (fields == null)
                {
                    continue;
                }

                Type type = GetPropertyType(entry.Key);
                if (type == null)
                {
                    continue;
                }

                foreach (FieldInfo field in GetSerializableFields(type))
                {
                    if (field.FieldType != typeof(Nebula.Game.Entity))
                    {
                        continue;
                    }

                    // A field the text does not carry names nothing, and a field it carries as null is a
                    // reference with no target rather than a reference the text left out
                    if (!fields.ContainsKey(field.Name))
                    {
                        continue;
                    }

                    JsonNode value = fields[field.Name];
                    if (value is not JsonValue valueString || valueString.GetValueKind() != JsonValueKind.String)
                    {
                        continue;
                    }

                    string guid = valueString.GetValue<string>();
                    if (string.IsNullOrEmpty(guid))
                    {
                        continue;
                    }

                    lines.Add(entry.Key + "." + field.Name + "\t" + guid);
                }
            }

            // Sorted, because every writer here produces the same order and the level format compares texts as
            // bytes. An order that differed between the writers would read as a change no value made
            lines.Sort((left, right) => string.CompareOrdinal(left, right));

            return lines;
        }

        public static int TextRefs(string text, IntPtr outBuffer, int maxLen)
        {
            return WriteNames(CollectRefs(text), outBuffer, maxLen);
        }

        /// <summary>
        /// The text with the named paths' references put as the given guids, as "path\tguid" lines. This is
        /// what lets a text that names one file's records be written into a file that names them by its own.
        /// A path given with no guid is written as nothing, which is what a file with no record for the target
        /// can carry.
        /// </summary>
        public static string RemapRefs(string text, string pairs)
        {
            JsonNode node = string.IsNullOrEmpty(text) ? null : JsonNode.Parse(text);
            JsonObject obj = node is JsonObject o ? o : null;
            if (obj == null)
            {
                return text;
            }

            // The paths are grouped by the property they name, because the text is written one property at a
            // time and a path is a property and a field rather than one key
            Dictionary<string, Dictionary<string, string>> byProperty = [];
            foreach (string line in SplitKeys(pairs))
            {
                int split = line.IndexOf('\t');
                if (split <= 0)
                {
                    continue;
                }

                string path = line.Substring(0, split);
                int dot = path.IndexOf('.');
                if (dot <= 0)
                {
                    continue;
                }

                string propertyName = path.Substring(0, dot);
                if (!byProperty.ContainsKey(propertyName))
                {
                    byProperty[propertyName] = [];
                }
                byProperty[propertyName][path.Substring(dot + 1)] = line.Substring(split + 1);
            }

            if (byProperty.Count == 0)
            {
                return text;
            }

            // Sorted, because every writer here produces the same order and the level format compares texts as
            // bytes. A property is written with its fields in name order, and the properties in name order
            List<string> names = [];
            foreach (KeyValuePair<string, JsonNode> entry in obj)
            {
                names.Add(entry.Key);
            }
            names.Sort((left, right) => string.CompareOrdinal(left, right));

            ArrayBufferWriter<byte> buffer = new();
            using (Utf8JsonWriter writer = new Utf8JsonWriter(buffer, writerOptions))
            {
                writer.WriteStartObject();

                foreach (string name in names)
                {
                    writer.WritePropertyName(name);

                    JsonObject fields = obj[name] is JsonObject f ? f : null;
                    if (fields == null)
                    {
                        WriteNode(writer, obj[name]);
                        continue;
                    }

                    Dictionary<string, string> remapped =
                        byProperty.TryGetValue(name, out Dictionary<string, string> named) ? named : null;

                    List<string> fieldNames = [];
                    foreach (KeyValuePair<string, JsonNode> field in fields)
                    {
                        fieldNames.Add(field.Key);
                    }
                    fieldNames.Sort((left, right) => string.CompareOrdinal(left, right));

                    writer.WriteStartObject();

                    foreach (string field in fieldNames)
                    {
                        writer.WritePropertyName(field);

                        if (remapped == null || !remapped.ContainsKey(field))
                        {
                            WriteNode(writer, fields[field]);
                            continue;
                        }

                        if (string.IsNullOrEmpty(remapped[field]))
                        {
                            writer.WriteNullValue();
                        }
                        else
                        {
                            writer.WriteStringValue(remapped[field]);
                        }
                    }

                    writer.WriteEndObject();
                }

                writer.WriteEndObject();
            }

            return Encoding.UTF8.GetString(buffer.WrittenSpan);
        }

        public static int RemapRefs(string text, string pairs, IntPtr outBuffer, int maxLen)
        {
            try
            {
                return WriteText(RemapRefs(text, pairs), outBuffer, maxLen);
            }
            catch (Exception exception)
            {
                Console.Error.WriteLine("[NSharp] Property level format could not remap refs: " + exception);
                return 0;
            }
        }

        /// <summary>
        /// What the base text gives when the keys the other text carries are put over it. A text that is not
        /// an object carries nothing, and a key the base does not have joins it.
        /// </summary>
        public static string MergeJson(string baseText, string overText)
        {
            JsonNode baseNode = string.IsNullOrEmpty(baseText) ? null : JsonNode.Parse(baseText);
            JsonNode overNode = string.IsNullOrEmpty(overText) ? null : JsonNode.Parse(overText);
            if (baseNode == null && overNode == null)
            {
                return "";
            }

            ArrayBufferWriter<byte> buffer = new();
            using (Utf8JsonWriter writer = new Utf8JsonWriter(buffer, writerOptions))
            {
                WriteMerged(writer, baseNode, baseNode != null, overNode, overNode != null);
            }

            return Encoding.UTF8.GetString(buffer.WrittenSpan);
        }

        public static int MergeJson(string baseText, string overText, IntPtr outBuffer, int maxLen)
        {
            return WriteText(MergeJson(baseText, overText), outBuffer, maxLen);
        }

        /// <summary>
        /// The text for the named keys, each value read from the base text, or the field's default when the
        /// base has nothing. A revert needs this because a key the chain does not carry has no value to put
        /// back, and only this side knows what its field's default is.
        /// </summary>
        public static string Resolve(string baseText, string keys)
        {
            List<string> named = SplitKeys(keys);
            if (named.Count == 0)
            {
                return "";
            }

            JsonNode baseNode = string.IsNullOrEmpty(baseText) ? null : JsonNode.Parse(baseText);
            JsonObject baseObj = baseNode is JsonObject b ? b : null;

            // The keys are grouped by the property they name, because the text is written one property at a
            // time and a key is a path rather than a field
            Dictionary<string, List<string>> byProperty = [];
            foreach (string key in named)
            {
                int split = key.IndexOf('.');
                if (split <= 0)
                {
                    continue;
                }

                string propertyName = key.Substring(0, split);
                if (!byProperty.ContainsKey(propertyName))
                {
                    byProperty[propertyName] = [];
                }
                byProperty[propertyName].Add(key.Substring(split + 1));
            }

            // Sorted, because every writer here produces the same order and the level format compares texts as
            // bytes. A property is written with its fields in name order, and the properties in name order
            List<string> names = [];
            foreach (KeyValuePair<string, List<string>> entry in byProperty)
            {
                names.Add(entry.Key);
                entry.Value.Sort((left, right) => string.CompareOrdinal(left, right));
            }
            names.Sort((left, right) => string.CompareOrdinal(left, right));

            ArrayBufferWriter<byte> buffer = new();
            using (Utf8JsonWriter writer = new Utf8JsonWriter(buffer, writerOptions))
            {
                writer.WriteStartObject();

                foreach (string name in names)
                {
                    Type type = GetPropertyType(name);
                    if (type == null)
                    {
                        // A property the text names but this build does not have. There is no default to
                        // answer with, so the key is left out rather than guessed at
                        continue;
                    }

                    JsonObject fields = baseObj != null && baseObj[name] is JsonObject f ? f : null;

                    // A field the base does not have falls back to what a default instance holds, which is
                    // what a revert to a prefab that never mentioned the field gives back
                    Nebula.Game.Property property = (Nebula.Game.Property)Activator.CreateInstance(type);

                    writer.WritePropertyName(name);
                    writer.WriteStartObject();

                    foreach (string field in byProperty[name])
                    {
                        writer.WritePropertyName(field);

                        if (fields != null && fields.ContainsKey(field))
                        {
                            // The node is null for a text that carries the field as null, which is the same as
                            // the key being missing, so ContainsKey is what tells the two apart
                            JsonNode value = fields[field];
                            if (value == null)
                            {
                                writer.WriteNullValue();
                            }
                            else
                            {
                                value.WriteTo(writer);
                            }
                            continue;
                        }

                        FieldInfo info = type.GetField(field, BindingFlags.Public | BindingFlags.Instance);
                        if (info == null)
                        {
                            writer.WriteNullValue();
                            continue;
                        }

                        JsonSerializer.Serialize(writer, info.GetValue(property), info.FieldType, jsonOptions);
                    }

                    writer.WriteEndObject();
                }

                writer.WriteEndObject();
            }

            return Encoding.UTF8.GetString(buffer.WrittenSpan);
        }

        public static int Resolve(string baseText, string keys, IntPtr outBuffer, int maxLen)
        {
            try
            {
                return WriteText(Resolve(baseText, keys), outBuffer, maxLen);
            }
            catch (Exception exception)
            {
                Console.Error.WriteLine("[NSharp] Property level format could not resolve keys: " + exception);
                return 0;
            }
        }

        // The merge writes through the same writer a capture writes through, so the text a merge produces
        // reads as the same bytes as the text the entity would be written with when nothing changed
        private static void WriteMerged(Utf8JsonWriter writer, JsonNode baseNode, bool hasBase, JsonNode overNode, bool hasOver)
        {
            if (!hasOver)
            {
                WriteNode(writer, baseNode);
                return;
            }

            // AsObject() throws when the node is not an object, so the type is checked rather than asked for
            JsonObject overObj = overNode is JsonObject o ? o : null;
            JsonObject baseObj = baseNode is JsonObject b ? b : null;

            // A leaf, or a leaf against an object. Whatever the override carries wins, even when it holds the
            // same value as the base. A diff would not do that: it would report no change for an equal value
            // and the field would start following the prefab
            if (overObj == null || baseObj == null)
            {
                WriteNode(writer, overNode);
                return;
            }

            // Sorted, because every writer here produces the same order and the level format compares texts as
            // bytes. An order that differed between the writers would read as a change no value made
            List<string> names = [];
            foreach (KeyValuePair<string, JsonNode> entry in baseObj)
            {
                names.Add(entry.Key);
            }
            foreach (KeyValuePair<string, JsonNode> entry in overObj)
            {
                // A key the override has and the base does not is something the entity authored on its own
                if (!names.Contains(entry.Key))
                {
                    names.Add(entry.Key);
                }
            }
            names.Sort((left, right) => string.CompareOrdinal(left, right));

            writer.WriteStartObject();

            foreach (string name in names)
            {
                writer.WritePropertyName(name);
                WriteMerged(writer, baseObj[name], baseObj.ContainsKey(name), overObj[name], overObj.ContainsKey(name));
            }

            writer.WriteEndObject();
        }

        // A node that is null is a value, not a missing key. The two have to be told apart, because a field the
        // override carries as null is still a field the instance authored
        private static void WriteNode(Utf8JsonWriter writer, JsonNode node)
        {
            if (node == null)
            {
                writer.WriteNullValue();
            }
            else
            {
                node.WriteTo(writer);
            }
        }

        private static int WriteNames(List<string> names, IntPtr outBuffer, int maxLen)
        {
            if (names.Count == 0)
            {
                return 0;
            }

            StringBuilder text = new();
            for (int i = 0; i < names.Count; i++)
            {
                if (i > 0)
                {
                    text.Append('\n');
                }
                text.Append(names[i]);
            }

            return WriteText(text.ToString(), outBuffer, maxLen);
        }

        private static int WriteText(string text, IntPtr outBuffer, int maxLen)
        {
            if (string.IsNullOrEmpty(text))
            {
                return 0;
            }

            byte[] bytes = Encoding.UTF8.GetBytes(text);
            return WriteBytes(bytes, outBuffer, maxLen);
        }

        private static int WriteBytes(byte[] bytes, IntPtr outBuffer, int maxLen)
        {
            if (bytes.Length == 0)
            {
                return 0;
            }
            if (bytes.Length > maxLen)
            {
                return bytes.Length;
            }

            Marshal.Copy(bytes, 0, outBuffer, bytes.Length);
            return bytes.Length;
        }

        /// <summary>
        /// The property types of an assembly, by the name the level file carries them under.
        /// </summary>
        public static void RegisterAssembly(Assembly assembly)
        {
            foreach (Type type in assembly.GetTypes())
            {
                // IsAssignableFrom is true for the type itself, so the base comes through as well. It is not a
                // property an entity can be given: it has no fields to draw and nothing to write, so it is only
                // noise in the add menu
                if (type.IsAbstract || type == typeof(Nebula.Game.Property) || !typeof(Nebula.Game.Property).IsAssignableFrom(type))
                {
                    continue;
                }
                propertyTypes[type.Name] = type;
            }

            MirrorNames();
        }

        /// <summary>
        /// The property types of an assembly that is going away.
        /// </summary>
        public static void ClearAssembly(Assembly assembly)
        {
            List<string> remove = [];
            foreach (KeyValuePair<string, Type> entry in propertyTypes)
            {
                if (entry.Value.Assembly == assembly)
                {
                    remove.Add(entry.Key);
                }
            }
            for (int i = 0; i < remove.Count; i++)
            {
                _ = propertyTypes.Remove(remove[i]);
            }

            MirrorNames();
        }

        // The mirror is rebuilt whole rather than patched, because an assembly can be registered more than
        // once and a reload takes types out again
        private static void MirrorNames()
        {
            ClearNamesCall();

            foreach (KeyValuePair<string, Type> entry in propertyTypes)
            {
                AddNameCall(entry.Key);
            }
        }

        /// <summary>
        /// The json object of one entity's properties, written into a buffer the native side owns.
        /// Answers with the length it needs when the buffer was too small, which is what makes the call
        /// be made again with a bigger one.
        /// </summary>
        public static int SerializeEntity(ulong entityId, IntPtr outBuffer, int maxLen)
        {
            try
            {
                // Game::Entity stores the world id in bits 32-39
                World world = World.Get((uint)(entityId >> 32));
                if (world == null)
                {
                    return 0;
                }

                Nebula.Game.Entity entity = ResolveEntity(world, entityId);
                if (entity == null)
                {
                    return 0;
                }

                byte[] json = BuildJson(entity, null);
                if (json.Length == 0)
                {
                    return 0;
                }
                if (json.Length > maxLen)
                {
                    return json.Length;
                }

                Marshal.Copy(json, 0, outBuffer, json.Length);
                return json.Length;
            }
            catch (Exception exception)
            {
                Console.Error.WriteLine("[NSharp] Property level format could not write entity: " + exception);
                return 0;
            }
        }

        /// <summary>
        /// The entity's text with only the keys it authored, as the newline separated paths the level format
        /// holds for it. This is what a linked entity writes: the rest of it is what its prefab chain gives,
        /// and writing the whole value here would make the instance stop following the prefab.
        /// </summary>
        public static int SerializeKeys(ulong entityId, string keys, IntPtr outBuffer, int maxLen)
        {
            try
            {
                World world = World.Get((uint)(entityId >> 32));
                if (world == null)
                {
                    return 0;
                }

                Nebula.Game.Entity entity = ResolveEntity(world, entityId);
                if (entity == null)
                {
                    return 0;
                }

                HashSet<string> authored = [];
                foreach (string key in SplitKeys(keys))
                {
                    authored.Add(key);
                }

                return WriteBytes(BuildJson(entity, authored), outBuffer, maxLen);
            }
            catch (Exception exception)
            {
                Console.Error.WriteLine("[NSharp] Property level format could not write entity keys: " + exception);
                return 0;
            }
        }

        /// <summary>
        /// One entity, put back from the json the file carried for it. Exact: the entity's properties are the
        /// nodes the text names, and what it does not name is taken off. A command holds the entity's whole text,
        /// so this is what makes a change that took one property off read as a change rather than as an addition.
        /// A property the text names but the assembly does not have is skipped, because there is no type to make
        /// it from.
        /// </summary>
        public static unsafe int ApplyEntity(ulong entityId, IntPtr jsonUtf8, int length)
        {
            try
            {
                World world = World.Get((uint)(entityId >> 32));
                if (world == null)
                {
                    return 0;
                }

                Nebula.Game.Entity entity = ResolveEntity(world, entityId);
                if (entity == null)
                {
                    return 0;
                }

                string text = Utf8(jsonUtf8, length);
                JsonNode node = string.IsNullOrEmpty(text) ? null : JsonNode.Parse(text);
                JsonObject obj = node is JsonObject o ? o : null;

                // An empty text is an entity with nothing on it, which is what a command holds when the property
                // it changed was the only one the entity had
                List<Nebula.Game.Property> stale = [];
                IReadOnlyList<Nebula.Game.Property> properties = entity.Properties;
                if (properties != null)
                {
                    for (int i = 0; i < properties.Count; i++)
                    {
                        Nebula.Game.Property property = properties[i];
                        if (property == null)
                        {
                            continue;
                        }

                        if (obj == null || !obj.ContainsKey(property.GetType().Name))
                        {
                            stale.Add(property);
                        }
                    }
                }

                // Collected first, because the removal is what changes the list it was read from
                for (int i = 0; i < stale.Count; i++)
                {
                    entity.RemoveProperty(stale[i]);
                }

                if (obj == null)
                {
                    return 1;
                }

                foreach (KeyValuePair<string, JsonNode> entry in obj)
                {
                    ApplyProperty(entity, entry.Key, entry.Value?.ToJsonString());
                }

                return 1;
            }
            catch (Exception exception)
            {
                Console.Error.WriteLine("[NSharp] Property level format could not read entity: " + exception);
                return 0;
            }
        }

        /// <summary>
        /// The write path the inspector uses. It does not write the field: it works out what the entity's text
        /// would be after the change and hands the before and the after text to the editor, which is what runs
        /// the change through a command. That is why the command is the only place a property changes, and why
        /// plugin code that sets a field directly does not record anything: a runtime change is not an authoring
        /// change.
        ///
        /// A null field means the property is left out, which is what a removal reports. The path is what the
        /// recorder builds the override marker from, and it is the only thing that says which key inside the text
        /// changed. A property the entity did not have is reported as the whole property, which is what the
        /// recorder reads as an add: every key the after text carries for it is the entity's own.
        /// </summary>
        public static void RecordChange(ulong entityId, Type type, string fieldName, string valueJson)
        {
            try
            {
                World world = World.Get((uint)(entityId >> 32));
                if (world == null)
                {
                    return;
                }

                Nebula.Game.Entity entity = ResolveEntity(world, entityId);
                if (entity == null)
                {
                    return;
                }

                string oldText = BuildChangeJson(entity, null, null, null);
                string newText = BuildChangeJson(entity, type, fieldName, valueJson);

                Nebula.Game.Property property = entity.GetProperty(type);
                string path = fieldName == null || property == null ? type.Name : KeyPath(type.Name, fieldName);

                RecordPropertyChange(entityId, path, oldText, newText);
            }
            catch (Exception exception)
            {
                Console.Error.WriteLine("[NSharp] Property level format could not record change: " + exception);
            }
        }

        // The values the widgets are working on. A drag answers on every frame it runs, so the value the widget
        // is holding has to survive from frame to frame; copying it from the property every frame would throw
        // away what the widget wrote and make the value snap back. Keyed with the entity, because the panel
        // draws one entity at a time and a value left from another one would be read as this entity's.
        private static readonly Dictionary<string, object> pendingValues = [];

        // Reused buffers. They are only valid for the duration of the draw
        private static readonly byte[] comboOptions = new byte[4096];
        private static readonly byte[] stringBuffer = new byte[1024];
        private static readonly float[] floatBuffer = new float[4];

        // The id the surface reads as no entity, and the same value Editor::InvalidEntityId is. It is not
        // Game::Entity::Invalid read back as an id, because that entity has no reserved bits and answers as
        // 0xFFFFFFFFFF. This side has no mirror for an entity it cannot resolve, so a reference with nothing
        // behind it is null here and the sentinel there
        private const ulong InvalidEntityId = 0xFFFFFFFFFFFFFFFF;

        /// <summary>
        /// The property section the inspector draws. The panel owns the window and calls this inside it, so
        /// everything here is inside the ImGui frame.
        ///
        /// A field the surface has no widget for is drawn as text rather than guessed at.
        /// </summary>
        public static void DrawProperties(ulong entityId)
        {
            try
            {
                World world = World.Get((uint)(entityId >> 32));
                if (world == null)
                {
                    return;
                }

                Nebula.Game.Entity entity = ResolveEntity(world, entityId);
                if (entity == null)
                {
                    return;
                }

                // The section title is drawn by the panel, because the panel owns the row the title is on. What
                // comes here is the content under it

                IReadOnlyList<Nebula.Game.Property> properties = entity.Properties;
                if (properties == null || properties.Count == 0)
                {
                    DrawSurface.TextDisabled("None");
                }
                else
                {
                    for (int i = 0; i < properties.Count; i++)
                    {
                        Nebula.Game.Property property = properties[i];
                        if (property == null)
                        {
                            continue;
                        }

                        DrawProperty(entityId, property);
                    }
                }

                // The values are dropped when no widget is still working on one. The command runs after the
                // draw, so the next frame reads what it applied. Dropping a value while its widget is still
                // working on it would make it snap back to what the entity has, which is what a drag looks like
                // when it is drawn from the model
                if (!DrawSurface.IsAnyItemActive())
                {
                    pendingValues.Clear();
                }
            }
            catch (Exception exception)
            {
                Console.Error.WriteLine("[NSharp] Property level format could not draw properties: " + exception);
            }
        }

        private static void DrawProperty(ulong entityId, Nebula.Game.Property property)
        {
            Type type = property.GetType();

            // The table does not scope ids, so the property name has to be pushed for the rows inside it
            DrawSurface.PushId(type.Name);

            bool tableBegun = false;

            try
            {
                // The remove button is reported as a change with an empty after text, which is what the command
                // reads as the property being gone. The before text is what the property has now, so the undo
                // puts it back with the value it had
                //
                // The marker is on the same row, drawn when the property has a field this instance authored. A
                // press is reported as a request rather than acted on here, because what a press means is the
                // editor's decision: it composes the text the prefab chain gives for the authored keys and runs
                // the command
                DrawSurface.RowCode row = DrawSurface.Section(type.Name, type.Name, IsAuthored(entityId, type.Name));
                if (row == DrawSurface.RowCode.Remove)
                {
                    // A null field means the property is left out of the after text, which is what the command
                    // reads as the property being gone. The before text is what the entity has now, so the undo
                    // puts it back with the value it had
                    RecordChange(entityId, type, null, null);
                    return;
                }
                if (row == DrawSurface.RowCode.Marker)
                {
                    RequestPropertyRevert(entityId, type.Name);
                    return;
                }

                bool open = DrawSurface.BeginTable(type.Name);

                // EndTable is called even when BeginTable answered false, because ImGui still wants the table
                // ended for one that was not drawn. It is not called for one that was never begun, which is the
                // frame the remove button was clicked on. The frame has to be left balanced even when a field
                // throws halfway through the table
                tableBegun = true;
                if (!open)
                {
                    return;
                }

                foreach (FieldInfo field in GetSerializableFields(type))
                {
                    DrawField(entityId, type, property, field);
                }
            }
            finally
            {
                if (tableBegun)
                {
                    DrawSurface.EndTable();
                }
                DrawSurface.PopId();
            }
        }

        private static void DrawField(ulong entityId, Type type, Nebula.Game.Property property, FieldInfo field)
        {
            string key = entityId + "." + type.Name + "." + field.Name;
            object current = pendingValues.TryGetValue(key, out object pending) ? pending : field.GetValue(property);

            bool changed = false;

            if (field.FieldType == typeof(int))
            {
                if (DrawSurface.RowInt(field.Name, (int)current, out int value, null))
                {
                    current = value;
                    changed = true;
                }
            }
            else if (field.FieldType == typeof(float))
            {
                if (DrawSurface.RowFloat(field.Name, (float)current, out float value, null))
                {
                    current = value;
                    changed = true;
                }
            }
            else if (field.FieldType == typeof(bool))
            {
                if (DrawSurface.RowBool(field.Name, (bool)current, out bool value, null))
                {
                    current = value;
                    changed = true;
                }
            }
            else if (field.FieldType == typeof(string))
            {
                string text = (string)current ?? "";
                int length = Encoding.UTF8.GetByteCount(text);
                if (length > stringBuffer.Length - 1)
                {
                    length = stringBuffer.Length - 1;
                }

                Encoding.UTF8.GetBytes(text, 0, length, stringBuffer, 0);
                stringBuffer[length] = 0;

                if (DrawSurface.RowString(field.Name, text, stringBuffer, null))
                {
                    int written = 0;
                    while (written < stringBuffer.Length && stringBuffer[written] != 0)
                    {
                        written++;
                    }

                    current = Encoding.UTF8.GetString(stringBuffer, 0, written);
                    changed = true;
                }
            }
            else if (field.FieldType.IsEnum)
            {
                if (DrawEnum(field.Name, field.FieldType, current, out int index, null))
                {
                    current = Enum.GetValues(field.FieldType).GetValue(index);
                    changed = true;
                }
            }
            else if (field.FieldType == typeof(Vector2))
            {
                Vector2 value = (Vector2)current;
                floatBuffer[0] = value.X;
                floatBuffer[1] = value.Y;

                if (DrawSurface.RowFloats(field.Name, floatBuffer, 2, null))
                {
                    value.X = floatBuffer[0];
                    value.Y = floatBuffer[1];
                    current = value;
                    changed = true;
                }
            }
            else if (field.FieldType == typeof(Vector3))
            {
                Vector3 value = (Vector3)current;
                floatBuffer[0] = value.X;
                floatBuffer[1] = value.Y;
                floatBuffer[2] = value.Z;

                if (DrawSurface.RowFloats(field.Name, floatBuffer, 3, null))
                {
                    value.X = floatBuffer[0];
                    value.Y = floatBuffer[1];
                    value.Z = floatBuffer[2];
                    current = value;
                    changed = true;
                }
            }
            else if (field.FieldType == typeof(Vector4))
            {
                Vector4 value = (Vector4)current;
                floatBuffer[0] = value.X;
                floatBuffer[1] = value.Y;
                floatBuffer[2] = value.Z;
                floatBuffer[3] = value.W;

                if (DrawSurface.RowFloats(field.Name, floatBuffer, 4, null))
                {
                    value.X = floatBuffer[0];
                    value.Y = floatBuffer[1];
                    value.Z = floatBuffer[2];
                    value.W = floatBuffer[3];
                    current = value;
                    changed = true;
                }
            }
            else if (field.FieldType == typeof(Quaternion))
            {
                Quaternion value = (Quaternion)current;
                floatBuffer[0] = value.X;
                floatBuffer[1] = value.Y;
                floatBuffer[2] = value.Z;
                floatBuffer[3] = value.W;

                if (DrawSurface.RowFloats(field.Name, floatBuffer, 4, null))
                {
                    value.X = floatBuffer[0];
                    value.Y = floatBuffer[1];
                    value.Z = floatBuffer[2];
                    value.W = floatBuffer[3];
                    current = value;
                    changed = true;
                }
            }
            else if (field.FieldType == typeof(Color))
            {
                // Color's components are bytes, so the drags work on floats and the result is clamped back
                Color value = (Color)current;
                floatBuffer[0] = (float)value.R;
                floatBuffer[1] = (float)value.G;
                floatBuffer[2] = (float)value.B;
                floatBuffer[3] = (float)value.A;

                if (DrawSurface.RowFloats(field.Name, floatBuffer, 4, null))
                {
                    value.R = ToByte(floatBuffer[0]);
                    value.G = ToByte(floatBuffer[1]);
                    value.B = ToByte(floatBuffer[2]);
                    value.A = ToByte(floatBuffer[3]);
                    current = value;
                    changed = true;
                }
            }
            else if (field.FieldType == typeof(Nebula.Game.Entity))
            {
                // The name on the row is the scene's and the drop can only be accepted by the side that draws
                // with ImGui, so this row is the surface's. What comes back is an id, because that is the only
                // thing that crosses the boundary
                Nebula.Game.Entity reference = current as Nebula.Game.Entity;
                ulong value = reference?.Id ?? InvalidEntityId;

                if (DrawSurface.RowEntity(field.Name, value, out ulong dropped, null))
                {
                    // A reference is only something the file can name when the file has a record for the
                    // target, and an entity of another world is not something this property can be given, so a
                    // drop from another world is left as the value the row already had
                    World world = World.Get((uint)(entityId >> 32));
                    if (dropped == InvalidEntityId)
                    {
                        current = null;
                    }
                    else if (world != null && (uint)(dropped >> 32) == world.Id)
                    {
                        current = ResolveEntity(world, dropped);
                    }
                    else
                    {
                        return;
                    }

                    changed = true;
                }
            }
            else
            {
                // No widget for this field. It is shown as it reads rather than written through something that
                // would have to know the type
                DrawSurface.RowTextDisabled(field.Name, current?.ToString() ?? "null", null);
            }

            // The marker says this row is the instance's own. It is drawn after the widget, on the same row, and
            // a press is reported as a request rather than acted on here, because what a press means is the
            // editor's decision. The key is the path the editor keys its marker by, not the name the row shows
            string path = KeyPath(type.Name, field.Name);
            if (DrawSurface.RowMarker(path, IsAuthored(entityId, path), "This instance authored this field. Click to revert it to the prefab"))
            {
                // The value the widget was holding is not the entity's anymore, so the row has to read it back
                pendingValues.Remove(key);
                RequestPropertyRevert(entityId, path);
            }

            if (changed)
            {
                pendingValues[key] = current;
                RecordChange(entityId, type, field.Name, JsonSerializer.Serialize(current, field.FieldType, jsonOptions));
            }
        }

        // A drag here has no range, so a byte field has to be clamped when the value comes back
        private static byte ToByte(float value)
        {
            if (value < 0.0f)
                return 0;
            if (value > 255.0f)
                return 255;
            return (byte)(int)Math.Round(value);
        }

        // The combo takes the names as one buffer, separated by a zero byte and terminated by two
        private static bool DrawEnum(string key, Type enumType, object current, out int index, string description)
        {
            Array values = Enum.GetValues(enumType);

            index = 0;
            for (int i = 0; i < values.Length; i++)
            {
                if (values.GetValue(i)?.Equals(current) == true)
                {
                    index = i;
                    break;
                }
            }

            int offset = 0;
            for (int i = 0; i < values.Length; i++)
            {
                string name = values.GetValue(i)?.ToString() ?? "";
                int written = Encoding.UTF8.GetByteCount(name);
                if (offset + written + 1 >= comboOptions.Length)
                {
                    break;
                }

                Encoding.UTF8.GetBytes(name, 0, written, comboOptions, offset);
                comboOptions[offset + written] = 0;
                offset += written + 1;
            }

            if (offset == 0)
            {
                DrawSurface.RowTextDisabled(key, current?.ToString() ?? "null", description);
                return false;
            }

            comboOptions[offset] = 0;

            bool changed = DrawSurface.RowCombo(key, comboOptions, index, out int picked, description);
            if (changed)
            {
                index = picked;
            }
            return changed;
        }

        /// <summary>
        /// Whether the entity has the property. This is what makes the editor's dropdown row read as already
        /// added, the same way a component that is already on the entity does.
        /// </summary>
        public static int HasProperty(ulong entityId, string propertyName)
        {
            World world = World.Get((uint)(entityId >> 32));
            if (world == null)
            {
                return 0;
            }

            Nebula.Game.Entity entity = ResolveEntity(world, entityId);
            if (entity == null)
            {
                return 0;
            }

            Type type = GetPropertyType(propertyName);
            if (type == null)
            {
                return 0;
            }

            return entity.GetProperty(type) != null ? 1 : 0;
        }

        /// <summary>
        /// The text a default instance of the type produces, which is what an editor's add writes. It is the
        /// entity shape, because that is the shape a command holds, so an add composes it over what the entity
        /// already has rather than applying it on its own.
        /// </summary>
        public static int DefaultPropertyJson(string propertyName, IntPtr outBuffer, int maxLen)
        {
            if (!propertyTypes.TryGetValue(propertyName, out Type type))
            {
                return 0;
            }

            ArrayBufferWriter<byte> buffer = new();
            using (Utf8JsonWriter writer = new Utf8JsonWriter(buffer, writerOptions))
            {
                writer.WriteStartObject();
                writer.WritePropertyName(type.Name);
                WritePropertyFields(writer, type, (Nebula.Game.Property)Activator.CreateInstance(type), null, null, null);
                writer.WriteEndObject();
            }

            return WriteText(Encoding.UTF8.GetString(buffer.WrittenSpan), outBuffer, maxLen);
        }

        private static int ApplyProperty(Nebula.Game.Entity entity, string propertyName, string json)
        {
            Type type = GetPropertyType(propertyName);
            if (type == null)
            {
                Console.Error.WriteLine("[NSharp] Property level format does not know property '" + propertyName + "'");
                return 0;
            }

            if (string.IsNullOrEmpty(json))
            {
                return 0;
            }

            JsonNode node = JsonNode.Parse(json);
            JsonObject fields = node?.AsObject();
            if (fields == null)
            {
                return 0;
            }

            Nebula.Game.Property property = entity.GetProperty(type);
            if (property == null)
            {
                property = (Nebula.Game.Property)Activator.CreateInstance(type);
                entity.AddProperty(property);
            }

            foreach (FieldInfo field in GetSerializableFields(type))
            {
                if (!fields.ContainsKey(field.Name))
                {
                    continue;
                }

                JsonNode value = fields[field.Name];
                if (value == null)
                {
                    // A field the text has as null. Only a reference type can hold that, so a value type is
                    // left as it is rather than guessed at
                    if (!field.FieldType.IsValueType)
                    {
                        field.SetValue(property, null);
                    }
                    continue;
                }

                object typed = JsonSerializer.Deserialize(value.ToJsonString(), field.FieldType, jsonOptions);
                field.SetValue(property, typed);
            }

            return 1;
        }

        // The editor can save an entity in the same frame it was created, before the create queue has
        // been flushed. Mirroring it here is what keeps a property authored on such an entity from being
        // written as if the entity had none.
        private static Nebula.Game.Entity ResolveEntity(World world, UInt64 id)
        {
            Nebula.Game.Entity entity = world.GetEntity(id);
            return entity ?? world.RegisterEntityFromNative(id);
        }

        private static Type GetPropertyType(string propertyName)
        {
            propertyTypes.TryGetValue(propertyName, out Type type);
            return type;
        }

        // The same text as a string. The level format writes into a buffer the native side owns, which is
        // what the byte array is for, and this is the shape the test reads
        public static string BuildJsonText(Nebula.Game.Entity entity, HashSet<string> keys)
        {
            return Encoding.UTF8.GetString(BuildJson(entity, keys));
        }

        // The properties in name order. Sorted rather than left in the order they were added, because every
        // writer here produces the same order and the level format compares texts as bytes. An order that
        // differed between the writers would read as a change no value made
        private static List<Nebula.Game.Property> SortedProperties(Nebula.Game.Entity entity)
        {
            List<Nebula.Game.Property> sorted = [];
            IReadOnlyList<Nebula.Game.Property> properties = entity.Properties;
            if (properties != null)
            {
                for (int i = 0; i < properties.Count; i++)
                {
                    Nebula.Game.Property property = properties[i];
                    if (property != null)
                    {
                        sorted.Add(property);
                    }
                }
            }

            sorted.Sort((left, right) => string.CompareOrdinal(left.GetType().Name, right.GetType().Name));
            return sorted;
        }

        private static byte[] BuildJson(Nebula.Game.Entity entity, HashSet<string> keys)
        {
            List<Nebula.Game.Property> properties = SortedProperties(entity);
            if (properties.Count == 0)
            {
                return [];
            }

            ArrayBufferWriter<byte> buffer = new();
            using (Utf8JsonWriter writer = new Utf8JsonWriter(buffer, writerOptions))
            {
                writer.WriteStartObject();

                for (int i = 0; i < properties.Count; i++)
                {
                    Nebula.Game.Property property = properties[i];
                    Type type = property.GetType();

                    // A property with none of its keys authored is nothing the entity has to say about it:
                    // the chain already gives it exactly what it holds
                    if (keys != null && !AnyKeyFor(keys, type))
                    {
                        continue;
                    }

                    writer.WritePropertyName(type.Name);
                    WritePropertyFields(writer, type, property, null, null, keys);
                }

                writer.WriteEndObject();
            }

            return buffer.WrittenSpan.ToArray();
        }

        // The entity's text as it would read after a change, without changing the entity. The write path does not
        // write: the recorder needs the before and the after text, and the command is the only thing that changes
        // the property. A null field means the property is left out, which is what a removal reports. An entity
        // with nothing left has nothing to say, which is the same as what a capture answers with
        private static string BuildChangeJson(Nebula.Game.Entity entity, Type type, string fieldName, string valueJson)
        {
            List<Nebula.Game.Property> properties = SortedProperties(entity);

            if (fieldName == null)
            {
                properties.RemoveAll((property) => property.GetType() == type);
            }
            else
            {
                // A property the entity does not have yet has no text to record, so the after text is built from a
                // throwaway instance. Nothing is added to the entity here, because the command does that
                bool present = false;
                for (int i = 0; i < properties.Count; i++)
                {
                    if (properties[i].GetType() == type)
                    {
                        present = true;
                        break;
                    }
                }

                if (!present)
                {
                    properties.Add((Nebula.Game.Property)Activator.CreateInstance(type));
                }
            }

            if (properties.Count == 0)
            {
                return "";
            }

            ArrayBufferWriter<byte> buffer = new();
            using (Utf8JsonWriter writer = new Utf8JsonWriter(buffer, writerOptions))
            {
                writer.WriteStartObject();

                for (int i = 0; i < properties.Count; i++)
                {
                    Nebula.Game.Property property = properties[i];
                    Type propertyType = property.GetType();

                    writer.WritePropertyName(propertyType.Name);

                    // The field name is only the key inside the property the change is about. Two properties
                    // can have a field with the same name, and writing the value into every one of them would
                    // change properties the change never touched
                    WritePropertyFields(writer, propertyType, property, propertyType == type ? fieldName : null, valueJson, null);
                }

                writer.WriteEndObject();
            }

            return Encoding.UTF8.GetString(buffer.WrittenSpan);
        }

        private static bool AnyKeyFor(HashSet<string> keys, Type type)
        {
            foreach (FieldInfo field in GetSerializableFields(type))
            {
                if (keys.Contains(KeyPath(type.Name, field.Name)))
                {
                    return true;
                }
            }
            return false;
        }

        // One property's fields as text. The override is what lets the write path know what the text would be
        // after a field is written, without writing it: the recorder needs the before and the after text, and
        // the command is the only thing that changes the property. The keys are what a linked entity writes,
        // and null means every field.
        private static void WritePropertyFields(Utf8JsonWriter writer, Type type, Nebula.Game.Property property, string fieldName, string valueJson, HashSet<string> keys)
        {
            writer.WriteStartObject();

            foreach (FieldInfo field in GetSerializableFields(type))
            {
                if (keys != null && !keys.Contains(KeyPath(type.Name, field.Name)))
                {
                    continue;
                }

                writer.WritePropertyName(field.Name);

                if (fieldName != null && field.Name == fieldName)
                {
                    if (string.IsNullOrEmpty(valueJson))
                    {
                        writer.WriteNullValue();
                    }
                    else
                    {
                        writer.WriteRawValue(valueJson);
                    }
                    continue;
                }

                JsonSerializer.Serialize(writer, field.GetValue(property), field.FieldType, jsonOptions);
            }

            writer.WriteEndObject();
        }

        // The fields a property is written with, in name order. Sorted rather than left in reflection order,
        // because moving a field in the class would otherwise move it in the file as well.
        private static FieldInfo[] GetSerializableFields(Type type)
        {
            FieldInfo[] fields = type.GetFields(BindingFlags.Public | BindingFlags.Instance);
            Array.Sort(fields, (left, right) => string.CompareOrdinal(left.Name, right.Name));
            return fields;
        }

        // An entity reference, as the guid the file names the target by. Written through the map the level
        // format set, because a reference is only something the file can carry when the file has a record for
        // the target, and read back through the same map.
        private sealed class EntityConverter : JsonConverter<Nebula.Game.Entity>
        {
            public override Nebula.Game.Entity Read(ref Utf8JsonReader reader, Type typeToConvert, JsonSerializerOptions options)
            {
                if (reader.TokenType == JsonTokenType.Null)
                {
                    return null;
                }

                string guid = reader.GetString();
                if (string.IsNullOrEmpty(guid) || !refByGuid.TryGetValue(guid, out UInt64 id))
                {
                    return null;
                }

                World world = World.Get((uint)(id >> 32));
                if (world == null)
                {
                    return null;
                }

                return ResolveEntity(world, id);
            }

            public override void Write(Utf8JsonWriter writer, Nebula.Game.Entity value, JsonSerializerOptions options)
            {
                if (value == null)
                {
                    writer.WriteNullValue();
                    return;
                }

                if (guidByRef.TryGetValue(value.Id, out string guid))
                {
                    writer.WriteStringValue(guid);
                    return;
                }

                // A reference the file has no record for is nothing the file can name, which is what a
                // reference at an entity outside the instance comes out as
                writer.WriteNullValue();
            }
        }

        private sealed class Vector2Converter : JsonConverter<Vector2>
        {
            public override Vector2 Read(ref Utf8JsonReader reader, Type typeToConvert, JsonSerializerOptions options)
            {
                float[] values = JsonSerializer.Deserialize<float[]>(ref reader, options);
                return new Vector2(values[0], values[1]);
            }

            public override void Write(Utf8JsonWriter writer, Vector2 value, JsonSerializerOptions options)
            {
                writer.WriteStartArray();
                writer.WriteNumberValue(value.X);
                writer.WriteNumberValue(value.Y);
                writer.WriteEndArray();
            }
        }

        private sealed class Vector3Converter : JsonConverter<Vector3>
        {
            public override Vector3 Read(ref Utf8JsonReader reader, Type typeToConvert, JsonSerializerOptions options)
            {
                float[] values = JsonSerializer.Deserialize<float[]>(ref reader, options);
                return new Vector3(values[0], values[1], values[2]);
            }

            public override void Write(Utf8JsonWriter writer, Vector3 value, JsonSerializerOptions options)
            {
                writer.WriteStartArray();
                writer.WriteNumberValue(value.X);
                writer.WriteNumberValue(value.Y);
                writer.WriteNumberValue(value.Z);
                writer.WriteEndArray();
            }
        }

        private sealed class Vector4Converter : JsonConverter<Vector4>
        {
            public override Vector4 Read(ref Utf8JsonReader reader, Type typeToConvert, JsonSerializerOptions options)
            {
                float[] values = JsonSerializer.Deserialize<float[]>(ref reader, options);
                return new Vector4(values[0], values[1], values[2], values[3]);
            }

            public override void Write(Utf8JsonWriter writer, Vector4 value, JsonSerializerOptions options)
            {
                writer.WriteStartArray();
                writer.WriteNumberValue(value.X);
                writer.WriteNumberValue(value.Y);
                writer.WriteNumberValue(value.Z);
                writer.WriteNumberValue(value.W);
                writer.WriteEndArray();
            }
        }

        private sealed class QuaternionConverter : JsonConverter<Quaternion>
        {
            public override Quaternion Read(ref Utf8JsonReader reader, Type typeToConvert, JsonSerializerOptions options)
            {
                float[] values = JsonSerializer.Deserialize<float[]>(ref reader, options);
                return new Quaternion(values[0], values[1], values[2], values[3]);
            }

            public override void Write(Utf8JsonWriter writer, Quaternion value, JsonSerializerOptions options)
            {
                writer.WriteStartArray();
                writer.WriteNumberValue(value.X);
                writer.WriteNumberValue(value.Y);
                writer.WriteNumberValue(value.Z);
                writer.WriteNumberValue(value.W);
                writer.WriteEndArray();
            }
        }

        private sealed class ColorConverter : JsonConverter<Color>
        {
            public override Color Read(ref Utf8JsonReader reader, Type typeToConvert, JsonSerializerOptions options)
            {
                float[] values = JsonSerializer.Deserialize<float[]>(ref reader, options);
                return new Color(values[0], values[1], values[2], values[3]);
            }

            public override void Write(Utf8JsonWriter writer, Color value, JsonSerializerOptions options)
            {
                writer.WriteStartArray();
                writer.WriteNumberValue(value.R);
                writer.WriteNumberValue(value.G);
                writer.WriteNumberValue(value.B);
                writer.WriteNumberValue(value.A);
                writer.WriteEndArray();
            }
        }
    }
}
